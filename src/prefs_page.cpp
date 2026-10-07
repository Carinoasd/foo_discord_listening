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
#include "strings.h"

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
    StringId label;
    int64_t value;
};

struct ComboBinding {
    int id;
    cfg_int& var;
    int64_t (*read)(); ///< 讀取目前設定（含未知值回落）
    int64_t default_value;
    std::span<const ComboOption> options;
};

/// 一個設定頁的內容：對話框與它的控制項清單。
struct PageSpec {
    std::span<const CheckBinding> checks;
    std::span<const TextBinding> texts;
    std::span<const ComboBinding> combos;
};

// ---- 主頁 ----

constexpr ComboOption kActivityTypes[] = {
    { StringId::type_listening, 2 },
    { StringId::type_playing, 0 },
    { StringId::type_watching, 3 },
};
constexpr ComboOption kStatusDisplays[] = {
    { StringId::status_line1, 2 },
    { StringId::status_line2, 1 },
    { StringId::status_app_name, 0 },
};
constexpr ComboOption kPauseModes[] = {
    { StringId::pause_keep, static_cast<int64_t>(config::PauseMode::keep) },
    { StringId::pause_clear, static_cast<int64_t>(config::PauseMode::clear) },
};
constexpr ComboOption kStopModes[] = {
    { StringId::stop_clear, static_cast<int64_t>(config::StopMode::clear) },
    { StringId::stop_keep, static_cast<int64_t>(config::StopMode::keep) },
};

int64_t ReadPauseMode() {
    return static_cast<int64_t>(config::GetPauseMode());
}

int64_t ReadStopMode() {
    return static_cast<int64_t>(config::GetStopMode());
}

const CheckBinding kMainChecks[] = {
    { IDC_ENABLED, config::enabled, config::default_enabled },
    { IDC_SHOW_TIME, config::show_time, config::default_show_time },
    { IDC_SMALL_ICONS, config::small_icons, config::default_small_icons },
    { IDC_NO_ART_IMAGE, config::no_art_image, config::default_no_art_image },
    { IDC_PAUSED_TEXT, config::paused_text, config::default_paused_text },
};
const TextBinding kMainTexts[] = {
    { IDC_DETAILS_FORMAT, config::details_format, config::default_details_format },
    { IDC_STATE_FORMAT, config::state_format, config::default_state_format },
    { IDC_LARGE_TEXT_FORMAT, config::large_text_format, config::default_large_text_format },
    { IDC_APP_ID, config::app_id, config::default_app_id },
};
const ComboBinding kMainCombos[] = {
    { IDC_ACTIVITY_TYPE, config::activity_type, &config::ActivityType, config::default_activity_type, kActivityTypes },
    { IDC_STATUS_DISPLAY, config::status_display, &config::StatusDisplay, config::default_status_display, kStatusDisplays },
    { IDC_PAUSE_MODE, config::pause_mode, &ReadPauseMode, static_cast<int64_t>(config::default_pause_mode), kPauseModes },
    { IDC_STOP_MODE, config::stop_mode, &ReadStopMode, static_cast<int64_t>(config::default_stop_mode), kStopModes },
};
constexpr PageSpec kMainPage{ kMainChecks, kMainTexts, kMainCombos };

// ---- 封面頁 ----

const CheckBinding kArtChecks[] = {
    { IDC_ART_ENABLED, config::art_enabled, config::default_art_enabled },
    { IDC_USE_MUSICBRAINZ, config::use_musicbrainz, config::default_use_musicbrainz },
    { IDC_USE_ITUNES, config::use_itunes, config::default_use_itunes },
    { IDC_USE_LASTFM, config::use_lastfm, config::default_use_lastfm },
    { IDC_USE_UPLOAD, config::use_upload, config::default_use_upload },
};
const TextBinding kArtTexts[] = {
    { IDC_MUSICBRAINZ_SERVER, config::musicbrainz_server, config::default_musicbrainz_server },
    { IDC_ITUNES_COUNTRY, config::itunes_country, config::default_itunes_country },
    { IDC_LASTFM_KEY, config::lastfm_api_key, config::default_lastfm_api_key },
    { IDC_UPLOAD_COMMAND, config::upload_command, config::default_upload_command },
    { IDC_UPLOAD_KEY, config::upload_key_format, config::default_upload_key_format },
    { IDC_ART_FILTER, config::art_filter_query, config::default_art_filter_query },
};
constexpr PageSpec kArtPage{ kArtChecks, kArtTexts, {} };

// ---- 連結與過濾頁 ----

const TextBinding kLinkTexts[] = {
    { IDC_DETAILS_URL, config::details_url, config::default_details_url },
    { IDC_STATE_URL, config::state_url, config::default_state_url },
    { IDC_LARGE_URL, config::large_url, config::default_large_url },
    { IDC_BUTTON1_LABEL, config::button1_label, config::default_button1_label },
    { IDC_BUTTON1_URL, config::button1_url, config::default_button1_url },
    { IDC_BUTTON2_LABEL, config::button2_label, config::default_button2_label },
    { IDC_BUTTON2_URL, config::button2_url, config::default_button2_url },
    { IDC_FILTER, config::filter_query, config::default_filter_query },
};
constexpr PageSpec kLinksPage{ {}, kLinkTexts, {} };

constexpr UINT_PTR kStatusTimer = 1;

/// 三個設定頁共用的對話框。Idd 決定對話框資源，Spec 決定控制項清單。
template <int Idd, const PageSpec& Spec>
class PrefsDialog : public CDialogImpl<PrefsDialog<Idd, Spec>>, public preferences_page_instance {
public:
    enum { IDD = Idd };

    explicit PrefsDialog(preferences_page_callback::ptr callback) : m_callback(std::move(callback)) {}

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
        if (HasChanged()) {
            state |= preferences_state::changed;
        }
        return state;
    }

    void apply() override {
        for (const auto& b : Spec.checks) {
            b.var = this->IsDlgButtonChecked(b.id) == BST_CHECKED;
        }
        for (const auto& b : Spec.texts) {
            b.var = uGetDlgItemText(*this, b.id);
        }
        for (const auto& b : Spec.combos) {
            b.var = ComboValue(b);
        }
        ApplySettings();
        OnChanged();
    }

    void reset() override {
        for (const auto& b : Spec.checks) {
            this->CheckDlgButton(b.id, b.default_value ? BST_CHECKED : BST_UNCHECKED);
        }
        for (const auto& b : Spec.texts) {
            uSetDlgItemText(*this, b.id, b.default_value);
        }
        for (const auto& b : Spec.combos) {
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
        for (const auto& b : Spec.checks) {
            this->CheckDlgButton(b.id, b.var ? BST_CHECKED : BST_UNCHECKED);
        }
        for (const auto& b : Spec.texts) {
            uSetDlgItemText(*this, b.id, b.var.get());
        }
        for (const auto& b : Spec.combos) {
            CComboBox combo(this->GetDlgItem(b.id));
            for (const auto& option : b.options) {
                combo.AddString(pfc::stringcvt::string_wide_from_utf8(Tr(option.label)));
            }
            SelectCombo(b, b.read());
        }
        m_dark.AddDialogWithControls(*this);
        UpdateEnabledState();
        UpdateStatus();
        this->SetTimer(kStatusTimer, 1000);
        m_initialized = true;
        return FALSE;
    }

    void OnDestroy() { this->KillTimer(kStatusTimer); }

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

    bool Checked(int id) { return this->IsDlgButtonChecked(id) == BST_CHECKED; }
    void Enable(int id, bool on) {
        if (auto w = this->GetDlgItem(id)) {
            w.EnableWindow(on);
        }
    }

    /// 依勾選狀態啟用或停用相關欄位（只處理本頁有的控制項）。
    void UpdateEnabledState() {
        if constexpr (Idd == IDD_PREFS_ART) {
            const bool art = Checked(IDC_ART_ENABLED);
            for (int id : { IDC_USE_MUSICBRAINZ, IDC_USE_ITUNES, IDC_USE_LASTFM, IDC_USE_UPLOAD, IDC_UPLOAD_KEY, IDC_ART_FILTER }) {
                Enable(id, art);
            }
            Enable(IDC_MUSICBRAINZ_SERVER, art && Checked(IDC_USE_MUSICBRAINZ));
            Enable(IDC_ITUNES_COUNTRY, art && Checked(IDC_USE_ITUNES));
            Enable(IDC_LASTFM_KEY, art && Checked(IDC_USE_LASTFM));
            Enable(IDC_UPLOAD_COMMAND, art && Checked(IDC_USE_UPLOAD));
        }
    }

    void UpdateStatus() {
        if constexpr (Idd == IDD_PREFS) {
            const auto status = discord::Client::Get().GetStatus();
            uSetDlgItemText(*this, IDC_STATUS, status.message.c_str());
        }
        if constexpr (Idd == IDD_PREFS_ART) {
            const auto count = art::Service::Get().CachedCount();
            uSetDlgItemText(*this, IDC_CACHE_INFO, Format(StringId::cache_count, count).c_str());
        }
    }

    int64_t ComboValue(const ComboBinding& b) {
        const int sel = CComboBox(this->GetDlgItem(b.id)).GetCurSel();
        return sel >= 0 && static_cast<size_t>(sel) < b.options.size() ? b.options[sel].value : b.default_value;
    }

    void SelectCombo(const ComboBinding& b, int64_t value) {
        int index = 0;
        for (size_t i = 0; i < b.options.size(); ++i) {
            if (b.options[i].value == value) {
                index = static_cast<int>(i);
            }
        }
        CComboBox(this->GetDlgItem(b.id)).SetCurSel(index);
    }

    bool HasChanged() {
        for (const auto& b : Spec.checks) {
            if (Checked(b.id) != b.var.get()) {
                return true;
            }
        }
        for (const auto& b : Spec.texts) {
            if (uGetDlgItemText(*this, b.id) != b.var.get()) {
                return true;
            }
        }
        for (const auto& b : Spec.combos) {
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

using MainDialog = PrefsDialog<IDD_PREFS, kMainPage>;
using ArtDialog = PrefsDialog<IDD_PREFS_ART, kArtPage>;
using LinksDialog = PrefsDialog<IDD_PREFS_LINKS, kLinksPage>;

class MainPage : public preferences_page_impl<MainDialog> {
public:
    const char* get_name() override { return "Discord Listening"; }
    GUID get_guid() override { return guids::prefs_page; }
    GUID get_parent_guid() override { return guid_tools; }
};

class ArtPage : public preferences_page_impl<ArtDialog> {
public:
    const char* get_name() override { return Tr(StringId::page_album_art); }
    GUID get_guid() override { return guids::prefs_page_art; }
    GUID get_parent_guid() override { return guids::prefs_page; }
};

class LinksPage : public preferences_page_impl<LinksDialog> {
public:
    const char* get_name() override { return Tr(StringId::page_links); }
    GUID get_guid() override { return guids::prefs_page_links; }
    GUID get_parent_guid() override { return guids::prefs_page; }
};

preferences_page_factory_t<MainPage> g_main_page;
preferences_page_factory_t<ArtPage> g_art_page;
preferences_page_factory_t<LinksPage> g_links_page;

} // namespace
} // namespace fdl
