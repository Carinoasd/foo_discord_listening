// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/service.h"
#include "config.h"
#include "guids.h"
#include "lifecycle.h"
#include "log.h"
#include "presence.h"

namespace fdl {
namespace {

/// Playback 選單裡的「Show on Discord」開關。
class MainMenu : public mainmenu_commands {
public:
    t_uint32 get_command_count() override { return 1; }
    GUID get_command(t_uint32) override { return guids::menu_toggle; }
    void get_name(t_uint32, pfc::string_base& out) override { out = "Show on Discord"; }
    bool get_description(t_uint32, pfc::string_base& out) override {
        out = "Shows what you are playing as your Discord status.";
        return true;
    }
    GUID get_parent() override { return mainmenu_groups::playback; }
    bool get_display(t_uint32 index, pfc::string_base& out, t_uint32& flags) override {
        get_name(index, out);
        flags = config::enabled ? flag_checked : 0;
        return true;
    }
    void execute(t_uint32, service_ptr_t<service_base>) override {
        Guarded("menu", [] {
            config::enabled = !config::enabled;
            ApplySettings();
        });
    }
};

FB2K_SERVICE_FACTORY(MainMenu);

/// 右鍵選單：清除所選曲目的封面快取並重新抓取（例如封面抓錯或之後才補上 MusicBrainz 標籤）。
class ContextMenu : public contextmenu_item_simple {
public:
    unsigned get_num_items() override { return 1; }
    GUID get_item_guid(unsigned) override { return guids::context_refetch_art; }
    void get_item_name(unsigned, pfc::string_base& out) override { out = "Re-fetch Discord album art"; }
    bool get_item_description(unsigned, pfc::string_base& out) override {
        out = "Forgets the cached Discord album art of the selected tracks and looks it up again.";
        return true;
    }
    GUID get_parent() override { return contextmenu_groups::utilities; }

    void context_command(unsigned, metadb_handle_list_cref items, const GUID&) override {
        Guarded("context menu", [&] {
            auto& service = art::Service::Get();
            size_t cleared = 0;
            for (size_t i = 0; i < items.get_count(); ++i) {
                for (const auto& key : presence::ArtKeysFor(items[i])) {
                    service.Clear(key);
                    ++cleared;
                }
            }
            Log("cleared {} cached album art entr{}", cleared, cleared == 1 ? "y" : "ies");
            presence::Refresh();
        });
    }
};

FB2K_SERVICE_FACTORY(ContextMenu);

} // namespace
} // namespace fdl
