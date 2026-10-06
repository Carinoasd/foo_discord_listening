// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/service.h"
#include "config.h"
#include "discord/client.h"
#include "guids.h"
#include "lifecycle.h"
#include "presence.h"
#include "resource.h"

#include <span>

namespace fdl {
namespace {

// 每個控制項對應一個設定值：apply / reset / 是否有變更都由同一份清單處理，不會漏存某個設定。
struct CheckBinding {
    int id;
    cfg_bool& var;
    bool default_value;
};

struct TextBinding {
    int id;
    cfg_string& var;
    const char* default_value;
};

struct ComboOption {
    const char* label;
    int64_t value;
};

struct ComboBinding {
    int id;
    cfg_int& var;
    int64_t (*read)(); ///< 讀取目前設定（含未知值回落）
    int64_t default_value;
    std::span<const ComboOption> options;
};

constexpr ComboOption kActivityTypes[] = {
    { "Listening to", 2 },
    { "Playing", 0 },
    { "Watching", 3 },
};
constexpr ComboOption kStatusDisplays[] = {
    { "Line 1 (song title)", 2 },
    { "Line 2 (artist)", 1 },
    { "Application name", 0 },
};
constexpr ComboOption kPauseModes[] = {
    { "Keep showing the song", static_cast<int64_t>(config::PauseMode::keep) },
    { "Clear the status", static_cast<int64_t>(config::PauseMode::clear) },
};
constexpr ComboOption kArtSources[] = {
    { "MusicBrainz", static_cast<int64_t>(config::ArtSource::musicbrainz) },
    { "MusicBrainz, then upload local art", static_cast<int64_t>(config::ArtSource::musicbrainz_then_upload) },
    { "Upload local art", static_cast<int64_t>(config::ArtSource::upload) },
};

const CheckBinding kChecks[] = {
    { IDC_ENABLED, config::enabled, config::default_enabled },
    { IDC_SHOW_TIME, config::show_time, config::default_show_time },
    { IDC_ART_ENABLED, config::art_enabled, config::default_art_enabled },
};

const TextBinding kTexts[] = {
    { IDC_DETAILS_FORMAT, config::details_format, config::default_details_format },
    { IDC_STATE_FORMAT, config::state_format, config::default_state_format },
    { IDC_LARGE_TEXT_FORMAT, config::large_text_format, config::default_large_text_format },
    { IDC_UPLOAD_COMMAND, config::upload_command, config::default_upload_command },
    { IDC_UPLOAD_KEY, config::upload_key_format, config::default_upload_key_format },
    { IDC_APP_ID, config::app_id, config::default_app_id },
};

int64_t ReadPauseMode() {
    return static_cast<int64_t>(config::GetPauseMode());
}

int64_t ReadArtSource() {
    return static_cast<int64_t>(config::GetArtSource());
}

const ComboBinding kCombos[] = {
    { IDC_ACTIVITY_TYPE, config::activity_type, &config::ActivityType, config::default_activity_type, kActivityTypes },
    { IDC_STATUS_DISPLAY, config::status_display, &config::StatusDisplay, config::default_status_display, kStatusDisplays },
    { IDC_PAUSE_MODE, config::pause_mode, &ReadPauseMode, static_cast<int64_t>(config::default_pause_mode), kPauseModes },
    { IDC_ART_SOURCE, config::art_source, &ReadArtSource, static_cast<int64_t>(config::default_art_source), kArtSources },
};
const ComboBinding& kArtSourceCombo = kCombos[3];

constexpr UINT_PTR kStatusTimer = 1;

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
        for (const auto& b : kChecks) {
            b.var = IsDlgButtonChecked(b.id) == BST_CHECKED;
        }
        for (const auto& b : kTexts) {
            b.var = uGetDlgItemText(*this, b.id);
        }
        for (const auto& b : kCombos) {
            b.var = ComboValue(b);
        }
        ApplySettings();
        OnChanged();
    }

    void reset() override {
        for (const auto& b : kChecks) {
            CheckDlgButton(b.id, b.default_value ? BST_CHECKED : BST_UNCHECKED);
        }
        for (const auto& b : kTexts) {
            uSetDlgItemText(*this, b.id, b.default_value);
        }
        for (const auto& b : kCombos) {
            SelectCombo(b, b.default_value);
        }
        UpdateEnabledState();
        OnChanged();
    }

    BEGIN_MSG_MAP_EX(PrefsDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_TIMER(OnTimer)
        COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnButton)
        COMMAND_CODE_HANDLER_EX(EN_CHANGE, OnEdit)
        COMMAND_CODE_HANDLER_EX(CBN_SELCHANGE, OnEdit)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow, LPARAM) {
        for (const auto& b : kChecks) {
            CheckDlgButton(b.id, b.var ? BST_CHECKED : BST_UNCHECKED);
        }
        for (const auto& b : kTexts) {
            uSetDlgItemText(*this, b.id, b.var.get());
        }
        for (const auto& b : kCombos) {
            CComboBox combo(GetDlgItem(b.id));
            for (const auto& option : b.options) {
                combo.AddString(pfc::stringcvt::string_wide_from_utf8(option.label));
            }
            SelectCombo(b, b.read());
        }
        m_dark.AddDialogWithControls(*this);
        UpdateEnabledState();
        UpdateStatus();
        SetTimer(kStatusTimer, 1000);
        m_initialized = true;
        return FALSE;
    }

    void OnDestroy() { KillTimer(kStatusTimer); }

    void OnTimer(UINT_PTR id) {
        if (id == kStatusTimer) {
            UpdateStatus();
        }
    }

    void OnButton(UINT, int id, CWindow) {
        if (id == IDC_CLEAR_CACHE) {
            art::Service::Get().ClearAll();
            presence::Refresh();
            UpdateStatus();
            return;
        }
        OnEdit(0, id, {});
    }

    void OnEdit(UINT, int, CWindow) {
        // 初始化時填值也會觸發 EN_CHANGE，那時還不需要通知 foobar2000。
        if (!m_initialized) {
            return;
        }
        UpdateEnabledState();
        OnChanged();
    }

    void UpdateEnabledState() {
        const bool art = IsDlgButtonChecked(IDC_ART_ENABLED) == BST_CHECKED;
        const bool upload = art && ComboValue(kArtSourceCombo) != static_cast<int64_t>(config::ArtSource::musicbrainz);
        GetDlgItem(IDC_ART_SOURCE).EnableWindow(art);
        GetDlgItem(IDC_UPLOAD_COMMAND).EnableWindow(upload);
        GetDlgItem(IDC_UPLOAD_KEY).EnableWindow(upload);
    }

    void UpdateStatus() {
        const auto status = discord::Client::Get().GetStatus();
        if (config::enabled && config::EffectiveAppId().empty()) {
            uSetDlgItemText(*this, IDC_STATUS, "Enter a Discord application ID, then click Apply");
        } else {
            uSetDlgItemText(*this, IDC_STATUS, status.message.c_str());
        }
        const auto count = art::Service::Get().CachedCount();
        pfc::string8 info;
        info << count << (count == 1 ? " entry cached" : " entries cached");
        uSetDlgItemText(*this, IDC_CACHE_INFO, info);
    }

    int64_t ComboValue(const ComboBinding& b) {
        const int sel = CComboBox(GetDlgItem(b.id)).GetCurSel();
        return sel >= 0 && static_cast<size_t>(sel) < b.options.size() ? b.options[sel].value : b.default_value;
    }

    void SelectCombo(const ComboBinding& b, int64_t value) {
        int index = 0;
        for (size_t i = 0; i < b.options.size(); ++i) {
            if (b.options[i].value == value) {
                index = static_cast<int>(i);
            }
        }
        CComboBox(GetDlgItem(b.id)).SetCurSel(index);
    }

    bool HasChanged() {
        for (const auto& b : kChecks) {
            if ((IsDlgButtonChecked(b.id) == BST_CHECKED) != b.var.get()) {
                return true;
            }
        }
        for (const auto& b : kTexts) {
            if (uGetDlgItemText(*this, b.id) != b.var.get()) {
                return true;
            }
        }
        for (const auto& b : kCombos) {
            if (ComboValue(b) != b.read()) {
                return true;
            }
        }
        return false;
    }

    void OnChanged() { m_callback->on_state_changed(); }

    const preferences_page_callback::ptr m_callback;
    fb2k::CDarkModeHooks m_dark;
    bool m_initialized = false;
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
