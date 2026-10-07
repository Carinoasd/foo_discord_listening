// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include <stdafx.h> // 角括號：測試工具會改用 tests/win_stub 的版本

#include "strings.h"

#include <array>

namespace fdl {
namespace {

struct Entry {
    const char* en;
    const char* zh_tw;
};

// 順序必須與 StringId 相同；下方的 static_assert 會檢查數量。
constexpr Entry kStrings[] = {
    { "Listening to", "正在聽（Listening to）" },
    { "Playing", "正在玩（Playing）" },
    { "Watching", "正在看（Watching）" },
    { "Line 1 (song title)", "第一行（歌名）" },
    { "Line 2 (artist)", "第二行（歌手）" },
    { "Application name", "應用程式名稱" },
    { "Keep showing the song", "繼續顯示歌曲" },
    { "Clear the status", "清除狀態" },
    { "Clear the status", "清除狀態" },
    { "Keep showing the last song", "保留最後一首歌" },
    { "Album art", "專輯封面" },
    { "Links & filters", "連結與過濾" },
    { "{} entries cached", "已快取 {} 筆" },
    { "Show on Discord", "在 Discord 顯示" },
    { "Shows what you are playing as your Discord status.", "在 Discord 狀態顯示正在播放的歌曲。" },
    { "Discord Listening settings", "Discord Listening 設定" },
    { "Opens the Discord Listening preferences page.", "開啟 Discord Listening 的設定頁。" },
    { "Re-fetch Discord album art", "重新抓取 Discord 專輯封面" },
    { "Forgets the cached Discord album art of the selected tracks and looks it up again.", "清除所選曲目的 Discord 封面快取並重新查詢。" },
    { "Set Discord album art...", "指定 Discord 專輯封面..." },
    { "Uses an image URL of your choice as the Discord album art for this album.", "以你指定的圖片網址作為這張專輯在 Discord 上的封面。" },
    { "This track has no album information, so its art can't be set by album.", "這首曲目沒有專輯資訊，無法以專輯為單位指定封面。" },
    { "Disabled", "已停用" },
    { "Connecting...", "連線中..." },
    { "Connected", "已連線" },
    { "Disconnected", "已斷線" },
    { "Discord is not running", "Discord 沒有在執行" },
    { "Playing", "播放中" },
    { "Paused", "已暫停" },
    { "Stopped", "已停止" },
    { "A new version is available: {}", "有新版本：{}" },
};
static_assert(std::size(kStrings) == static_cast<size_t>(StringId::count_), "kStrings 與 StringId 數量不一致");

bool DetectTraditionalChinese() {
    const LANGID lang = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(lang) != LANG_CHINESE) {
        return false;
    }
    const auto sub = SUBLANGID(lang);
    return sub == SUBLANG_CHINESE_TRADITIONAL || sub == SUBLANG_CHINESE_HONGKONG || sub == SUBLANG_CHINESE_MACAU;
}

} // namespace

bool UseTraditionalChinese() {
    static const bool zh = DetectTraditionalChinese();
    return zh;
}

const char* Tr(StringId id) {
    const auto& e = kStrings[static_cast<size_t>(id)];
    return UseTraditionalChinese() ? e.zh_tw : e.en;
}

std::string Format(StringId id, size_t value) {
    std::string s = Tr(id);
    if (const auto pos = s.find("{}"); pos != std::string::npos) {
        s.replace(pos, 2, std::to_string(value));
    }
    return s;
}

} // namespace fdl
