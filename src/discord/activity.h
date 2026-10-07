// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fdl::discord {

enum class ActivityType : int {
    playing = 0,
    listening = 2,
    watching = 3,
};

/// 成員清單與個人狀態列要顯示哪個欄位：應用程式名稱、state 或 details。
enum class StatusDisplay : int {
    name = 0,
    state = 1,
    details = 2,
};

struct Button {
    std::string label;
    std::string url;
    bool operator==(const Button&) const = default;
};

struct Activity {
    ActivityType type = ActivityType::listening;
    StatusDisplay status_display = StatusDisplay::details;

    std::string details;
    std::string details_url;
    std::string state;
    std::string state_url;

    std::string large_image;
    std::string large_text;
    std::string large_url;
    std::string small_image;
    std::string small_text;

    /// Unix 毫秒。只給 start 顯示「已經過」，同時給 start/end 顯示進度條。
    std::optional<int64_t> start_ms;
    std::optional<int64_t> end_ms;

    std::vector<Button> buttons;

    bool operator==(const Activity&) const = default;
};

/// 轉成 SET_ACTIVITY 的 activity 物件，會依 Discord 的限制截斷或略過不合法的欄位。
nlohmann::json ToJson(const Activity& activity);

/// 把使用者用 title formatting 組出的網址整理成合法網址：前後空白去掉，
/// 空白、非 ASCII 與不安全字元做 percent-encoding，已編碼的 %XX 與保留字元維持原樣。
std::string NormalizeUrl(std::string_view url);

/// 截斷到 max_units 個 UTF-16 code unit（Discord 的計算方式），不切斷字元，截斷時以刪節號結尾；
/// 太短（Discord 要求至少 2）時補零寬空白。空字串回傳空字串，呼叫端應略過該欄位。
std::string FitText(std::string_view text, size_t max_units);

} // namespace fdl::discord
