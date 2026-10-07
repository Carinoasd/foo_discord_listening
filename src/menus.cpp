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

/// Playback 選單：「Show on Discord」開關，以及直接開啟本元件設定頁的捷徑。
class MainMenu : public mainmenu_commands {
public:
    enum : t_uint32 { cmd_toggle, cmd_settings, cmd_count };

    t_uint32 get_command_count() override { return cmd_count; }
    GUID get_command(t_uint32 index) override { return index == cmd_toggle ? guids::menu_toggle : guids::menu_settings; }
    void get_name(t_uint32 index, pfc::string_base& out) override {
        out = index == cmd_toggle ? "Show on Discord" : "Discord Listening settings";
    }
    bool get_description(t_uint32 index, pfc::string_base& out) override {
        out = index == cmd_toggle ? "Shows what you are playing as your Discord status." : "Opens the Discord Listening preferences page.";
        return true;
    }
    GUID get_parent() override { return mainmenu_groups::playback; }
    bool get_display(t_uint32 index, pfc::string_base& out, t_uint32& flags) override {
        get_name(index, out);
        flags = index == cmd_toggle && config::enabled ? flag_checked : 0;
        return true;
    }
    void execute(t_uint32 index, service_ptr_t<service_base>) override {
        Guarded("menu", [index] {
            DebugLog("main menu command {}", index);
            if (index == cmd_toggle) {
                config::enabled = !config::enabled;
                ApplySettings();
            } else {
                ui_control::get()->show_preferences(guids::prefs_page);
            }
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
