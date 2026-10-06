// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <stop_token>
#include <string>

namespace fdl::discord {

enum class Opcode : uint32_t {
    handshake = 0,
    frame = 1,
    close = 2,
    ping = 3,
    pong = 4,
};

struct Message {
    Opcode opcode;
    nlohmann::json payload;
};

/// Discord 本機 IPC（named pipe）的最低層封裝。非執行緒安全，只給 worker 執行緒使用。
class IpcConnection {
public:
    IpcConnection() = default;
    ~IpcConnection();
    IpcConnection(const IpcConnection&) = delete;
    IpcConnection& operator=(const IpcConnection&) = delete;

    /// 依序嘗試 discord-ipc-0..9，送出 handshake 並等待 READY。失敗時 error 會填入原因。
    bool Open(const std::string& client_id, std::string& error, std::stop_token stop);
    void Close();
    bool IsOpen() const { return m_pipe != INVALID_HANDLE_VALUE; }

    bool Send(Opcode opcode, const nlohmann::json& payload);
    /// 若 pipe 裡已有完整封包就讀出；沒有資料時立即回傳 nullopt。讀寫錯誤會關閉連線。
    std::optional<Message> Poll();

private:
    std::optional<Message> ReadBlocking();
    bool ReadExact(void* buffer, size_t size);
    bool WriteExact(const void* buffer, size_t size);

    HANDLE m_pipe = INVALID_HANDLE_VALUE;
};

} // namespace fdl::discord
