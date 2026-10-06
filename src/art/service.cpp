// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/service.h"

#include "art/http.h"
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
    m_thread = std::jthread([this](std::stop_token stop) { Guarded("art worker", [&] { Run(stop); }); });
}

void Service::Stop() {
    if (m_thread.joinable()) {
        m_thread.request_stop();
        if (m_http) {
            m_http->Cancel();
        }
        m_cv.notify_all();
        m_thread.join();
    }
}

std::string Service::KeyFor(const TrackInfo& track) {
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

std::optional<std::string> Service::Resolve(const TrackInfo& track) {
    const auto key = KeyFor(track);
    if (key.empty()) {
        return std::nullopt;
    }
    {
        std::scoped_lock lock(m_mutex);
        if (const auto it = m_cache.find(key); it != m_cache.end() && IsFreshLocked(it->second)) {
            return it->second.url.empty() ? std::nullopt : std::optional(it->second.url);
        }
        if (const auto it = m_retry_after.find(key); it != m_retry_after.end() && std::chrono::steady_clock::now() < it->second) {
            return std::nullopt;
        }
        m_pending = { key, track };
    }
    m_cv.notify_all();
    return std::nullopt;
}

void Service::ClearAll() {
    std::scoped_lock lock(m_mutex);
    m_cache.clear();
    m_retry_after.clear();
    SaveLocked();
}

void Service::Clear(const std::string& key) {
    std::scoped_lock lock(m_mutex);
    m_cache.erase(key);
    m_retry_after.erase(key);
    SaveLocked();
}

size_t Service::CachedCount() const {
    std::scoped_lock lock(m_mutex);
    return m_cache.size();
}

bool Service::IsFreshLocked(const Entry& entry) const {
    return !entry.url.empty() || NowSeconds() - entry.saved_at < kNegativeTtlSeconds;
}

void Service::Run(std::stop_token stop) {
    while (!stop.stop_requested()) {
        std::pair<std::string, TrackInfo> job;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, stop, [&] { return m_pending.has_value(); });
            if (stop.stop_requested()) {
                break;
            }
            // 等一下，若期間又切歌就改查最新那首。
            const auto key = m_pending->first;
            m_cv.wait_for(lock, stop, kDebounce, [&] { return !m_pending || m_pending->first != key; });
            if (stop.stop_requested() || !m_pending) {
                continue;
            }
            job = std::move(*m_pending);
            m_pending.reset();
            if (m_cache.contains(job.first) && IsFreshLocked(m_cache.at(job.first))) {
                continue;
            }
        }

        LookupResult result;
        try {
            result = LookupCoverArt(*m_http, job.second);
        } catch (const std::exception& e) {
            Log("cover art lookup failed: {}", e.what());
            result.status = LookupStatus::network_error;
        }
        if (stop.stop_requested()) {
            break;
        }

        {
            std::scoped_lock lock(m_mutex);
            if (result.status == LookupStatus::network_error) {
                m_retry_after[job.first] = std::chrono::steady_clock::now() + kRetryAfterError;
                Log("cover art lookup for \"{}\" failed, will retry later", job.first);
                continue;
            }
            m_retry_after.erase(job.first);
            m_cache[job.first] = { result.url, NowSeconds() };
            SaveLocked();
        }
        if (result.status == LookupStatus::found) {
            fb2k::inMainThread([] { Guarded("art refresh", [] { presence::Refresh(); }); });
        }
    }
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

void Service::SaveLocked() const {
    if (m_file.empty()) {
        return;
    }
    nlohmann::json entries = nlohmann::json::object();
    for (const auto& [key, entry] : m_cache) {
        entries[key] = { { "url", entry.url }, { "t", entry.saved_at } };
    }
    const nlohmann::json doc = { { "version", kCacheVersion }, { "entries", std::move(entries) } };

    std::error_code ec;
    std::filesystem::create_directories(m_file.parent_path(), ec);
    auto tmp = m_file;
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
    if (!MoveFileExW(tmp.c_str(), m_file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        Log("could not replace cover art cache ({})", GetLastError());
    }
}

} // namespace fdl::art
