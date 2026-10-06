// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "config.h"
#include "guids.h"

namespace fdl::config {

cfg_bool enabled(guids::cfg_enabled, default_enabled);
cfg_string app_id(guids::cfg_app_id, default_app_id);

} // namespace fdl::config
