// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "lifecycle.h"

#include "art/service.h"
#include "config.h"
#include "discord/client.h"
#include "guids.h"
#include "log.h"
#include "import_rich.h"
#include "presence.h"
#include "update_check.h"

#include <fstream>
#include <sstream>

namespace fdl {

void ApplySettings() {
    discord::Client::Get().SetClientId(config::enabled ? config::EffectiveAppId() : std::string{});
    presence::Refresh();
}

namespace {

cfg_bool g_first_run_done(guids::cfg_first_run_done, false);

/// 第一次啟動時，沿用 foo_discord_rich 留下的設定（只匯入使用者改過的值）。之後不再執行。
void ImportFromDiscordRich(const std::filesystem::path& profile) {
    if (g_first_run_done) {
        return;
    }
    g_first_run_done = true;
    std::ifstream in(profile / L"configuration" / L"foo_discord_rich.dll.cfg", std::ios::binary);
    if (!in) {
        return;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const auto s = import_rich::Parse(buffer.str());
    if (s.Empty()) {
        return;
    }
    if (s.enabled) config::enabled = *s.enabled;
    if (s.line1) config::details_format = s.line1->c_str();
    if (s.line2) config::state_format = s.line2->c_str();
    if (s.line3) config::large_text_format = s.line3->c_str();
    if (s.app_id) config::app_id = s.app_id->c_str();
    if (s.clear_when_paused) config::pause_mode = static_cast<int64_t>(config::PauseMode::clear);
    if (s.use_musicbrainz) config::use_musicbrainz = *s.use_musicbrainz;
    if (s.use_upload) config::use_upload = *s.use_upload;
    if (s.upload_command) config::upload_command = s.upload_command->c_str();
    if (s.album_key) config::upload_key_format = s.album_key->c_str();
    Log("imported your settings from foo_discord_rich");
}

class InitQuit : public initquit {
public:
    void on_init() override {
        Guarded("init", [] {
            InitLog();
            DebugLog("foo_discord_listening {} starting", FDL_VERSION);
            const auto native = filesystem::g_get_native_path(core_api::get_profile_path());
            const auto profile = std::filesystem::path(pfc::stringcvt::string_wide_from_utf8(native.c_str()).get_ptr());
            ImportFromDiscordRich(profile);
            art::Service::Get().Start(profile / L"foo_discord_listening" / L"art_cache.json");
            discord::Client::Get().Start();
            ApplySettings();
            update::Start();
        });
    }
    void on_quit() override {
        Guarded("quit", [] {
            update::Stop();
            art::Service::Get().Stop();
            discord::Client::Get().Stop();
        });
    }
};

FB2K_SERVICE_FACTORY(InitQuit);

} // namespace
} // namespace fdl
