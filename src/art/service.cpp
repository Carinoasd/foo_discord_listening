// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/service.h"

#include "art/http.h"
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
    if (!track.artist.empty() && !track.album.empty()) {
        return "search:" + NormalizeTitle(track.artist) + "|" + NormalizeTitle(track.album);
    }
    return {};
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
    if (const auto it = m_retry_after.find(key); it != m_retry_after.end() && std::chrono::steady_clock::now() < it->second) {
        return KeyState::waiting;
    }
    return KeyState::unknown;
}

std::optional<std::string> Service::Resolve(ArtRequest request) {
    {
        std::scoped_lock lock(m_mutex);
        std::string url;
        const auto mb = StateLocked(request.musicbrainz_key, &url);
        if (mb == KeyState::found) {
            return url;
        }
        const auto up = request.upload_command.empty() ? KeyState::none : StateLocked(request.upload_key, &url);
        if (up == KeyState::found) {
            return url;
        }
        if (mb != KeyState::unknown && up != KeyState::unknown) {
            // 兩種來源都已確定沒有封面或正在等待重試。
            return std::nullopt;
        }
        m_pending = std::move(request);
    }
    m_cv.notify_all();
    return std::nullopt;
}

void Service::ClearAll() {
    {
        std::scoped_lock lock(m_mutex);
        m_cache.clear();
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

void Service::Process(const ArtRequest& request) {
    DebugLog("art lookup: musicbrainz_key=\"{}\" upload_key=\"{}\"", request.musicbrainz_key, request.upload_key);
    KeyState mb;
    KeyState up;
    {
        std::scoped_lock lock(m_mutex);
        std::string url;
        mb = StateLocked(request.musicbrainz_key, &url);
        up = request.upload_command.empty() ? KeyState::none : StateLocked(request.upload_key, &url);
    }

    if (mb == KeyState::unknown) {
        const auto result = LookupCoverArt(*m_http, request.info);
        if (m_cancel) {
            return;
        }
        if (result.status == LookupStatus::network_error) {
            DeferRetry(request.musicbrainz_key, "MusicBrainz request failed");
            return;
        }
        StoreResult(request.musicbrainz_key, result.status == LookupStatus::found ? std::optional(result.url) : std::nullopt);
        if (result.status == LookupStatus::found) {
            return;
        }
    } else if (mb == KeyState::found) {
        return;
    }
    // MusicBrainz 確定沒有封面，或暫時連不上（waiting）時，若有設定上傳就改用上傳。

    if (up != KeyState::unknown) {
        return;
    }
    const auto image = ExportCoverArt(request.track, kUploadSize, m_abort);
    if (!image) {
        StoreResult(request.upload_key, std::nullopt);
        return;
    }
    const auto result = RunUploader(request.upload_command, *image, m_cancel);
    std::error_code ec;
    std::filesystem::remove(*image, ec);
    if (m_cancel) {
        return;
    }
    if (result.url.empty()) {
        DeferRetry(request.upload_key, "upload failed: " + result.error);
        return;
    }
    StoreResult(request.upload_key, result.url);
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
