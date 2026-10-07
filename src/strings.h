// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <string>

namespace fdl {

/// 程式中出現的介面文字。對話框的文字在 .rc 裡依語言各有一份。
enum class StringId {
    type_listening,
    type_playing,
    type_watching,
    status_line1,
    status_line2,
    status_app_name,
    pause_keep,
    pause_clear,
    stop_clear,
    stop_keep,
    page_album_art,
    page_links,
    cache_count,
    menu_show,
    menu_show_desc,
    menu_settings,
    menu_settings_desc,
    ctx_refetch,
    ctx_refetch_desc,
    ctx_set_art,
    ctx_set_art_desc,
    ctx_no_album,
    conn_disabled,
    conn_connecting,
    conn_connected,
    conn_disconnected,
    conn_not_running,
    icon_playing,
    icon_paused,
    icon_stopped,
    update_available,
    count_ // 必須在最後
};

/// 依 Windows 介面語言回傳文字（繁體中文或英文）。可從任何執行緒呼叫。
const char* Tr(StringId id);

/// 把文字中的 {} 換成數字。
std::string Format(StringId id, size_t value);

/// 介面是否使用繁體中文。
bool UseTraditionalChinese();

} // namespace fdl
