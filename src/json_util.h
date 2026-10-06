// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace fdl::json {

// nlohmann::json::value() 在欄位存在但型別不符時會丟例外。外部來源（Discord、MusicBrainz、快取檔）
// 的資料一律透過這些函式讀取：型別不符就當作沒有這個欄位。

inline std::string GetString(const nlohmann::json& obj, const char* key, std::string fallback = {}) {
    if (obj.is_object()) {
        if (const auto it = obj.find(key); it != obj.end() && it->is_string()) {
            return it->get<std::string>();
        }
    }
    return fallback;
}

inline int64_t GetInt(const nlohmann::json& obj, const char* key, int64_t fallback = 0) {
    if (obj.is_object()) {
        if (const auto it = obj.find(key); it != obj.end() && it->is_number_integer()) {
            return it->get<int64_t>();
        }
    }
    return fallback;
}

inline const nlohmann::json& GetObject(const nlohmann::json& obj, const char* key) {
    static const nlohmann::json empty = nlohmann::json::object();
    if (obj.is_object()) {
        if (const auto it = obj.find(key); it != obj.end() && it->is_object()) {
            return *it;
        }
    }
    return empty;
}

} // namespace fdl::json
