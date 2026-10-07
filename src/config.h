// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

namespace fdl::config {

enum class PauseMode : int64_t {
    clear = 0, ///< 暫停時清除 Discord 狀態
    keep = 1,  ///< 暫停時保留歌曲資訊，但不顯示時間
};

enum class StopMode : int64_t {
    clear = 0, ///< 停止時清除 Discord 狀態
    keep = 1,  ///< 停止時保留最後一首的資訊，顯示停止圖示
};

inline constexpr bool default_enabled = true;
inline constexpr char default_app_id[] = "";
inline constexpr int64_t default_activity_type = 2; // listening
inline constexpr int64_t default_status_display = 2; // details（歌名）
inline constexpr char default_details_format[] = "[%title%]";
inline constexpr char default_state_format[] = "[%artist%]";
inline constexpr char default_large_text_format[] = "[%album%]";
inline constexpr bool default_show_time = true;
inline constexpr PauseMode default_pause_mode = PauseMode::keep;
inline constexpr bool default_art_enabled = true;
inline constexpr bool default_small_icons = true;
inline constexpr bool default_no_art_image = true;
inline constexpr bool default_paused_text = true;
inline constexpr StopMode default_stop_mode = StopMode::clear;

// 連結與按鈕：title formatting，結果為空就不送。網址中的空白與非 ASCII 字元會自動編碼。
inline constexpr char default_details_url[] = "";
inline constexpr char default_state_url[] = "";
inline constexpr char default_large_url[] = "";
inline constexpr char default_button1_label[] = "";
inline constexpr char default_button1_url[] = "";
inline constexpr char default_button2_label[] = "";
inline constexpr char default_button2_url[] = "";
// 過濾：foobar2000 搜尋語法，空字串表示不過濾。
inline constexpr char default_filter_query[] = "";
inline constexpr char default_art_filter_query[] = "";

/// 圖示放在 GitHub repo，以外部網址交給 Discord（不必上傳到 Developer Portal）。
inline constexpr char icon_base_url[] = "https://raw.githubusercontent.com/Carinoasd/foo_discord_listening/main/assets/";
// 封面來源依序嘗試：手動指定 → MusicBrainz → iTunes → Last.fm → 上傳本機封面。
inline constexpr bool default_use_musicbrainz = true;
inline constexpr bool default_use_itunes = true;
inline constexpr bool default_use_lastfm = false;
inline constexpr char default_lastfm_api_key[] = "";
inline constexpr bool default_use_upload = false;
inline constexpr char default_musicbrainz_server[] = ""; // 空字串 = https://musicbrainz.org
inline constexpr char default_itunes_country[] = "JP";   // 日本商店對動畫、遊戲音樂收錄最齊全
inline constexpr char default_upload_command[] = "";
// 「同一張專輯」的判定：上傳本機封面與手動指定封面都以此為單位。沒有專輯名稱的曲目各自獨立。
inline constexpr char default_upload_key_format[] = "$if([%album%],[%album artist%]|[%album%],%path%)";

/// 內建的 Discord 應用程式 ID；使用者沒填時採用。
inline constexpr char builtin_app_id[] = "1557181309678260225"; // Discord application "foobar2000"

extern cfg_bool enabled;
extern cfg_string app_id;
extern cfg_int activity_type;
extern cfg_int status_display;
extern cfg_string details_format;
extern cfg_string state_format;
extern cfg_string large_text_format;
extern cfg_bool show_time;
extern cfg_int pause_mode; // PauseMode
extern cfg_bool art_enabled;
extern cfg_bool small_icons;
extern cfg_bool no_art_image;
extern cfg_bool paused_text;
extern cfg_int stop_mode; // StopMode
extern cfg_string details_url;
extern cfg_string state_url;
extern cfg_string large_url;
extern cfg_string button1_label;
extern cfg_string button1_url;
extern cfg_string button2_label;
extern cfg_string button2_url;
extern cfg_string filter_query;
extern cfg_string art_filter_query;
extern cfg_bool use_musicbrainz;
extern cfg_bool use_itunes;
extern cfg_bool use_lastfm;
extern cfg_string lastfm_api_key;
extern cfg_bool use_upload;
extern cfg_string musicbrainz_server;
extern cfg_string itunes_country;
extern cfg_string upload_command;
extern cfg_string upload_key_format;

/// 使用者設定的 app ID，沒設定則回傳內建值。
std::string EffectiveAppId();

// 持久化的 enum 只能新增值，不能重新編號；讀到未知值（例如從新版降級）一律回落到預設。
int64_t ActivityType();
int64_t StatusDisplay();
PauseMode GetPauseMode();
StopMode GetStopMode();

} // namespace fdl::config
