// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include "art/musicbrainz.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace fdl::art {

class HttpClient;

/// 封面網址的取得與快取。查快取不會阻塞；沒命中時在背景抓取，完成後於主執行緒刷新 presence。
class Service {
public:
    static Service& Get();

    void Start(std::filesystem::path cache_file);
    void Stop();

    /// 依曲目資訊算出的快取 key。資訊不足以查詢時回傳空字串。
    static std::string KeyFor(const TrackInfo& track);

    /// 已知的封面網址；未知時排入背景抓取並回傳 nullopt。
    std::optional<std::string> Resolve(const TrackInfo& track);

    void ClearAll();
    void Clear(const std::string& key);
    size_t CachedCount() const;

private:
    struct Entry {
        std::string url;  ///< 空字串表示「確定沒有封面」
        int64_t saved_at = 0;
    };

    Service() = default;
    void Run(std::stop_token stop);
    void Load();
    void SaveLocked() const;
    bool IsFreshLocked(const Entry& entry) const;

    mutable std::mutex m_mutex;
    std::condition_variable_any m_cv;
    std::filesystem::path m_file;
    std::unordered_map<std::string, Entry> m_cache;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_retry_after;
    std::optional<std::pair<std::string, TrackInfo>> m_pending;
    std::shared_ptr<HttpClient> m_http;
    std::jthread m_thread;
};

} // namespace fdl::art
