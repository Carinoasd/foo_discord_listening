// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <string_view>

namespace fdl {

/// 介面上出現的所有文字（包含對話框標籤，執行時依語言填入）。
enum class StringId {
    // 下拉選單
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
    client_any,
    // 設定頁名稱與動態文字
    page_album_art,
    page_links,
    cache_count,
    preview_nothing,
    app_id_cue,
    // 選單
    menu_show,
    menu_show_desc,
    menu_settings,
    menu_settings_desc,
    ctx_refetch,
    ctx_refetch_desc,
    ctx_set_art,
    ctx_set_art_desc,
    ctx_no_album,
    // 連線狀態與 Discord 上顯示的文字
    conn_disabled,
    conn_connecting,
    conn_connected,
    conn_disconnected,
    conn_not_running,
    icon_playing,
    icon_paused,
    icon_stopped,
    update_available,
    // 主頁標籤
    ui_enabled,
    ui_activity,
    ui_type,
    ui_show_time,
    ui_status_shows,
    ui_when_paused,
    ui_idle_prefix,
    ui_idle_suffix,
    ui_when_stopped,
    ui_text,
    ui_line1,
    ui_line2,
    ui_line3,
    ui_preview,
    ui_icons,
    ui_small_icons,
    ui_no_art_image,
    ui_paused_text,
    ui_discord,
    ui_app_id,
    ui_status,
    // 封面頁標籤
    ui_art_enabled,
    ui_sources,
    ui_server,
    ui_country,
    ui_api_key,
    ui_use_upload,
    ui_command,
    ui_manual_note,
    ui_matching,
    ui_same_album,
    ui_no_art_for,
    ui_cache,
    ui_clear_cache,
    // 連結與過濾頁標籤
    ui_links_intro,
    ui_links,
    ui_line1_link,
    ui_line2_link,
    ui_art_link,
    ui_buttons,
    ui_button1_text,
    ui_button1_link,
    ui_button2_text,
    ui_button2_link,
    ui_privacy,
    ui_hide_tracks,
    ui_query_note,
    ui_hide_playlists,
    ui_only_playlists,
    ui_playlist_note,
    ui_example,
    // 手動指定封面對話框
    ui_manual_caption,
    ui_manual_url_note,
    ui_ok,
    ui_cancel,
    count_ // 必須在最後
};

enum class Language { en, zh_tw, zh_cn, ja };

/// 依 Windows 介面語言決定（繁中：台灣／香港／澳門，簡中：中國／新加坡，日文），其他為英文。
Language CurrentLanguage();

/// 回傳目前語言的文字（UTF-8）。可從任何執行緒呼叫。
const char* Tr(StringId id);
/// 指定語言的文字（測試與檢查用）。
const char* Tr(StringId id, Language lang);

/// 把文字中的 {} 換成數字或字串。
std::string Format(StringId id, size_t value);
std::string Format(StringId id, std::string_view value);

} // namespace fdl
