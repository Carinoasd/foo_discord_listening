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

bool IsContinuationByte(char c) {
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
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
    if (IsValidUrl(url, kMaxUrl)) {
        obj[key] = url;
    }
}

} // namespace

std::string FitText(std::string_view text, size_t max_chars) {
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

    size_t chars = 0;
    size_t cut = text.size();
    for (size_t i = 0; i < text.size(); ++i) {
        if (IsContinuationByte(text[i])) {
            continue;
        }
        if (chars == max_chars) {
            cut = i;
            break;
        }
        ++chars;
    }

    std::string result(text.substr(0, cut));
    if (cut < text.size() && max_chars >= 1) {
        // 截斷時把最後一個字換成刪節號，讓使用者知道內容被截掉了。
        size_t last = result.size();
        do {
            --last;
        } while (last > 0 && IsContinuationByte(result[last]));
        result.resize(last);
        result += "\xE2\x80\xA6";
    }
    for (; chars < kMinText; ++chars) {
        result += kZeroWidthSpace;
    }
    return result;
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
        if (!label.empty() && IsValidUrl(button.url, kMaxButtonUrl)) {
            buttons.push_back({ { "label", std::move(label) }, { "url", button.url } });
        }
    }
    if (!buttons.empty()) {
        j["buttons"] = std::move(buttons);
    }

    return j;
}

} // namespace fdl::discord
