// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "config.h"
#include "guids.h"
#include "lifecycle.h"
#include "resource.h"

namespace fdl {
namespace {

class PrefsDialog : public CDialogImpl<PrefsDialog>, public preferences_page_instance {
public:
    enum { IDD = IDD_PREFS };

    explicit PrefsDialog(preferences_page_callback::ptr callback) : m_callback(std::move(callback)) {}

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
        if (HasChanged()) {
            state |= preferences_state::changed;
        }
        return state;
    }

    void apply() override {
        config::enabled = IsDlgButtonChecked(IDC_ENABLED) == BST_CHECKED;
        config::app_id = GetAppIdText();
        ApplySettings();
        m_callback->on_state_changed();
    }

    void reset() override {
        CheckDlgButton(IDC_ENABLED, config::default_enabled ? BST_CHECKED : BST_UNCHECKED);
        uSetDlgItemText(*this, IDC_APP_ID, config::default_app_id);
        m_callback->on_state_changed();
    }

    BEGIN_MSG_MAP_EX(PrefsDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_ENABLED, BN_CLICKED, OnChange)
        COMMAND_HANDLER_EX(IDC_APP_ID, EN_CHANGE, OnChange)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM) {
        m_dark.AddDialogWithControls(*this);
        CheckDlgButton(IDC_ENABLED, config::enabled ? BST_CHECKED : BST_UNCHECKED);
        uSetDlgItemText(*this, IDC_APP_ID, config::app_id.get());
        uSetDlgItemText(*this, IDC_STATUS, "Not connected");
        return FALSE;
    }

    void OnChange(UINT, int, CWindow) { m_callback->on_state_changed(); }

    pfc::string8 GetAppIdText() { return uGetDlgItemText(*this, IDC_APP_ID); }

    bool HasChanged() {
        return (IsDlgButtonChecked(IDC_ENABLED) == BST_CHECKED) != config::enabled.get()
            || GetAppIdText() != config::app_id.get();
    }

    const preferences_page_callback::ptr m_callback;
    fb2k::CDarkModeHooks m_dark;
};

class PrefsPage : public preferences_page_impl<PrefsDialog> {
public:
    const char* get_name() override { return "Discord Listening"; }
    GUID get_guid() override { return guids::prefs_page; }
    GUID get_parent_guid() override { return guid_tools; }
};

preferences_page_factory_t<PrefsPage> g_prefs_page;

} // namespace
} // namespace fdl
