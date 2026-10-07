// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include "discord/activity.h"
#include "discord/variant.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace fdl::discord {

enum class ConnectionState {
    disabled,
    connecting,
    connected,
    error,
};

struct Status {
    ConnectionState state = ConnectionState::disabled;
    std::string message;
};

/// 在背景執行緒維持與 Discord 的連線。所有公開方法都是執行緒安全的，而且不會阻塞呼叫端。
class Client {
public:
    static Client& Get();

    void Start();
    void Stop();

    /// 空字串表示停用（斷線且不再重連）。variant 指定要連哪個版本的 Discord。
    void SetClientId(std::string client_id, ClientVariant variant = ClientVariant::any);
    /// nullopt 表示清除目前的 activity。只保留最新的一筆，舊的還沒送出就會被覆蓋。
    void SetActivity(std::optional<Activity> activity);

    Status GetStatus() const;

private:
    Client() = default;
    void Run(std::stop_token stop);

    mutable std::mutex m_mutex;
    std::condition_variable_any m_cv;
    std::string m_client_id;
    ClientVariant m_variant = ClientVariant::any;
    std::optional<Activity> m_activity;
    uint64_t m_activity_version = 0;
    Status m_status;
    std::jthread m_thread;
};

} // namespace fdl::discord
