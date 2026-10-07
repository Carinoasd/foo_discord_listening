// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include "discord/variant.h"

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

/// 測試用：改變要連線的 pipe 名稱前綴（預設 \\.\pipe\discord-ipc-），讓測試連到假的 Discord。
void SetPipePrefixForTesting(std::wstring prefix);

/// Discord 本機 IPC（named pipe）的最低層封裝。非執行緒安全，只給 worker 執行緒使用。
class IpcConnection {
public:
    IpcConnection() = default;
    ~IpcConnection();
    IpcConnection(const IpcConnection&) = delete;
    IpcConnection& operator=(const IpcConnection&) = delete;

    /// 依序嘗試 discord-ipc-0..9，送出 handshake 並等待 READY；preferred 不是 any 時，略過其他版本的 Discord。
    /// 失敗時 error 會填入原因。
    bool Open(const std::string& client_id, std::string& error, std::stop_token stop, ClientVariant preferred = ClientVariant::any);
    /// 目前連上的 Discord 版本（由 READY 判斷）。
    ClientVariant Variant() const;
    void Close();
    bool IsOpen() const { return m_pipe != INVALID_HANDLE_VALUE; }
    /// 最近一次握手成功時 Discord 回傳的 READY 資料（含 config.api_endpoint 與 user）。
    const nlohmann::json& ReadyData() const { return m_ready; }

    bool Send(Opcode opcode, const nlohmann::json& payload);
    /// 若 pipe 裡已有完整封包就讀出；沒有資料時立即回傳 nullopt。讀寫錯誤會關閉連線。
    std::optional<Message> Poll();

private:
    /// 嘗試單一 pipe：握手並等待 READY。回傳 false 時 error 說明原因；pipe 不存在時 error 為空。
    bool TryPipe(const std::wstring& name, const std::string& client_id, std::string& error, std::stop_token stop);
    std::optional<Message> ReadBlocking();
    bool ReadExact(void* buffer, size_t size);
    bool WriteExact(const void* buffer, size_t size);

    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    nlohmann::json m_ready = nlohmann::json::object();
};

} // namespace fdl::discord
