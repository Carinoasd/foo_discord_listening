// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/service.h"

#include "art/http.h"
#include "art/providers.h"
#include "art/local_art.h"
#include "art/uploader.h"
#include "json_util.h"
#include "log.h"
#include "presence.h"

#include <fstream>

namespace fdl::art {
namespace {

constexpr int kCacheVersion = 1;
// 「確定沒有封面」的結果保留一週，之後重查（MusicBrainz 上可能已經有人補上封面）。
constexpr int64_t kNegativeTtlSeconds = 7 * 24 * 3600;
// 網路錯誤不寫入快取，只在記憶體中暫停重試一段時間。
constexpr auto kRetryAfterError = std::chrono::minutes(10);
// 快速切歌時不必每首都查，等使用者停下來再查。
constexpr auto kDebounce = std::chrono::milliseconds(1500);
// 上傳前縮圖的最長邊。Discord 顯示的大圖不需要更大。
constexpr unsigned kUploadSize = 512;

constexpr std::string_view kManualPrefix = "manual:";

int64_t NowSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace

Service& Service::Get() {
    static Service instance;
    return instance;
}

void Service::Start(std::filesystem::path cache_file) {
    {
        std::scoped_lock lock(m_mutex);
        m_file = std::move(cache_file);
    }
    Load();
    m_http = std::make_shared<HttpClient>();
    m_thread = std::jthread([this](std::stop_token stop) {
        // WIC 需要 COM。
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        Guarded("art worker", [&] { Run(stop); });
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
    });
}

void Service::Stop() {
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_cancel = true;
        m_abort.abort();
        if (m_http) {
            m_http->Cancel();
        }
        m_cv.notify_all();
        m_thread.join();
    }
    // 在 on_quit 就釋放 WinHTTP，不要拖到 DLL 卸載時（loader lock 底下）才關閉。
    m_http.reset();
}

std::string Service::MusicBrainzKey(const TrackInfo& track) {
    if (IsMbid(track.release_mbid)) {
        return "release:" + AsciiLower(track.release_mbid);
    }
    if (IsMbid(track.release_group_mbid)) {
        return "release-group:" + AsciiLower(track.release_group_mbid);
    }
    return SearchKey("search:", track);
}

std::string Service::SearchKey(std::string_view prefix, const TrackInfo& track) {
    if (track.artist.empty() || track.album.empty()) {
        return {};
    }
    return std::string(prefix) + NormalizeTitle(track.artist) + "|" + NormalizeTitle(track.album);
}

Service::KeyState Service::StateLocked(const std::string& key, std::string* url) const {
    if (key.empty()) {
        return KeyState::none;
    }
    if (const auto it = m_cache.find(key); it != m_cache.end()) {
        const auto& entry = it->second;
        if (!entry.url.empty()) {
            *url = entry.url;
            return KeyState::found;
        }
        if (NowSeconds() - entry.saved_at < kNegativeTtlSeconds) {
            return KeyState::none;
        }
    }
    // 手動指定的來源沒有「去查」這回事：沒設定就是沒有。
    if (key.starts_with(kManualPrefix)) {
        return KeyState::none;
    }
    if (const auto it = m_retry_after.find(key); it != m_retry_after.end() && std::chrono::steady_clock::now() < it->second) {
        return KeyState::waiting;
    }
    return KeyState::unknown;
}

std::optional<std::string> Service::Resolve(ArtRequest request) {
    {
        std::scoped_lock lock(m_mutex);
        bool need_fetch = false;
        for (const auto& step : request.steps) {
            std::string url;
            switch (StateLocked(step.key, &url)) {
            case KeyState::found:
                // 已有結果就直接用；即使更前面的來源還沒查過，也不為了換圖再發請求。
                return url;
            case KeyState::unknown:
                need_fetch = true;
                break;
            case KeyState::none:
            case KeyState::waiting:
                break;
            }
        }
        if (!need_fetch) {
            return std::nullopt;
        }
        m_pending = std::move(request);
    }
    m_cv.notify_all();
    return std::nullopt;
}

void Service::SetManual(const std::string& key, const std::string& url) {
    {
        std::scoped_lock lock(m_mutex);
        if (url.empty()) {
            m_cache.erase(key);
        } else {
            m_cache[key] = { url, NowSeconds() };
        }
        ++m_cache_version;
    }
    Save();
}

std::optional<std::string> Service::GetManual(const std::string& key) const {
    std::scoped_lock lock(m_mutex);
    if (const auto it = m_cache.find(key); it != m_cache.end() && !it->second.url.empty()) {
        return it->second.url;
    }
    return std::nullopt;
}

void Service::ClearAll() {
    {
        std::scoped_lock lock(m_mutex);
        std::erase_if(m_cache, [](const auto& kv) { return !kv.first.starts_with(kManualPrefix); });
        m_retry_after.clear();
        ++m_cache_version;
    }
    Save();
}

void Service::Clear(const std::string& key) {
    {
        std::scoped_lock lock(m_mutex);
        m_cache.erase(key);
        m_retry_after.erase(key);
        ++m_cache_version;
    }
    Save();
}

size_t Service::CachedCount() const {
    std::scoped_lock lock(m_mutex);
    return m_cache.size();
}

void Service::Run(std::stop_token stop) {
    while (!stop.stop_requested()) {
        ArtRequest job;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, stop, [&] { return m_pending.has_value(); });
            if (stop.stop_requested()) {
                break;
            }
            // 快速切歌時不必每首都查：等一下，若期間又換了曲目就改處理最新那首。
            const auto track = m_pending->track;
            m_cv.wait_for(lock, stop, kDebounce, [&] { return !m_pending || m_pending->track != track; });
            if (stop.stop_requested() || !m_pending || m_pending->track != track) {
                continue;
            }
            job = std::move(*m_pending);
            m_pending.reset();
        }
        try {
            Process(job);
        } catch (const std::exception& e) {
            Log("cover art error: {}", e.what());
        }
    }
}

LookupResult Service::Fetch(const ArtRequest& request, const ArtStep& step) {
    switch (step.kind) {
    case SourceKind::musicbrainz:
        return LookupCoverArt(*m_http, request.info, request.musicbrainz_server);
    case SourceKind::itunes:
        return LookupITunes(*m_http, request.info, request.itunes_country);
    case SourceKind::lastfm:
        return LookupLastFm(*m_http, request.info, request.lastfm_api_key);
    case SourceKind::upload: {
        const auto image = ExportCoverArt(request.track, kUploadSize, m_abort);
        if (!image) {
            return { LookupStatus::not_found, {} };
        }
        const auto result = RunUploader(request.upload_command, *image, m_cancel);
        std::error_code ec;
        std::filesystem::remove(*image, ec);
        if (result.url.empty()) {
            if (!m_cancel) {
                Log("cover art upload failed: {}", result.error);
            }
            return { LookupStatus::network_error, {} };
        }
        return { LookupStatus::found, result.url };
    }
    case SourceKind::manual:
        break;
    }
    return { LookupStatus::not_found, {} };
}

void Service::Process(const ArtRequest& request) {
    for (const auto& step : request.steps) {
        KeyState state;
        {
            std::scoped_lock lock(m_mutex);
            std::string url;
            state = StateLocked(step.key, &url);
        }
        if (state == KeyState::found) {
            return;
        }
        if (state != KeyState::unknown) {
            continue; // 確定沒有，或暫時連不上而在等待重試：試下一個來源
        }
        DebugLog("art lookup: {}", step.key);
        const auto result = Fetch(request, step);
        if (m_cancel) {
            return;
        }
        if (result.status == LookupStatus::network_error) {
            DeferRetry(step.key, "temporary error");
            continue;
        }
        const bool found = result.status == LookupStatus::found;
        StoreResult(step.key, found ? std::optional(result.url) : std::nullopt);
        if (found) {
            return;
        }
    }
}

void Service::StoreResult(const std::string& key, std::optional<std::string> url) {
    DebugLog("art result for \"{}\": {}", key, url.value_or("(none)"));
    const bool found = url.has_value();
    {
        std::scoped_lock lock(m_mutex);
        m_retry_after.erase(key);
        m_cache[key] = { url.value_or(std::string{}), NowSeconds() };
        ++m_cache_version;
    }
    Save();
    if (found) {
        fb2k::inMainThread([] { Guarded("art refresh", [] { presence::Refresh(); }); });
    }
}

void Service::DeferRetry(const std::string& key, const std::string& reason) {
    Log("cover art for \"{}\": {}; will retry in 10 minutes", key, reason);
    std::scoped_lock lock(m_mutex);
    m_retry_after[key] = std::chrono::steady_clock::now() + kRetryAfterError;
}

void Service::Load() {
    std::scoped_lock lock(m_mutex);
    m_cache.clear();
    std::ifstream in(m_file, std::ios::binary);
    if (!in) {
        return;
    }
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    if (doc.is_discarded() || !doc.is_object() || json::GetInt(doc, "version") != kCacheVersion) {
        Log("cover art cache is unreadable or from another version, starting fresh");
        return;
    }
    const auto entries = doc.find("entries");
    if (entries == doc.end() || !entries->is_object()) {
        return;
    }
    for (const auto& [key, value] : entries->items()) {
        if (!value.is_object()) {
            continue;
        }
        Entry entry{ json::GetString(value, "url"), json::GetInt(value, "t") };
        // 只接受 https 網址，避免壞掉或被手動改壞的資料送到 Discord。
        if (!entry.url.empty() && !entry.url.starts_with("https://")) {
            continue;
        }
        m_cache.emplace(key, std::move(entry));
    }
}

void Service::Save() {
    nlohmann::json entries = nlohmann::json::object();
    std::filesystem::path file;
    uint64_t version = 0;
    {
        std::scoped_lock lock(m_mutex);
        if (m_file.empty()) {
            return;
        }
        for (const auto& [key, entry] : m_cache) {
            entries[key] = { { "url", entry.url }, { "t", entry.saved_at } };
        }
        file = m_file;
        version = m_cache_version;
    }

    std::scoped_lock file_lock(m_file_mutex);
    // 主執行緒與背景執行緒可能同時存檔；較舊的快照晚到時不要蓋掉較新的內容。
    if (version < m_saved_version) {
        return;
    }
    const nlohmann::json doc = { { "version", kCacheVersion }, { "entries", std::move(entries) } };
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    auto tmp = file;
    tmp += L".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        out << doc.dump(1);
        if (!out) {
            Log("could not write cover art cache");
            return;
        }
    }
    // 先寫暫存檔再取代，寫到一半當機也不會留下半個檔案。
    if (!MoveFileExW(tmp.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        Log("could not replace cover art cache ({})", GetLastError());
        return;
    }
    m_saved_version = version;
}

} // namespace fdl::art
