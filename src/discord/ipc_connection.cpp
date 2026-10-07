// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "discord/ipc_connection.h"
#include "json_util.h"

#include <chrono>
#include <thread>

namespace fdl::discord {
namespace {

#pragma pack(push, 1)
struct FrameHeader {
    uint32_t opcode;
    uint32_t length;
};
#pragma pack(pop)

// Discord 端單一封包上限是 64 KiB，超過代表資料流已經錯亂。
constexpr uint32_t kMaxPayload = 64 * 1024;
constexpr auto kReadyTimeout = std::chrono::seconds(5);

std::wstring& PipePrefix() {
    static std::wstring prefix = L"\\\\.\\pipe\\discord-ipc-";
    return prefix;
}

} // namespace

void SetPipePrefixForTesting(std::wstring prefix) {
    PipePrefix() = std::move(prefix);
}

IpcConnection::~IpcConnection() {
    Close();
}

bool IpcConnection::Open(const std::string& client_id, std::string& error, std::stop_token stop) {
    Close();

    for (int i = 0; i < 10 && !IsOpen(); ++i) {
        const auto name = PipePrefix() + std::to_wstring(i);
        m_pipe = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    }
    if (!IsOpen()) {
        error = "Discord is not running";
        return false;
    }

    if (!Send(Opcode::handshake, { { "v", 1 }, { "client_id", client_id } })) {
        error = "handshake failed";
        Close();
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + kReadyTimeout;
    while (IsOpen() && !stop.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        auto msg = Poll();
        if (!msg) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if (msg->opcode == Opcode::close) {
            // 例如 client_id 無效時，Discord 會回 CLOSE 並附上 message。
            error = json::GetString(msg->payload, "message", "connection closed by Discord");
            Close();
            return false;
        }
        if (msg->opcode == Opcode::frame && json::GetString(msg->payload, "evt") == "READY") {
            return true;
        }
    }

    if (error.empty()) {
        error = "timed out waiting for Discord";
    }
    Close();
    return false;
}

void IpcConnection::Close() {
    if (IsOpen()) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
}

bool IpcConnection::Send(Opcode opcode, const nlohmann::json& payload) {
    if (!IsOpen()) {
        return false;
    }
    const std::string body = payload.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    const FrameHeader header{ static_cast<uint32_t>(opcode), static_cast<uint32_t>(body.size()) };
    if (!WriteExact(&header, sizeof(header)) || !WriteExact(body.data(), body.size())) {
        Close();
        return false;
    }
    return true;
}

std::optional<Message> IpcConnection::Poll() {
    if (!IsOpen()) {
        return std::nullopt;
    }
    DWORD available = 0;
    if (!PeekNamedPipe(m_pipe, nullptr, 0, nullptr, &available, nullptr)) {
        Close();
        return std::nullopt;
    }
    if (available < sizeof(FrameHeader)) {
        return std::nullopt;
    }
    return ReadBlocking();
}

std::optional<Message> IpcConnection::ReadBlocking() {
    FrameHeader header{};
    if (!ReadExact(&header, sizeof(header))) {
        return std::nullopt;
    }
    if (header.length > kMaxPayload) {
        Close();
        return std::nullopt;
    }
    std::string body(header.length, '\0');
    if (header.length > 0 && !ReadExact(body.data(), body.size())) {
        return std::nullopt;
    }

    Message msg{ static_cast<Opcode>(header.opcode), nlohmann::json::parse(body, nullptr, false) };
    if (msg.payload.is_discarded()) {
        msg.payload = nlohmann::json::object();
    }
    if (msg.opcode == Opcode::ping) {
        Send(Opcode::pong, msg.payload);
    }
    return msg;
}

bool IpcConnection::ReadExact(void* buffer, size_t size) {
    auto* p = static_cast<char*>(buffer);
    while (size > 0) {
        DWORD read = 0;
        if (!ReadFile(m_pipe, p, static_cast<DWORD>(size), &read, nullptr) || read == 0) {
            Close();
            return false;
        }
        p += read;
        size -= read;
    }
    return true;
}

bool IpcConnection::WriteExact(const void* buffer, size_t size) {
    const auto* p = static_cast<const char*>(buffer);
    while (size > 0) {
        DWORD written = 0;
        if (!WriteFile(m_pipe, p, static_cast<DWORD>(size), &written, nullptr) || written == 0) {
            return false;
        }
        p += written;
        size -= written;
    }
    return true;
}

} // namespace fdl::discord
