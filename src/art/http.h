// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <mutex>
#include <string>

namespace fdl::art {

struct HttpResponse {
    int status = 0;     ///< 0 表示連線失敗或逾時
    std::string body;
    std::string error;  ///< status 為 0 時的原因
};

/// 同步的 WinHTTP 用戶端，只給背景執行緒使用。Cancel() 可從其他執行緒呼叫，用來在關閉時中斷進行中的請求。
class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    HttpResponse Get(const std::string& url, bool follow_redirects = true);
    HttpResponse Head(const std::string& url, bool follow_redirects = true);
    void Cancel();

private:
    HttpResponse Send(const wchar_t* method, const std::string& url, bool follow_redirects);

    void* m_session = nullptr;
    std::mutex m_mutex;
    void* m_active_request = nullptr;
    std::atomic<bool> m_cancelled = false;
};

/// 依 RFC 3986 做 percent-encoding（UTF-8 輸入）。
std::string UrlEncode(std::string_view text);

} // namespace fdl::art
