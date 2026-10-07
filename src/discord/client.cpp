// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "discord/client.h"

#include "discord/ipc_connection.h"
#include "json_util.h"
#include "log.h"
#include "i18n.h"

#include <chrono>
#include <deque>

namespace fdl::discord {
namespace {

using Clock = std::chrono::steady_clock;

constexpr auto kTick = std::chrono::milliseconds(500);
constexpr auto kMinRetry = std::chrono::seconds(2);
constexpr auto kMaxRetry = std::chrono::seconds(60);

// Discord 限制 SET_ACTIVITY 每 20 秒最多 5 次。
constexpr size_t kRateLimitCount = 5;
constexpr auto kRateLimitWindow = std::chrono::seconds(20);

class RateLimiter {
public:
    bool CanSend(Clock::time_point now) {
        while (!m_sent.empty() && now - m_sent.front() >= kRateLimitWindow) {
            m_sent.pop_front();
        }
        return m_sent.size() < kRateLimitCount;
    }
    void OnSent(Clock::time_point now) { m_sent.push_back(now); }

private:
    std::deque<Clock::time_point> m_sent;
};

} // namespace

Client& Client::Get() {
    static Client instance;
    return instance;
}

void Client::Start() {
    if (!m_thread.joinable()) {
        m_thread = std::jthread([this](std::stop_token stop) { Guarded("Discord worker", [&] { Run(stop); }); });
    }
}

void Client::Stop() {
    if (!m_thread.joinable()) {
        return;
    }
    m_thread.request_stop();
    m_cv.notify_all();
    // pipe 是同步 I/O：Discord 卡住時 ReadFile / WriteFile 可能一直阻塞，
    // 所以在等待期間反覆取消該執行緒上的同步 I/O，確保 foobar2000 能順利關閉。
    const auto handle = static_cast<HANDLE>(m_thread.native_handle());
    while (WaitForSingleObject(handle, 100) == WAIT_TIMEOUT) {
        CancelSynchronousIo(handle);
    }
    m_thread.join();
}

void Client::SetClientId(std::string client_id, ClientVariant variant) {
    {
        std::scoped_lock lock(m_mutex);
        if (m_client_id == client_id && m_variant == variant) {
            return;
        }
        m_client_id = std::move(client_id);
        m_variant = variant;
    }
    m_cv.notify_all();
}

namespace {

/// 時間戳取整到秒，跨過秒的邊界時會差 1 秒。這種差異不值得多用一次 Discord 的限流額度。
bool SameIgnoringJitter(const std::optional<Activity>& a, const std::optional<Activity>& b) {
    if (a.has_value() != b.has_value()) {
        return false;
    }
    if (!a) {
        return true;
    }
    auto close = [](const std::optional<int64_t>& x, const std::optional<int64_t>& y) {
        return x.has_value() == y.has_value() && (!x || std::llabs(*x - *y) <= 1000);
    };
    if (!close(a->start_ms, b->start_ms) || !close(a->end_ms, b->end_ms)) {
        return false;
    }
    Activity x = *a;
    Activity y = *b;
    x.start_ms = y.start_ms;
    x.end_ms = y.end_ms;
    return x == y;
}

} // namespace

void Client::SetActivity(std::optional<Activity> activity) {
    {
        std::scoped_lock lock(m_mutex);
        if (SameIgnoringJitter(m_activity, activity)) {
            return;
        }
        m_activity = std::move(activity);
        ++m_activity_version;
    }
    m_cv.notify_all();
}

Status Client::GetStatus() const {
    std::scoped_lock lock(m_mutex);
    return m_status;
}

void Client::Run(std::stop_token stop) {
    IpcConnection conn;
    RateLimiter limiter;
    std::string connected_id;
    uint64_t sent_version = 0;
    uint64_t seen_version = 0;
    std::string seen_id;
    bool sent_any = false;
    std::string last_error;
    auto retry_delay = kMinRetry;
    auto next_retry = Clock::now();
    uint64_t nonce = 0;

    auto set_status = [this](ConnectionState state, std::string message) {
        std::scoped_lock lock(m_mutex);
        m_status = { state, std::move(message) };
    };

    while (!stop.stop_requested()) {
        try {
            std::string client_id;
            ClientVariant variant = ClientVariant::any;
            std::optional<Activity> activity;
            uint64_t version = 0;
            {
                std::unique_lock lock(m_mutex);
                // 只在有新資料時提早醒來；被限流而尚未送出的更新由下一個 tick 處理，避免空轉。
                // 連線目標 = app ID + Discord 版本；任一改變都要重連。
                const auto target = [&] { return m_client_id.empty() ? std::string{} : m_client_id + "#" + std::to_string(static_cast<int>(m_variant)); };
                m_cv.wait_for(lock, stop, kTick, [&] { return m_activity_version != seen_version || target() != seen_id; });
                seen_id = target();
                client_id = m_client_id;
                variant = m_variant;
                activity = m_activity;
                version = seen_version = m_activity_version;
            }
            if (stop.stop_requested()) {
                break;
            }

            if (conn.IsOpen() && seen_id != connected_id) {
                conn.Close();
            }
            if (client_id.empty()) {
                connected_id.clear();
                set_status(ConnectionState::disabled, Tr(StringId::conn_disabled));
                continue;
            }

            const auto now = Clock::now();
            if (!conn.IsOpen()) {
                if (now < next_retry) {
                    continue;
                }
                set_status(ConnectionState::connecting, Tr(StringId::conn_connecting));
                std::string error;
                if (!conn.Open(client_id, error, stop, variant)) {
                    set_status(ConnectionState::error, error);
                    // Discord 沒開時會一直重試；同樣的錯誤只在主控台記一次，避免洗版。
                    if (error != last_error) {
                        Log("could not connect to Discord: {}", error);
                        last_error = error;
                    } else {
                        DebugLog("could not connect to Discord: {}", error);
                    }
                    next_retry = Clock::now() + retry_delay;
                    retry_delay = std::min<std::chrono::seconds>(retry_delay * 2, kMaxRetry);
                    continue;
                }
                Log("connected to {}", VariantName(conn.Variant()));
                last_error.clear();
                set_status(ConnectionState::connected, std::string(Tr(StringId::conn_connected)) + " (" + VariantName(conn.Variant()) + ")");
                connected_id = seen_id;
                retry_delay = kMinRetry;
                sent_any = false;
            }

            while (auto msg = conn.Poll()) {
                DebugLog("<- Discord op={} {}", static_cast<unsigned>(msg->opcode), msg->payload.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
                if (msg->opcode == Opcode::frame && json::GetString(msg->payload, "evt") == "ERROR") {
                    const auto& data = json::GetObject(msg->payload, "data");
                    Log("Discord rejected the activity (code {}): {}", json::GetInt(data, "code"), json::GetString(data, "message", "unknown error"));
                } else if (msg->opcode == Opcode::close) {
                    Log("Discord closed the connection: {}", json::GetString(msg->payload, "message"));
                    conn.Close();
                }
            }
            if (!conn.IsOpen()) {
                Log("disconnected from Discord");
                set_status(ConnectionState::error, Tr(StringId::conn_disconnected));
                next_retry = Clock::now() + retry_delay;
                continue;
            }

            if ((version != sent_version || !sent_any) && limiter.CanSend(now)) {
                nlohmann::json args = { { "pid", GetCurrentProcessId() } };
                if (activity) {
                    args["activity"] = ToJson(*activity);
                }
                const nlohmann::json payload = { { "cmd", "SET_ACTIVITY" }, { "args", std::move(args) }, { "nonce", std::to_string(++nonce) } };
                DebugLog("-> Discord {}", payload.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
            if (conn.Send(Opcode::frame, payload)) {
                    limiter.OnSent(now);
                    sent_version = version;
                    sent_any = true;
                }
            }
        } catch (const std::exception& e) {
            // 例如 Discord 回傳格式不符預期的 JSON。斷線後走正常的重連流程。
            Log("Discord connection error: {}", e.what());
            conn.Close();
            set_status(ConnectionState::error, e.what());
            next_retry = Clock::now() + retry_delay;
        } catch (...) {
            Log("Discord connection error");
            conn.Close();
            set_status(ConnectionState::error, "Unexpected error");
            next_retry = Clock::now() + retry_delay;
        }
    }

    // 不另外送清除訊息：pipe 關閉時 Discord 會自動清除狀態，也避免結束時多一個可能阻塞的寫入。
}

} // namespace fdl::discord
