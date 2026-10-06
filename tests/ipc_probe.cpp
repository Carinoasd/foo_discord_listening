// foo_discord_listening — Discord IPC 探測工具（本機測試用，不進交付包）
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT
//
// 用元件同一份 IpcConnection 連上本機的 Discord 並完成 handshake。
//   ipc_probe.exe                 以無效的 client ID 握手：預期 Discord 以錯誤關閉連線，不會改動任何狀態
//   ipc_probe.exe <app id>        以真正的 ID 握手，送出一筆測試 activity，10 秒後清除

#include "stdafx.h"

#include "discord/activity.h"
#include "discord/ipc_connection.h"

#include <chrono>
#include <cstdio>
#include <thread>

using namespace fdl::discord;

int main(int argc, char** argv) {
    const std::string client_id = argc > 1 ? argv[1] : "1";
    IpcConnection conn;
    std::string error;
    std::stop_source stop;
    const bool ok = conn.Open(client_id, error, stop.get_token());
    std::printf("handshake with client_id=%s: %s%s%s\n", client_id.c_str(), ok ? "READY" : "rejected", ok ? "" : " - ", error.c_str());
    if (!ok) {
        return argc > 1 ? 1 : (error.empty() ? 1 : 0);
    }

    Activity a;
    a.details = "ipc_probe test";
    a.state = "foo_discord_listening";
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    a.start_ms = now;
    a.end_ms = now + 180000;
    conn.Send(Opcode::frame, { { "cmd", "SET_ACTIVITY" }, { "args", { { "pid", GetCurrentProcessId() }, { "activity", ToJson(a) } } }, { "nonce", "1" } });
    for (int i = 0; i < 20; ++i) {
        while (auto msg = conn.Poll()) {
            std::printf("<- op=%u %s\n", static_cast<unsigned>(msg->opcode), msg->payload.dump().c_str());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    conn.Send(Opcode::frame, { { "cmd", "SET_ACTIVITY" }, { "args", { { "pid", GetCurrentProcessId() } } }, { "nonce", "2" } });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::printf("cleared\n");
    return 0;
}
