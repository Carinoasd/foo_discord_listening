// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

DECLARE_COMPONENT_VERSION(
    "Discord Listening",
    FDL_VERSION,
    "Shows what you are playing in foobar2000 as a Discord \"Listening to\" activity.\n"
    "\n"
    "(C) 2026 Carinoasd\n"
    "Released under the MIT License.\n"
    "Inspired by foo_discord_rich by TheQwertiest.");

VALIDATE_COMPONENT_FILENAME("foo_discord_listening.dll");

FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
