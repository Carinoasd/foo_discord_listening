// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "lifecycle.h"

#include "art/service.h"
#include "config.h"
#include "discord/client.h"
#include "log.h"
#include "presence.h"

namespace fdl {

void ApplySettings() {
    discord::Client::Get().SetClientId(config::enabled ? config::EffectiveAppId() : std::string{});
    presence::Refresh();
}

namespace {

class InitQuit : public initquit {
public:
    void on_init() override {
        Guarded("init", [] {
            const auto profile = filesystem::g_get_native_path(core_api::get_profile_path());
            auto cache = std::filesystem::path(pfc::stringcvt::string_wide_from_utf8(profile.c_str()).get_ptr());
            art::Service::Get().Start(cache / L"foo_discord_listening" / L"art_cache.json");
            discord::Client::Get().Start();
            ApplySettings();
        });
    }
    void on_quit() override {
        Guarded("quit", [] {
            art::Service::Get().Stop();
            discord::Client::Get().Stop();
        });
    }
};

FB2K_SERVICE_FACTORY(InitQuit);

} // namespace
} // namespace fdl
