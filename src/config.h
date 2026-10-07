// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

namespace fdl::config {

enum class PauseMode : int64_t {
    clear = 0, ///< 暫停時清除 Discord 狀態
    keep = 1,  ///< 暫停時保留歌曲資訊，但不顯示時間
};

enum class ArtSource : int64_t {
    musicbrainz = 0,            ///< 只查 MusicBrainz / Cover Art Archive
    upload = 1,                 ///< 只上傳本機封面
    musicbrainz_then_upload = 2, ///< 先查 MusicBrainz，找不到再上傳
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
inline constexpr ArtSource default_art_source = ArtSource::musicbrainz;
inline constexpr char default_upload_command[] = "";
// 同一張專輯只上傳一次；沒有專輯名稱的曲目各自上傳。
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
extern cfg_int art_source; // ArtSource
extern cfg_string upload_command;
extern cfg_string upload_key_format;

/// 使用者設定的 app ID，沒設定則回傳內建值。
std::string EffectiveAppId();

// 持久化的 enum 只能新增值，不能重新編號；讀到未知值（例如從新版降級）一律回落到預設。
int64_t ActivityType();
int64_t StatusDisplay();
PauseMode GetPauseMode();
ArtSource GetArtSource();

} // namespace fdl::config
