// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace fdl::update {

/// 解析「v1.2.3」或「1.2.3」。格式不符時回傳 nullopt。
inline std::optional<std::array<int, 3>> ParseVersion(std::string_view text) {
    if (!text.empty() && (text.front() == 'v' || text.front() == 'V')) {
        text.remove_prefix(1);
    }
    std::array<int, 3> v{};
    size_t part = 0;
    bool digit = false;
    for (char c : text) {
        if (c >= '0' && c <= '9') {
            v[part] = v[part] * 10 + (c - '0');
            digit = true;
            if (v[part] > 100000) {
                return std::nullopt;
            }
        } else if (c == '.' && digit && part < 2) {
            ++part;
            digit = false;
        } else {
            return std::nullopt;
        }
    }
    if (part != 2 || !digit) {
        return std::nullopt;
    }
    return v;
}

/// latest 是否比 current 新。任一個無法解析時回傳 false。
inline bool IsNewer(std::string_view latest, std::string_view current) {
    const auto a = ParseVersion(latest);
    const auto b = ParseVersion(current);
    return a && b && *a > *b;
}

/// 啟動背景檢查（每天最多一次，可在 Advanced Preferences 關閉）。只能在主執行緒呼叫。
void Start();
void Stop();

/// 已知比目前新的版本號（例如 "v0.3.0"），沒有時回傳空字串。可從任何執行緒呼叫。
std::string NewerVersion();

} // namespace fdl::update
