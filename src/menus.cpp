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
#include "resource.h"
#include "i18n.h"

#include "art/service.h"
#include "discord/activity.h"

namespace fdl {
namespace {

/// Playback 選單：「Show on Discord」開關，以及直接開啟本元件設定頁的捷徑。
class MainMenu : public mainmenu_commands {
public:
    enum : t_uint32 { cmd_toggle, cmd_settings, cmd_count };

    t_uint32 get_command_count() override { return cmd_count; }
    GUID get_command(t_uint32 index) override { return index == cmd_toggle ? guids::menu_toggle : guids::menu_settings; }
    void get_name(t_uint32 index, pfc::string_base& out) override {
        out = Tr(index == cmd_toggle ? StringId::menu_show : StringId::menu_settings);
    }
    bool get_description(t_uint32 index, pfc::string_base& out) override {
        out = Tr(index == cmd_toggle ? StringId::menu_show_desc : StringId::menu_settings_desc);
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

/// 手動指定封面的小對話框。
class ManualArtDialog : public CDialogImpl<ManualArtDialog> {
public:
    enum { IDD = IDD_MANUAL_ART };

    ManualArtDialog(std::string album, std::string url) : m_album(std::move(album)), m_url(std::move(url)) {}
    const std::string& Url() const { return m_url; }

    BEGIN_MSG_MAP_EX(ManualArtDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_ID_HANDLER_EX(IDOK, OnOk)
        COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM) {
        uSetWindowText(*this, Tr(StringId::ui_manual_caption));
        uSetDlgItemText(*this, IDC_T_MANUAL_URL_NOTE, Tr(StringId::ui_manual_url_note));
        uSetDlgItemText(*this, IDOK, Tr(StringId::ui_ok));
        uSetDlgItemText(*this, IDCANCEL, Tr(StringId::ui_cancel));
        m_dark.AddDialogWithControls(*this);
        uSetDlgItemText(*this, IDC_MANUAL_ALBUM, m_album.c_str());
        uSetDlgItemText(*this, IDC_MANUAL_URL, m_url.c_str());
        return TRUE;
    }
    void OnOk(UINT, int, CWindow) {
        m_url = discord::NormalizeUrl(uGetDlgItemText(*this, IDC_MANUAL_URL).c_str());
        if (!m_url.empty() && !m_url.starts_with("https://")) {
            // Discord 只接受 https 的外部圖片。
            GetDlgItem(IDC_MANUAL_URL).SetFocus();
            MessageBeep(MB_ICONWARNING);
            return;
        }
        EndDialog(IDOK);
    }
    void OnCancel(UINT, int, CWindow) { EndDialog(IDCANCEL); }

    std::string m_album;
    std::string m_url;
    fb2k::CDarkModeHooks m_dark;
};

/// 右鍵選單：重新抓取封面、手動指定封面。
class ContextMenu : public contextmenu_item_simple {
public:
    enum : unsigned { cmd_refetch, cmd_set_art, cmd_count };

    unsigned get_num_items() override { return cmd_count; }
    GUID get_item_guid(unsigned index) override { return index == cmd_refetch ? guids::context_refetch_art : guids::context_set_art; }
    void get_item_name(unsigned index, pfc::string_base& out) override { out = Tr(index == cmd_refetch ? StringId::ctx_refetch : StringId::ctx_set_art); }
    bool get_item_description(unsigned index, pfc::string_base& out) override {
        out = Tr(index == cmd_refetch ? StringId::ctx_refetch_desc : StringId::ctx_set_art_desc);
        return true;
    }
    GUID get_parent() override { return contextmenu_groups::utilities; }

    void context_command(unsigned index, metadb_handle_list_cref items, const GUID&) override {
        Guarded("context menu", [&] {
            if (index == cmd_refetch) {
                Refetch(items);
            } else if (items.get_count() > 0) {
                SetArt(items[0]);
            }
        });
    }

private:
    static void Refetch(metadb_handle_list_cref items) {
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
    }

    static void SetArt(const metadb_handle_ptr& track) {
        const auto key = presence::ManualArtKeyFor(track);
        if (key.empty()) {
            popup_message::g_show(Tr(StringId::ctx_no_album), "Discord Listening");
            return;
        }
        auto& service = art::Service::Get();
        ManualArtDialog dlg(key.substr(std::string_view("manual:").size()), service.GetManual(key).value_or(std::string{}));
        if (dlg.DoModal(core_api::get_main_window()) != IDOK) {
            return;
        }
        service.SetManual(key, dlg.Url());
        presence::Refresh();
    }
};

FB2K_SERVICE_FACTORY(ContextMenu);

} // namespace
} // namespace fdl
