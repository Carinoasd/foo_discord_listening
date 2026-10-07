// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <format>
#include <string>

namespace fdl {

void WriteLog(const std::string& message, bool debug_only);
bool DebugLogEnabled();
/// 在 init 階段（主執行緒）呼叫一次，決定紀錄檔位置。
void InitLog();

/// 輸出到 foobar2000 主控台（View > Console），開啟除錯紀錄時也寫入紀錄檔。可從任何執行緒呼叫。
template <typename... Args>
void Log(std::format_string<Args...> fmt, Args&&... args) {
    WriteLog(std::format(fmt, std::forward<Args>(args)...), false);
}

/// 只在開啟除錯紀錄（Advanced Preferences）時寫入紀錄檔，不輸出到主控台。
template <typename... Args>
void DebugLog(std::format_string<Args...> fmt, Args&&... args) {
    if (DebugLogEnabled()) {
        WriteLog(std::format(fmt, std::forward<Args>(args)...), true);
    }
}

/// 包住所有從 foobar2000 或背景執行緒進入的程式碼：任何例外都只記錄，不讓它把 foobar2000 拖垮。
template <typename F>
void Guarded(const char* where, F&& fn) noexcept {
    try {
        fn();
    } catch (const std::exception& e) {
        try {
            Log("unexpected error in {}: {}", where, e.what());
        } catch (...) {
        }
    } catch (...) {
        try {
            Log("unexpected error in {}", where);
        } catch (...) {
        }
    }
}

} // namespace fdl
