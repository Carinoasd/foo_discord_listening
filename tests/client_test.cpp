// foo_discord_listening — Client 與假 Discord 伺服器的整合測試（Windows，本機測試用，不進交付包）
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT
//
// 在自己的 named pipe 上模擬 Discord，驗證重連、限流、錯誤處理與關閉流程，不會碰到真正的 Discord。

#include "stdafx.h"

#include "discord/client.h"
#include "discord/ipc_connection.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace fdl::discord;
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

static int g_failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failed;                                                 \
        }                                                               \
    } while (0)

static bool WaitUntil(const std::function<bool()>& pred, std::chrono::milliseconds timeout) {
    const auto deadline = Clock::now() + timeout;
    while (Clock::now() < deadline) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(20ms);
    }
    return pred();
}

/// 最小的假 Discord：接受連線、回應 handshake、記錄收到的 SET_ACTIVITY。
class FakeDiscord {
public:
    enum class Mode { normal, reject, hang_after_ready };

    explicit FakeDiscord(std::wstring pipe_name) : m_name(std::move(pipe_name)) {
        m_thread = std::jthread([this](std::stop_token stop) { Run(stop); });
    }
    ~FakeDiscord() {
        m_thread.request_stop();
        // 叫醒卡在 ConnectNamedPipe 的伺服器執行緒。
        HANDLE h = CreateFileW(m_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
        }
    }

    void SetMode(Mode mode) { m_mode = mode; }

    /// 要求伺服器執行緒中斷目前的連線（同步 pipe 的 I/O 會排隊，不能從別的執行緒直接 Disconnect）。
    void DropClient() { m_drop = true; }

    std::vector<std::pair<Clock::time_point, nlohmann::json>> Activities() {
        std::scoped_lock lock(m_mutex);
        return m_activities;
    }
    void ClearActivities() {
        std::scoped_lock lock(m_mutex);
        m_activities.clear();
    }
    int Connections() const { return m_connections; }
    bool Connected() const { return m_connected; }

private:
    /// 等到 pipe 裡有完整的 header 才讀；期間可被 stop 或 DropClient 中斷。
    bool WaitForData(HANDLE pipe, std::stop_token stop) {
        for (;;) {
            if (stop.stop_requested() || m_drop) {
                return false;
            }
            DWORD available = 0;
            if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr)) {
                return false;
            }
            if (available >= 8) {
                return true;
            }
            std::this_thread::sleep_for(5ms);
        }
    }

    bool ReadFrame(HANDLE pipe, uint32_t& op, std::string& body) {
        uint32_t header[2];
        DWORD read = 0;
        if (!ReadFile(pipe, header, sizeof(header), &read, nullptr) || read != sizeof(header)) {
            return false;
        }
        op = header[0];
        body.assign(header[1], '\0');
        size_t got = 0;
        while (got < body.size()) {
            if (!ReadFile(pipe, body.data() + got, static_cast<DWORD>(body.size() - got), &read, nullptr) || read == 0) {
                return false;
            }
            got += read;
        }
        return true;
    }

    void WriteFrame(HANDLE pipe, uint32_t op, const nlohmann::json& payload) {
        const auto body = payload.dump();
        const uint32_t header[2] = { op, static_cast<uint32_t>(body.size()) };
        DWORD written = 0;
        WriteFile(pipe, header, sizeof(header), &written, nullptr);
        WriteFile(pipe, body.data(), static_cast<DWORD>(body.size()), &written, nullptr);
    }

    void Run(std::stop_token stop) {
        while (!stop.stop_requested()) {
            // hang 模式用極小的輸入緩衝區，Client 的寫入很快就會因為沒人讀而阻塞。
            const DWORD in_buffer = m_mode == Mode::hang_after_ready ? 1 : 64 * 1024;
            HANDLE pipe = CreateNamedPipeW(m_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT, 1, 64 * 1024, in_buffer, 0, nullptr);
            if (pipe == INVALID_HANDLE_VALUE) {
                std::this_thread::sleep_for(50ms);
                continue;
            }
            {
                std::scoped_lock lock(m_mutex);
                m_pipe = pipe;
            }
            if (ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
                if (!stop.stop_requested()) {
                    Serve(pipe, stop);
                }
            }
            {
                std::scoped_lock lock(m_mutex);
                m_pipe = INVALID_HANDLE_VALUE;
            }
            m_connected = false;
            m_drop = false;
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
        }
    }

    void Serve(HANDLE pipe, std::stop_token stop) {
        uint32_t op = 0;
        std::string body;
        if (!WaitForData(pipe, stop) || !ReadFrame(pipe, op, body) || op != 0) {
            return;
        }
        ++m_connections;
        if (m_mode == Mode::reject) {
            WriteFrame(pipe, 2, { { "code", 4000 }, { "message", "Invalid Client ID" } });
            std::this_thread::sleep_for(200ms);
            return;
        }
        WriteFrame(pipe, 1, { { "cmd", "DISPATCH" }, { "evt", "READY" }, { "data", { { "v", 1 } } } });
        m_connected = true;
        if (m_mode == Mode::hang_after_ready) {
            // 模擬 Discord 卡住：不再讀取任何資料，直到測試結束。
            while (!stop.stop_requested() && !m_drop && m_mode == Mode::hang_after_ready) {
                std::this_thread::sleep_for(50ms);
            }
            return;
        }
        while (WaitForData(pipe, stop) && ReadFrame(pipe, op, body)) {
            const auto msg = nlohmann::json::parse(body, nullptr, false);
            if (op == 1 && msg.is_object() && msg.value("cmd", "") == "SET_ACTIVITY") {
                {
                    std::scoped_lock lock(m_mutex);
                    m_activities.emplace_back(Clock::now(), msg["args"].value("activity", nlohmann::json()));
                }
                WriteFrame(pipe, 1, { { "cmd", "SET_ACTIVITY" }, { "evt", nullptr }, { "nonce", msg.value("nonce", "") } });
            }
        }
    }

    std::wstring m_name;
    std::atomic<Mode> m_mode = Mode::normal;
    std::mutex m_mutex;
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    std::vector<std::pair<Clock::time_point, nlohmann::json>> m_activities;
    std::atomic<int> m_connections = 0;
    std::atomic<bool> m_connected = false;
    std::atomic<bool> m_drop = false;
    std::jthread m_thread;
};

static Activity Song(const std::string& title) {
    Activity a;
    a.details = title;
    a.state = "Artist";
    return a;
}

static std::string LastDetails(FakeDiscord& server) {
    const auto list = server.Activities();
    if (list.empty() || !list.back().second.is_object()) {
        return {};
    }
    return list.back().second.value("details", "");
}

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const std::wstring prefix = L"\\\\.\\pipe\\fdl-test-" + std::to_wstring(GetCurrentProcessId()) + L"-";
    SetPipePrefixForTesting(prefix);
    auto& client = Client::Get();

    std::printf("== 連線與送出狀態\n");
    auto server = std::make_unique<FakeDiscord>(prefix + L"0");
    client.Start();
    client.SetClientId("123");
    client.SetActivity(Song("First"));
    CHECK(WaitUntil([&] { return client.GetStatus().state == ConnectionState::connected; }, 3s));
    CHECK(WaitUntil([&] { return LastDetails(*server) == "First"; }, 3s));

    std::printf("== 相同內容不重送\n");
    const auto before = server->Activities().size();
    client.SetActivity(Song("First"));
    std::this_thread::sleep_for(1s);
    CHECK(server->Activities().size() == before);

    std::printf("== Discord 斷線後自動重連並補送目前狀態\n");
    server->ClearActivities();
    server->DropClient();
    CHECK(WaitUntil([&] { return server->Connections() >= 2; }, 6s));
    CHECK(WaitUntil([&] { return LastDetails(*server) == "First"; }, 3s));

    std::printf("== 限流：20 秒內最多 5 次，且最後一定送出最新狀態\n");
    std::this_thread::sleep_for(21s); // 讓先前的送出次數離開限流視窗
    server->ClearActivities();
    for (int i = 1; i <= 12; ++i) {
        client.SetActivity(Song("Rapid " + std::to_string(i)));
        std::this_thread::sleep_for(100ms);
    }
    std::this_thread::sleep_for(3s);
    const auto burst = server->Activities();
    std::printf("  12 次更新中，前 4 秒實際送出 %zu 次\n", burst.size());
    CHECK(burst.size() <= 5);
    CHECK(!burst.empty());
    CHECK(WaitUntil([&] { return LastDetails(*server) == "Rapid 12"; }, 22s));
    {
        const auto all = server->Activities();
        for (size_t i = 5; i < all.size(); ++i) {
            CHECK(all[i].first - all[i - 5].first >= 19s); // 任意連續 6 次之間至少隔約 20 秒
        }
    }

    std::printf("== 清除狀態\n");
    client.SetActivity(std::nullopt);
    CHECK(WaitUntil([&] { const auto l = server->Activities(); return !l.empty() && l.back().second.is_null(); }, 22s));

    std::printf("== 停用時斷線\n");
    client.SetClientId("");
    CHECK(WaitUntil([&] { return client.GetStatus().state == ConnectionState::disabled; }, 3s));
    CHECK(WaitUntil([&] { return !server->Connected(); }, 3s));

    std::printf("== 無效的 client ID\n");
    server->SetMode(FakeDiscord::Mode::reject);
    client.SetClientId("456");
    CHECK(WaitUntil([&] { return client.GetStatus().message == "Invalid Client ID"; }, 5s));
    CHECK(client.GetStatus().state == ConnectionState::error);

    std::printf("== Discord 卡住不讀資料時，關閉仍在時限內完成\n");
    server->SetMode(FakeDiscord::Mode::hang_after_ready);
    client.SetClientId("789");
    CHECK(WaitUntil([&] { return client.GetStatus().state == ConnectionState::connected; }, 10s));
    std::string big(100, 'x');
    for (int i = 0; i < 5; ++i) {
        client.SetActivity(Song(big + std::to_string(i))); // 讓寫入塞住
        std::this_thread::sleep_for(150ms);
    }
    std::this_thread::sleep_for(500ms);
    const auto t0 = Clock::now();
    client.Stop();
    const auto stop_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count();
    std::printf("  Stop() 花了 %lld ms\n", static_cast<long long>(stop_ms));
    CHECK(stop_ms < 2000);
    server->SetMode(FakeDiscord::Mode::normal);
    server.reset();

    if (g_failed == 0) {
        std::printf("all tests passed\n");
    }
    return g_failed == 0 ? 0 : 1;
}
