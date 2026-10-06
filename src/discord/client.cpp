// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "discord/client.h"

#include "discord/ipc_connection.h"
#include "log.h"

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
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_cv.notify_all();
        m_thread.join();
    }
}

void Client::SetClientId(std::string client_id) {
    {
        std::scoped_lock lock(m_mutex);
        if (m_client_id == client_id) {
            return;
        }
        m_client_id = std::move(client_id);
    }
    m_cv.notify_all();
}

void Client::SetActivity(std::optional<Activity> activity) {
    {
        std::scoped_lock lock(m_mutex);
        if (m_activity == activity) {
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
            std::optional<Activity> activity;
            uint64_t version = 0;
            {
                std::unique_lock lock(m_mutex);
                // 只在有新資料時提早醒來；被限流而尚未送出的更新由下一個 tick 處理，避免空轉。
                m_cv.wait_for(lock, stop, kTick, [&] { return m_activity_version != seen_version || m_client_id != seen_id; });
                client_id = seen_id = m_client_id;
                activity = m_activity;
                version = seen_version = m_activity_version;
            }
            if (stop.stop_requested()) {
                break;
            }

            if (conn.IsOpen() && client_id != connected_id) {
                conn.Close();
            }
            if (client_id.empty()) {
                connected_id.clear();
                set_status(ConnectionState::disabled, "Disabled");
                continue;
            }

            const auto now = Clock::now();
            if (!conn.IsOpen()) {
                if (now < next_retry) {
                    continue;
                }
                set_status(ConnectionState::connecting, "Connecting...");
                std::string error;
                if (!conn.Open(client_id, error)) {
                    set_status(ConnectionState::error, error);
                    next_retry = Clock::now() + retry_delay;
                    retry_delay = std::min<std::chrono::seconds>(retry_delay * 2, kMaxRetry);
                    continue;
                }
                Log("connected to Discord");
                set_status(ConnectionState::connected, "Connected");
                connected_id = client_id;
                retry_delay = kMinRetry;
                sent_any = false;
            }

            while (auto msg = conn.Poll()) {
                if (msg->opcode == Opcode::frame && msg->payload.value("evt", std::string{}) == "ERROR") {
                    const auto& data = msg->payload.value("data", nlohmann::json::object());
                    Log("Discord rejected the activity: {}", data.value("message", std::string{ "unknown error" }));
                } else if (msg->opcode == Opcode::close) {
                    Log("Discord closed the connection: {}", msg->payload.value("message", std::string{}));
                    conn.Close();
                }
            }
            if (!conn.IsOpen()) {
                Log("disconnected from Discord");
                set_status(ConnectionState::error, "Disconnected");
                next_retry = Clock::now() + retry_delay;
                continue;
            }

            if ((version != sent_version || !sent_any) && limiter.CanSend(now)) {
                nlohmann::json args = { { "pid", GetCurrentProcessId() } };
                if (activity) {
                    args["activity"] = ToJson(*activity);
                }
                const nlohmann::json payload = { { "cmd", "SET_ACTIVITY" }, { "args", std::move(args) }, { "nonce", std::to_string(++nonce) } };
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

    if (conn.IsOpen()) {
        conn.Send(Opcode::frame, { { "cmd", "SET_ACTIVITY" }, { "args", { { "pid", GetCurrentProcessId() } } }, { "nonce", std::to_string(++nonce) } });
    }
}

} // namespace fdl::discord
