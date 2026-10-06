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

} // namespace fdl
