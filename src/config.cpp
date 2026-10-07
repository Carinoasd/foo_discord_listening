// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "config.h"
#include "guids.h"

namespace fdl::config {

cfg_bool enabled(guids::cfg_enabled, default_enabled);
cfg_string app_id(guids::cfg_app_id, default_app_id);
cfg_int activity_type(guids::cfg_activity_type, default_activity_type);
cfg_int status_display(guids::cfg_status_display, default_status_display);
cfg_string details_format(guids::cfg_details_format, default_details_format);
cfg_string state_format(guids::cfg_state_format, default_state_format);
cfg_string large_text_format(guids::cfg_large_text_format, default_large_text_format);
cfg_bool show_time(guids::cfg_show_time, default_show_time);
cfg_int pause_mode(guids::cfg_pause_mode, static_cast<int64_t>(default_pause_mode));
cfg_bool art_enabled(guids::cfg_art_enabled, default_art_enabled);
cfg_bool small_icons(guids::cfg_small_icons, default_small_icons);
cfg_bool no_art_image(guids::cfg_no_art_image, default_no_art_image);
cfg_bool paused_text(guids::cfg_paused_text, default_paused_text);
cfg_int stop_mode(guids::cfg_stop_mode, static_cast<int64_t>(default_stop_mode));
cfg_int idle_clear_minutes(guids::cfg_idle_clear_minutes, default_idle_clear_minutes);
cfg_string details_url(guids::cfg_details_url, default_details_url);
cfg_string state_url(guids::cfg_state_url, default_state_url);
cfg_string large_url(guids::cfg_large_url, default_large_url);
cfg_string button1_label(guids::cfg_button1_label, default_button1_label);
cfg_string button1_url(guids::cfg_button1_url, default_button1_url);
cfg_string button2_label(guids::cfg_button2_label, default_button2_label);
cfg_string button2_url(guids::cfg_button2_url, default_button2_url);
cfg_string filter_query(guids::cfg_filter_query, default_filter_query);
cfg_string art_filter_query(guids::cfg_art_filter_query, default_art_filter_query);
cfg_string hidden_playlists(guids::cfg_hidden_playlists, default_hidden_playlists);
cfg_string only_playlists(guids::cfg_only_playlists, default_only_playlists);
cfg_bool use_musicbrainz(guids::cfg_use_musicbrainz, default_use_musicbrainz);
cfg_bool use_itunes(guids::cfg_use_itunes, default_use_itunes);
cfg_bool use_lastfm(guids::cfg_use_lastfm, default_use_lastfm);
cfg_string lastfm_api_key(guids::cfg_lastfm_api_key, default_lastfm_api_key);
cfg_bool use_upload(guids::cfg_use_upload, default_use_upload);
cfg_string musicbrainz_server(guids::cfg_musicbrainz_server, default_musicbrainz_server);
cfg_string itunes_country(guids::cfg_itunes_country, default_itunes_country);
cfg_string upload_command(guids::cfg_upload_command, default_upload_command);
cfg_string upload_key_format(guids::cfg_upload_key_format, default_upload_key_format);

std::string EffectiveAppId() {
    const auto id = app_id.get();
    return id.is_empty() ? std::string(builtin_app_id) : std::string(id.c_str());
}

int64_t ActivityType() {
    const auto v = activity_type.get();
    return v == 0 || v == 2 || v == 3 ? v : default_activity_type;
}

int64_t StatusDisplay() {
    const auto v = status_display.get();
    return v >= 0 && v <= 2 ? v : default_status_display;
}

PauseMode GetPauseMode() {
    const auto v = static_cast<PauseMode>(pause_mode.get());
    return v == PauseMode::clear || v == PauseMode::keep ? v : default_pause_mode;
}

StopMode GetStopMode() {
    const auto v = static_cast<StopMode>(stop_mode.get());
    return v == StopMode::clear || v == StopMode::keep ? v : default_stop_mode;
}

} // namespace fdl::config
