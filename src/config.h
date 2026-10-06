// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

namespace fdl::config {

inline constexpr bool default_enabled = true;
inline constexpr char default_app_id[] = "";

extern cfg_bool enabled;
extern cfg_string app_id;

} // namespace fdl::config
