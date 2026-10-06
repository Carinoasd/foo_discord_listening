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

} // namespace fdl::config
