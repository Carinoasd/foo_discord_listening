// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "discord/activity.h"

namespace fdl::discord {
namespace {

constexpr size_t kMaxText = 128;
constexpr size_t kMinText = 2;
constexpr size_t kMaxUrl = 256;
constexpr size_t kMaxButtonLabel = 32;
constexpr size_t kMaxButtonUrl = 512;
constexpr size_t kMaxButtons = 2;
constexpr std::string_view kZeroWidthSpace = "\xE2\x80\x8B";
constexpr std::string_view kEllipsis = "\xE2\x80\xA6";

bool IsContinuationByte(char c) {
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

std::string PadToMin(std::string text, size_t units) {
    for (; units < kMinText; ++units) {
        text += kZeroWidthSpace;
    }
    return text;
}

bool IsHex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool IsValidUrl(std::string_view url, size_t max_len) {
    return url.size() <= max_len && (url.starts_with("https://") || url.starts_with("http://"));
}

/// 圖片可以是上傳到 Discord 應用程式的 asset key，也可以是外部 https 網址。
bool IsValidImage(std::string_view image) {
    return !image.empty() && image.size() <= kMaxUrl;
}

void PutText(nlohmann::json& obj, const char* key, std::string_view text, size_t max_chars = kMaxText) {
    auto fitted = FitText(text, max_chars);
    if (!fitted.empty()) {
        obj[key] = std::move(fitted);
    }
}

void PutUrl(nlohmann::json& obj, const char* key, std::string_view url) {
    auto normalized = NormalizeUrl(url);
    if (IsValidUrl(normalized, kMaxUrl)) {
        obj[key] = std::move(normalized);
    }
}

} // namespace

std::string NormalizeUrl(std::string_view url) {
    while (!url.empty() && static_cast<unsigned char>(url.front()) <= ' ') {
        url.remove_prefix(1);
    }
    while (!url.empty() && static_cast<unsigned char>(url.back()) <= ' ') {
        url.remove_suffix(1);
    }
    static constexpr char kHex[] = "0123456789ABCDEF";
    static constexpr std::string_view kUnsafe = " \"<>\\^`{|}";
    std::string out;
    out.reserve(url.size());
    for (size_t i = 0; i < url.size(); ++i) {
        const auto c = static_cast<unsigned char>(url[i]);
        const bool escaped = c == '%' && i + 2 < url.size() && IsHex(url[i + 1]) && IsHex(url[i + 2]);
        if (c == '%' && !escaped) {
            out += "%25";
        } else if (c >= 0x80 || c < 0x20 || kUnsafe.find(static_cast<char>(c)) != std::string_view::npos) {
            out += '%';
            out += kHex[c >> 4];
            out += kHex[c & 0xF];
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}

std::string FitText(std::string_view text, size_t max_units) {
    // 去掉前後空白：Discord 會自行 trim，trim 後長度不足會讓整個 activity 被拒。
    while (!text.empty() && static_cast<unsigned char>(text.front()) <= ' ') {
        text.remove_prefix(1);
    }
    while (!text.empty() && static_cast<unsigned char>(text.back()) <= ' ') {
        text.remove_suffix(1);
    }
    if (text.empty()) {
        return {};
    }

    // Discord 以 UTF-16 code unit 計算長度：BMP 以外的字元（例如 emoji）算 2。
    // 走訪每個 code point，記下「放得下刪節號」的最後切點，超過上限就在該處截斷。
    size_t units = 0;
    size_t cut_with_ellipsis = 0;
    for (size_t i = 0; i < text.size();) {
        const auto lead = static_cast<unsigned char>(text[i]);
        size_t len = 1;
        while (i + len < text.size() && IsContinuationByte(text[i + len])) {
            ++len;
        }
        const size_t cp_units = lead >= 0xF0 ? 2 : 1;
        if (units + cp_units <= max_units - 1) {
            cut_with_ellipsis = i + len;
        }
        units += cp_units;
        if (units > max_units) {
            std::string result(text.substr(0, cut_with_ellipsis));
            result += kEllipsis;
            return PadToMin(std::move(result), cut_with_ellipsis == 0 ? 1 : 2);
        }
        i += len;
    }
    return PadToMin(std::string(text), units);
}

nlohmann::json ToJson(const Activity& activity) {
    nlohmann::json j = nlohmann::json::object();
    j["type"] = static_cast<int>(activity.type);
    j["status_display_type"] = static_cast<int>(activity.status_display);

    PutText(j, "details", activity.details);
    PutText(j, "state", activity.state);
    if (j.contains("details")) {
        PutUrl(j, "details_url", activity.details_url);
    }
    if (j.contains("state")) {
        PutUrl(j, "state_url", activity.state_url);
    }

    if (activity.start_ms || activity.end_ms) {
        auto& ts = j["timestamps"];
        if (activity.start_ms) {
            ts["start"] = *activity.start_ms;
        }
        if (activity.end_ms) {
            ts["end"] = *activity.end_ms;
        }
    }

    nlohmann::json assets = nlohmann::json::object();
    if (IsValidImage(activity.large_image)) {
        assets["large_image"] = activity.large_image;
        PutText(assets, "large_text", activity.large_text);
        PutUrl(assets, "large_url", activity.large_url);
    }
    if (IsValidImage(activity.small_image)) {
        assets["small_image"] = activity.small_image;
        PutText(assets, "small_text", activity.small_text);
    }
    if (!assets.empty()) {
        j["assets"] = std::move(assets);
    }

    nlohmann::json buttons = nlohmann::json::array();
    for (const auto& button : activity.buttons) {
        if (buttons.size() == kMaxButtons) {
            break;
        }
        auto label = FitText(button.label, kMaxButtonLabel);
        auto url = NormalizeUrl(button.url);
        if (!label.empty() && IsValidUrl(url, kMaxButtonUrl)) {
            buttons.push_back({ { "label", std::move(label) }, { "url", std::move(url) } });
        }
    }
    if (!buttons.empty()) {
        j["buttons"] = std::move(buttons);
    }

    return j;
}

} // namespace fdl::discord
