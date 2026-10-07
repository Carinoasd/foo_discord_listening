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
#include <vector>

namespace fdl::art {

class HttpClient;

enum class SourceKind { manual, musicbrainz, itunes, lastfm, upload };

/// 封面來源鏈中的一步：來源種類與它在快取中的 key。
struct ArtStep {
    SourceKind kind;
    std::string key;
};

struct ArtRequest {
    TrackInfo info;
    metadb_handle_ptr track;
    std::vector<ArtStep> steps; ///< 依優先順序排列
    std::string musicbrainz_server;
    std::string itunes_country;
    std::string lastfm_api_key;
    std::string upload_command;
};

/// 封面網址的取得與快取。查快取不會阻塞；沒命中時在背景處理，完成後於主執行緒刷新 presence。
/// 依 ArtRequest::steps 的順序嘗試各來源；某來源暫時連不上時改試下一個。
class Service {
public:
    static Service& Get();

    void Start(std::filesystem::path cache_file);
    void Stop();

    /// MusicBrainz 查詢用的快取 key。資訊不足以查詢時回傳空字串。
    static std::string MusicBrainzKey(const TrackInfo& track);
    /// 以歌手與專輯名稱為準的快取 key（iTunes、Last.fm 用）。資訊不足時回傳空字串。
    static std::string SearchKey(std::string_view prefix, const TrackInfo& track);

    /// 已知的封面網址；未知時排入背景處理並回傳 nullopt。
    std::optional<std::string> Resolve(ArtRequest request);

    /// 手動指定某張專輯的封面網址；空字串表示取消手動指定。手動指定的優先於所有自動來源，清除快取時保留。
    void SetManual(const std::string& key, const std::string& url);
    std::optional<std::string> GetManual(const std::string& key) const;

    /// 清除所有自動取得的封面（手動指定的保留）。
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
    LookupResult Fetch(const ArtRequest& request, const ArtStep& step);
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
