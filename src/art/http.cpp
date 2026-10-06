// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/http.h"

#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace fdl::art {
namespace {

constexpr int kResolveTimeoutMs = 5000;
constexpr int kConnectTimeoutMs = 5000;
constexpr int kSendTimeoutMs = 10000;
constexpr int kReceiveTimeoutMs = 15000;
constexpr size_t kMaxBody = 4 * 1024 * 1024;

const wchar_t* UserAgent() {
    // MusicBrainz 要求 User-Agent 帶上應用名稱、版本與聯絡方式（這裡用專案網址）。
    static const std::wstring ua = std::wstring(L"foo_discord_listening/") + pfc::stringcvt::string_wide_from_utf8(FDL_VERSION).get_ptr()
        + L" ( https://github.com/Carinoasd/foo_discord_listening )";
    return ua.c_str();
}

std::wstring Widen(const std::string& s) {
    return pfc::stringcvt::string_wide_from_utf8(s.c_str()).get_ptr();
}

std::string LastErrorText(const char* what) {
    return std::string(what) + " failed (" + std::to_string(GetLastError()) + ")";
}

} // namespace

HttpClient::HttpClient() {
    m_session = WinHttpOpen(UserAgent(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (m_session) {
        WinHttpSetTimeouts(m_session, kResolveTimeoutMs, kConnectTimeoutMs, kSendTimeoutMs, kReceiveTimeoutMs);
    }
}

HttpClient::~HttpClient() {
    if (m_session) {
        WinHttpCloseHandle(m_session);
    }
}

HttpResponse HttpClient::Get(const std::string& url, bool follow_redirects) {
    return Send(L"GET", url, follow_redirects);
}

HttpResponse HttpClient::Head(const std::string& url, bool follow_redirects) {
    return Send(L"HEAD", url, follow_redirects);
}

void HttpClient::Cancel() {
    m_cancelled = true;
    std::scoped_lock lock(m_mutex);
    if (m_active_request) {
        // 關閉 handle 會讓另一個執行緒上阻塞中的 WinHTTP 呼叫立刻失敗返回。
        WinHttpCloseHandle(m_active_request);
        m_active_request = nullptr;
    }
}

HttpResponse HttpClient::Send(const wchar_t* method, const std::string& url, bool follow_redirects) {
    HttpResponse response;
    if (!m_session) {
        response.error = "WinHttpOpen failed";
        return response;
    }
    if (m_cancelled) {
        response.error = "cancelled";
        return response;
    }

    const std::wstring wurl = Widen(url);
    URL_COMPONENTS parts{ sizeof(parts) };
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &parts)) {
        response.error = "invalid URL";
        return response;
    }
    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    const std::wstring path = std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) + std::wstring(parts.lpszExtraInfo, parts.dwExtraInfoLength);

    HINTERNET connect = WinHttpConnect(m_session, host.c_str(), parts.nPort, 0);
    if (!connect) {
        response.error = LastErrorText("WinHttpConnect");
        return response;
    }
    const DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, method, path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        response.error = LastErrorText("WinHttpOpenRequest");
        WinHttpCloseHandle(connect);
        return response;
    }
    {
        std::scoped_lock lock(m_mutex);
        m_active_request = request;
    }
    // 結束時只關一次 request：若 Cancel() 已經關掉就不再重複關閉。
    auto finish = [&] {
        std::scoped_lock lock(m_mutex);
        if (m_active_request) {
            WinHttpCloseHandle(m_active_request);
            m_active_request = nullptr;
        }
        WinHttpCloseHandle(connect);
    };

    if (!follow_redirects) {
        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
    }
    const wchar_t* headers = L"Accept: application/json, */*\r\n";
    if (!WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(request, nullptr)) {
        response.error = m_cancelled ? "cancelled" : LastErrorText("HTTP request");
        finish();
        return response;
    }

    DWORD status = 0;
    DWORD size = sizeof(status);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);

    std::string body;
    if (wcscmp(method, L"HEAD") != 0) {
        for (;;) {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request, &available)) {
                response.error = m_cancelled ? "cancelled" : LastErrorText("WinHttpQueryDataAvailable");
                finish();
                return response;
            }
            if (available == 0) {
                break;
            }
            if (body.size() + available > kMaxBody) {
                response.error = "response too large";
                finish();
                return response;
            }
            const size_t offset = body.size();
            body.resize(offset + available);
            DWORD read = 0;
            if (!WinHttpReadData(request, body.data() + offset, available, &read)) {
                response.error = m_cancelled ? "cancelled" : LastErrorText("WinHttpReadData");
                finish();
                return response;
            }
            body.resize(offset + read);
        }
    }

    finish();
    response.status = static_cast<int>(status);
    response.body = std::move(body);
    return response;
}

} // namespace fdl::art
