// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "log.h"

#include "guids.h"

#include <filesystem>
#include <fstream>
#include <mutex>

namespace fdl {
namespace {

advconfig_branch_factory g_branch("Discord Listening", guids::advconfig_branch, advconfig_branch::guid_branch_tools, 0);
advconfig_checkbox_factory g_debug_log("Write debug log (profile\\\\foo_discord_listening\\\\debug.log)", "foo_discord_listening.debug_log",
                                       guids::cfg_debug_log, guids::advconfig_branch, 0, false);

// 紀錄檔超過這個大小就從頭重寫，避免長時間開著除錯紀錄把磁碟塞滿。
constexpr std::uintmax_t kMaxLogSize = 2 * 1024 * 1024;

std::mutex g_file_mutex;
std::filesystem::path g_log_path;

std::filesystem::path LogPath() {
    // 只能在主執行緒呼叫 core_api，所以在第一次使用時（init 階段）就算好路徑。
    if (g_log_path.empty()) {
        const auto profile = filesystem::g_get_native_path(core_api::get_profile_path());
        g_log_path = std::filesystem::path(pfc::stringcvt::string_wide_from_utf8(profile.c_str()).get_ptr()) / L"foo_discord_listening" / L"debug.log";
    }
    return g_log_path;
}

void AppendToFile(const std::string& message) {
    std::scoped_lock lock(g_file_mutex);
    if (g_log_path.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(g_log_path.parent_path(), ec);
    const bool too_big = std::filesystem::file_size(g_log_path, ec) > kMaxLogSize && !ec;
    std::ofstream out(g_log_path, std::ios::binary | (too_big ? std::ios::trunc : std::ios::app));
    // 不用 std::chrono::current_zone()：它依賴系統的時區資料庫，在部分 Windows 上會丟例外。
    SYSTEMTIME t{};
    GetLocalTime(&t);
    out << std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03} ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds)
        << message << "\n";
}

} // namespace

bool DebugLogEnabled() {
    return g_debug_log.get();
}

void WriteLog(const std::string& message, bool debug_only) {
    if (!debug_only) {
        console::print("[foo_discord_listening] ", message.c_str());
    }
    if (DebugLogEnabled()) {
        AppendToFile(message);
    }
}

void InitLog() {
    std::scoped_lock lock(g_file_mutex);
    LogPath();
}

} // namespace fdl
