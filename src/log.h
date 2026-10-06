// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <format>

namespace fdl {

/// 輸出到 foobar2000 主控台（View > Console），可從任何執行緒呼叫。
template <typename... Args>
void Log(std::format_string<Args...> fmt, Args&&... args) {
    console::print("[foo_discord_listening] ", std::format(fmt, std::forward<Args>(args)...).c_str());
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
