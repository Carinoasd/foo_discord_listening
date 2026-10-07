// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "presence.h"

#include "art/service.h"
#include "config.h"
#include "discord/client.h"
#include "log.h"
#include "i18n.h"

#include <chrono>
#include <map>

namespace fdl::presence {
namespace {

/// now_playing 為 true 時改用 playback_format_title_ex：會一併處理串流的動態資訊（電台目前播放的曲名）
/// 與 %playback_time% 之類的播放欄位。
std::string FormatTitle(const metadb_handle_ptr& track, const char* pattern, bool now_playing = true) {
    titleformat_object::ptr script;
    titleformat_compiler::get()->compile_safe_ex(script, pattern);
    pfc::string8 out;
    if (!now_playing
        || !playback_control::get()->playback_format_title_ex(track, nullptr, out, script, nullptr, playback_control::display_level_all)) {
        track->format_title(nullptr, out, script, nullptr);
    }
    return out.c_str();
}

// 欄位不存在時 %album% 會變成 "?"，所以一律用 [...] 包起來，缺值就得到空字串（上游 #95 錯誤封面的根因之一）。
constexpr char kArtArtist[] = "[%album artist%]";
constexpr char kArtAlbum[] = "[%album%]";
constexpr char kReleaseMbid[] = "$if3($meta(MUSICBRAINZ_ALBUMID),$meta(MUSICBRAINZ ALBUM ID))";
constexpr char kReleaseGroupMbid[] = "$if3($meta(MUSICBRAINZ_RELEASEGROUPID),$meta(MUSICBRAINZ RELEASE GROUP ID))";

std::string AlbumKey(const metadb_handle_ptr& track, bool now_playing) {
    return FormatTitle(track, config::upload_key_format.get(), now_playing);
}

/// 依設定組出封面來源鏈。所有來源的 key 都會算出來；enabled_only 為 false 時（清除快取用）不看開關。
art::ArtRequest BuildArtRequest(const metadb_handle_ptr& track, bool now_playing, bool enabled_only) {
    using art::SourceKind;
    art::ArtRequest req;
    req.track = track;
    req.info.artist = FormatTitle(track, kArtArtist, now_playing);
    req.info.album = FormatTitle(track, kArtAlbum, now_playing);
    req.info.release_mbid = FormatTitle(track, kReleaseMbid, now_playing);
    req.info.release_group_mbid = FormatTitle(track, kReleaseGroupMbid, now_playing);
    req.musicbrainz_server = config::musicbrainz_server.get().c_str();
    req.itunes_country = config::itunes_country.get().c_str();
    req.lastfm_api_key = config::lastfm_api_key.get().c_str();
    req.upload_command = config::upload_command.get().c_str();

    const auto album_key = AlbumKey(track, now_playing);
    auto add = [&](SourceKind kind, bool enabled, std::string key) {
        if ((enabled || !enabled_only) && !key.empty()) {
            req.steps.push_back({ kind, std::move(key) });
        }
    };
    add(SourceKind::manual, true, album_key.empty() ? std::string{} : "manual:" + album_key);
    add(SourceKind::musicbrainz, config::use_musicbrainz, art::Service::MusicBrainzKey(req.info));
    add(SourceKind::itunes, config::use_itunes, art::Service::SearchKey("itunes:" + req.itunes_country + ":", req.info));
    add(SourceKind::lastfm, config::use_lastfm && !req.lastfm_api_key.empty(), art::Service::SearchKey("lastfm:", req.info));
    add(SourceKind::upload, config::use_upload && !req.upload_command.empty(), album_key.empty() ? std::string{} : "upload:" + album_key);
    return req;
}

std::optional<std::string> ResolveArt(const metadb_handle_ptr& track) {
    return art::Service::Get().Resolve(BuildArtRequest(track, true, true));
}

/// 串流目前這首歌開始的時間（電台換歌時更新），非串流時不使用。
std::optional<int64_t> g_stream_title_start_ms;

int64_t RoundToSecond(int64_t ms) {
    return (ms + 500) / 1000 * 1000;
}

int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

/// 曲目是否符合 foobar2000 搜尋語法的條件。條件為空或語法錯誤時回傳 false（不過濾）。
bool Matches(const char* query, const metadb_handle_ptr& track) {
    if (!query || !*query) {
        return false;
    }
    // 每個條件字串只編譯一次；語法錯誤的條件存成空指標，只記錄一次錯誤。
    static std::map<std::string, search_filter::ptr, std::less<>> cache;
    auto it = cache.find(query);
    if (it == cache.end()) {
        if (cache.size() > 16) {
            cache.clear();
        }
        search_filter::ptr filter;
        try {
            filter = search_filter_manager::get()->create(query);
        } catch (const std::exception& e) {
            Log("invalid filter \"{}\": {}", query, e.what());
        }
        it = cache.emplace(query, filter).first;
    }
    if (!it->second.is_valid()) {
        return false;
    }
    bool out = false;
    it->second->test_multi(pfc::list_single_ref_t<metadb_handle_ptr>(track), &out);
    return out;
}

std::string IconUrl(const char* name) {
    return std::string(config::icon_base_url) + name + ".png";
}

enum class PlayState { playing, paused, stopped };

/// 依曲目與播放狀態組出 activity。now_playing 為 false 時（停止後保留狀態）只用曲目本身的標籤。
discord::Activity BuildFor(const metadb_handle_ptr& track, PlayState state, bool now_playing) {
    discord::Activity a;
    a.type = static_cast<discord::ActivityType>(config::ActivityType());
    a.status_display = static_cast<discord::StatusDisplay>(config::StatusDisplay());
    a.details = FormatTitle(track, config::details_format.get(), now_playing);
    a.state = FormatTitle(track, config::state_format.get(), now_playing);
    a.large_text = FormatTitle(track, config::large_text_format.get(), now_playing);
    if (config::art_enabled && !Matches(config::art_filter_query.get(), track)) {
        if (auto url = ResolveArt(track)) {
            a.large_image = std::move(*url);
        }
    }
    if (a.large_image.empty() && config::no_art_image) {
        // 沒有封面時顯示預設圖，而不是 Discord 的應用程式預設圖示。
        a.large_image = IconUrl("no-art");
    }

    a.details_url = FormatTitle(track, config::details_url.get(), now_playing);
    a.state_url = FormatTitle(track, config::state_url.get(), now_playing);
    a.large_url = FormatTitle(track, config::large_url.get(), now_playing);
    const std::pair<cfg_string*, cfg_string*> buttons[] = { { &config::button1_label, &config::button1_url },
                                                            { &config::button2_label, &config::button2_url } };
    for (const auto& [label, url] : buttons) {
        discord::Button button{ FormatTitle(track, label->get(), now_playing), FormatTitle(track, url->get(), now_playing) };
        if (!button.label.empty() && !button.url.empty()) {
            a.buttons.push_back(std::move(button));
        }
    }

    if (config::small_icons) {
        switch (state) {
        case PlayState::playing: a.small_image = IconUrl("playing"); a.small_text = Tr(StringId::icon_playing); break;
        case PlayState::paused: a.small_image = IconUrl("paused"); a.small_text = Tr(StringId::icon_paused); break;
        case PlayState::stopped: a.small_image = IconUrl("stopped"); a.small_text = Tr(StringId::icon_stopped); break;
        }
    }
    // 小圖示只出現在個人資料卡上；成員清單只顯示一行文字，所以也可以把狀態寫進第一行。
    if (config::paused_text && state != PlayState::playing) {
        const char* label = Tr(state == PlayState::paused ? StringId::icon_paused : StringId::icon_stopped);
        a.details = a.details.empty() ? std::string(label) : a.details + " (" + label + ")";
    }
    return a;
}

/// 停止後保留狀態時使用的最後一首曲目。
metadb_handle_ptr g_last_track;

/// 開始暫停（或停止）的時間；播放中為 nullopt。用來在閒置太久後清除狀態。
std::optional<int64_t> g_idle_since_ms;

bool IdleTooLong() {
    const auto minutes = config::idle_clear_minutes.get();
    return minutes > 0 && g_idle_since_ms && NowMs() - *g_idle_since_ms >= minutes * 60'000;
}

/// 閒置計時到期時主動刷新一次，不必等到有其他播放事件才清除。
void ScheduleIdleCheck() {
    const auto minutes = config::idle_clear_minutes.get();
    if (minutes > 0) {
        fb2k::callLater(static_cast<double>(minutes) * 60.0 + 1.0, [] { Guarded("idle check", [] { Refresh(); }); });
    }
}

std::optional<discord::Activity> Build() {
    auto pc = playback_control::get();
    metadb_handle_ptr track;
    if (!pc->is_playing() || !pc->get_now_playing(track)) {
        if (config::GetStopMode() == config::StopMode::keep && g_last_track.is_valid() && !IdleTooLong()
            && !Matches(config::filter_query.get(), g_last_track)) {
            return BuildFor(g_last_track, PlayState::stopped, false);
        }
        return std::nullopt;
    }
    g_last_track = track;
    // 符合過濾條件的曲目（例如不想公開的專輯）完全不顯示（上游 #68）。
    if (Matches(config::filter_query.get(), track)) {
        return std::nullopt;
    }

    const bool paused = pc->is_paused();
    if (paused && (config::GetPauseMode() == config::PauseMode::clear || IdleTooLong())) {
        return std::nullopt;
    }
    auto a = BuildFor(track, paused ? PlayState::paused : PlayState::playing, true);

    if (!paused && config::show_time) {
        const double position = pc->playback_get_position();
        const double length = pc->playback_get_length_ex();
        // 取整到秒：Discord 只顯示到秒，而且這樣重複刷新時算出的 activity 完全相同，
        // Client 會略過不送，不浪費 Discord 的限流額度。
        int64_t start = RoundToSecond(NowMs() - static_cast<int64_t>(position * 1000.0));
        if (length <= 0 && g_stream_title_start_ms) {
            // 串流的播放位置從開台起累計；電台換歌時改從換歌那一刻計時。
            start = *g_stream_title_start_ms;
        }
        a.start_ms = start;
        if (length > 0 && pc->playback_can_seek()) {
            a.end_ms = start + RoundToSecond(static_cast<int64_t>(length * 1000.0));
        }
    }
    return a;
}

} // namespace

namespace {

bool IsNowPlaying(const metadb_handle_ptr& track) {
    metadb_handle_ptr playing;
    return playback_control::get()->get_now_playing(playing) && playing == track;
}

} // namespace

std::vector<std::string> ArtKeysFor(const metadb_handle_ptr& track) {
    // 正在播放的曲目要用與 ResolveArt 相同的方式計算（含串流的動態資訊），才清得到實際使用的快取。
    const auto req = BuildArtRequest(track, IsNowPlaying(track), false);
    std::vector<std::string> keys;
    for (const auto& step : req.steps) {
        if (step.kind != art::SourceKind::manual) { // 手動指定的封面不因「重新抓取」而消失
            keys.push_back(step.key);
        }
    }
    return keys;
}

std::string ManualArtKeyFor(const metadb_handle_ptr& track) {
    const auto key = AlbumKey(track, IsNowPlaying(track));
    return key.empty() ? std::string{} : "manual:" + key;
}

void OnNewTrack() {
    g_stream_title_start_ms.reset();
    g_idle_since_ms.reset();
}

void OnPause(bool paused) {
    if (paused) {
        g_idle_since_ms = NowMs();
        ScheduleIdleCheck();
    } else {
        g_idle_since_ms.reset();
    }
}

void OnStop() {
    g_idle_since_ms = NowMs();
    ScheduleIdleCheck();
}

void OnStreamTitleChanged() {
    g_stream_title_start_ms = RoundToSecond(NowMs());
}

void Refresh() {
    auto& client = discord::Client::Get();
    if (!config::enabled) {
        client.SetActivity(std::nullopt);
        return;
    }
    client.SetActivity(Build());
}

} // namespace fdl::presence
