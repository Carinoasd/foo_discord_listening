// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace fdl::discord {

/// Discord 的發行版本。同時開了好幾個時，可以指定要連哪一個。
enum class ClientVariant : int {
    any = 0,
    stable = 1,
    ptb = 2,
    canary = 3,
};

/// 由握手後 READY 的 config.api_endpoint 判斷版本，例如 "//canary.discord.com/api"。無法判斷時回傳 any。
inline ClientVariant ClassifyEndpoint(std::string_view api_endpoint) {
    if (api_endpoint.find("canary.discord.com") != std::string_view::npos) {
        return ClientVariant::canary;
    }
    if (api_endpoint.find("ptb.discord.com") != std::string_view::npos) {
        return ClientVariant::ptb;
    }
    if (api_endpoint.find("discord.com") != std::string_view::npos) {
        return ClientVariant::stable;
    }
    return ClientVariant::any;
}

inline const char* VariantName(ClientVariant v) {
    switch (v) {
    case ClientVariant::stable: return "Discord";
    case ClientVariant::ptb: return "Discord PTB";
    case ClientVariant::canary: return "Discord Canary";
    case ClientVariant::any: break;
    }
    return "Discord";
}

} // namespace fdl::discord
