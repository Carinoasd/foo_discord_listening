// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

DECLARE_COMPONENT_VERSION(
    "Discord Listening",
    FDL_VERSION,
    "Shows what you are playing in foobar2000 as a Discord \"Listening to\" activity.\n"
    "https://github.com/Carinoasd/foo_discord_listening\n"
    "\n"
    "(C) 2026 Carinoasd. Released under the MIT License.\n"
    "Inspired by foo_discord_rich by TheQwertiest; no code from it is used.\n"
    "\n"
    "Third-party software:\n"
    "- foobar2000 SDK, pfc, libPPUI (C) Peter Pawlowski\n"
    "- Windows Template Library (C) Microsoft Corporation, WTL Team; MS-PL\n"
    "- JSON for Modern C++ (C) 2013-2025 Niels Lohmann; MIT License\n"
    "Full license texts: https://github.com/Carinoasd/foo_discord_listening/blob/main/THIRD_PARTY_NOTICES.md");

VALIDATE_COMPONENT_FILENAME("foo_discord_listening.dll");

FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
