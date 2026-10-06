// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "lifecycle.h"

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
            discord::Client::Get().Start();
            ApplySettings();
        });
    }
    void on_quit() override {
        Guarded("quit", [] { discord::Client::Get().Stop(); });
    }
};

FB2K_SERVICE_FACTORY(InitQuit);

} // namespace
} // namespace fdl
