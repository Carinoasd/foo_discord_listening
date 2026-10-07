// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <string_view>

namespace fdl::playlist_filter {

/// 以 * 為萬用字元比對（不分 ASCII 大小寫）。
inline bool WildcardMatch(std::string_view pattern, std::string_view text) {
    auto lower = [](char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; };
    size_t p = 0, t = 0, star = std::string_view::npos, mark = 0;
    while (t < text.size()) {
        if (p < pattern.size() && pattern[p] != '*' && lower(pattern[p]) == lower(text[t])) {
            ++p;
            ++t;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            mark = t;
        } else if (star != std::string_view::npos) {
            p = star + 1;
            t = ++mark;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }
    return p == pattern.size();
}

/// 名稱是否符合以分號分隔的任一模式（前後空白忽略，空的模式略過）。
inline bool MatchesAny(std::string_view patterns, std::string_view name) {
    while (!patterns.empty()) {
        const auto pos = patterns.find(';');
        auto item = patterns.substr(0, pos);
        patterns = pos == std::string_view::npos ? std::string_view{} : patterns.substr(pos + 1);
        while (!item.empty() && item.front() == ' ') item.remove_prefix(1);
        while (!item.empty() && item.back() == ' ') item.remove_suffix(1);
        if (!item.empty() && WildcardMatch(item, name)) {
            return true;
        }
    }
    return false;
}

/// 從這個播放清單播放時是否要隱藏狀態。only 非空時，只有符合 only 的清單才顯示。
inline bool IsHidden(std::string_view playlist, std::string_view hide, std::string_view only) {
    if (MatchesAny(hide, playlist)) {
        return true;
    }
    auto trimmed = only;
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == ';')) trimmed.remove_prefix(1);
    return !trimmed.empty() && !MatchesAny(only, playlist);
}

} // namespace fdl::playlist_filter
