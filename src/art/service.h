// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include "art/musicbrainz.h"

#include <atomic>
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

struct ArtRequest {
    TrackInfo info;
    metadb_handle_ptr track;
    std::string musicbrainz_key; ///< 空字串表示不查 MusicBrainz
    std::string upload_key;      ///< 空字串表示不上傳
    std::string upload_command;
};

/// 封面網址的取得與快取。查快取不會阻塞；沒命中時在背景處理，完成後於主執行緒刷新 presence。
/// 順序：先查 MusicBrainz（若啟用），找不到再上傳本機封面（若啟用）。
class Service {
public:
    static Service& Get();

    void Start(std::filesystem::path cache_file);
    void Stop();

    /// MusicBrainz 查詢用的快取 key。資訊不足以查詢時回傳空字串。
    static std::string MusicBrainzKey(const TrackInfo& track);

    /// 已知的封面網址；未知時排入背景處理並回傳 nullopt。
    std::optional<std::string> Resolve(ArtRequest request);

    void ClearAll();
    void Clear(const std::string& key);
    size_t CachedCount() const;

private:
    struct Entry {
        std::string url;  ///< 空字串表示「確定沒有封面」
        int64_t saved_at = 0;
    };
    enum class KeyState { found, none, unknown, waiting };

    Service() = default;
    void Run(std::stop_token stop);
    void Process(const ArtRequest& request);
    void Load();
    /// 在 m_mutex 內取得快取快照，鎖外寫檔，避免磁碟慢時擋住主執行緒的 Resolve。
    void Save();
    KeyState StateLocked(const std::string& key, std::string* url) const;
    void StoreResult(const std::string& key, std::optional<std::string> url);
    void DeferRetry(const std::string& key, const std::string& reason);

    mutable std::mutex m_mutex;
    std::condition_variable_any m_cv;
    std::filesystem::path m_file;
    std::unordered_map<std::string, Entry> m_cache;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_retry_after;
    std::optional<ArtRequest> m_pending;
    std::shared_ptr<HttpClient> m_http;
    std::atomic<bool> m_cancel = false;
    abort_callback_impl m_abort;
    std::mutex m_file_mutex;
    uint64_t m_cache_version = 0;   ///< 受 m_mutex 保護，每次快取變動 +1
    uint64_t m_saved_version = 0;   ///< 受 m_file_mutex 保護
    std::jthread m_thread;
};

} // namespace fdl::art
