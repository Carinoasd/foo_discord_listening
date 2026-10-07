// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "presence.h"

#include "art/service.h"
#include "config.h"
#include "discord/client.h"

#include <chrono>

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

/// 組出封面請求。兩個 key 都會計算，是否使用由 ResolveArt 依設定決定。
art::ArtRequest BuildArtRequest(const metadb_handle_ptr& track, bool now_playing) {
    art::ArtRequest req;
    req.track = track;
    req.info.artist = FormatTitle(track, kArtArtist, now_playing);
    req.info.album = FormatTitle(track, kArtAlbum, now_playing);
    req.info.release_mbid = FormatTitle(track, kReleaseMbid, now_playing);
    req.info.release_group_mbid = FormatTitle(track, kReleaseGroupMbid, now_playing);
    req.musicbrainz_key = art::Service::MusicBrainzKey(req.info);
    if (const auto key = FormatTitle(track, config::upload_key_format.get(), now_playing); !key.empty()) {
        req.upload_key = "upload:" + key;
    }
    return req;
}

std::optional<std::string> ResolveArt(const metadb_handle_ptr& track) {
    const auto source = config::GetArtSource();
    auto req = BuildArtRequest(track, true);
    if (source == config::ArtSource::upload) {
        req.musicbrainz_key.clear();
    }
    if (source != config::ArtSource::musicbrainz && !config::upload_command.get().is_empty()) {
        req.upload_command = config::upload_command.get().c_str();
    } else {
        req.upload_key.clear();
    }
    return art::Service::Get().Resolve(std::move(req));
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
    if (config::art_enabled) {
        if (auto url = ResolveArt(track)) {
            a.large_image = std::move(*url);
        }
    }
    if (a.large_image.empty() && config::no_art_image) {
        // 沒有封面時顯示預設圖，而不是 Discord 的應用程式預設圖示。
        a.large_image = IconUrl("no-art");
    }

    if (config::small_icons) {
        switch (state) {
        case PlayState::playing: a.small_image = IconUrl("playing"); a.small_text = "Playing"; break;
        case PlayState::paused: a.small_image = IconUrl("paused"); a.small_text = "Paused"; break;
        case PlayState::stopped: a.small_image = IconUrl("stopped"); a.small_text = "Stopped"; break;
        }
    }
    // 小圖示只出現在個人資料卡上；成員清單只顯示一行文字，所以也可以把狀態寫進第一行。
    if (config::paused_text && state != PlayState::playing) {
        const char* label = state == PlayState::paused ? "Paused" : "Stopped";
        a.details = a.details.empty() ? std::string(label) : a.details + " (" + label + ")";
    }
    return a;
}

/// 停止後保留狀態時使用的最後一首曲目。
metadb_handle_ptr g_last_track;

std::optional<discord::Activity> Build() {
    auto pc = playback_control::get();
    metadb_handle_ptr track;
    if (!pc->is_playing() || !pc->get_now_playing(track)) {
        if (config::GetStopMode() == config::StopMode::keep && g_last_track.is_valid()) {
            return BuildFor(g_last_track, PlayState::stopped, false);
        }
        return std::nullopt;
    }
    g_last_track = track;

    const bool paused = pc->is_paused();
    if (paused && config::GetPauseMode() == config::PauseMode::clear) {
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

std::vector<std::string> ArtKeysFor(const metadb_handle_ptr& track) {
    // 正在播放的曲目要用與 ResolveArt 相同的方式計算（含串流的動態資訊），才清得到實際使用的快取。
    metadb_handle_ptr playing;
    const bool now_playing = playback_control::get()->get_now_playing(playing) && playing == track;
    const auto req = BuildArtRequest(track, now_playing);
    std::vector<std::string> keys;
    for (const auto& key : { req.musicbrainz_key, req.upload_key }) {
        if (!key.empty()) {
            keys.push_back(key);
        }
    }
    return keys;
}

void OnNewTrack() {
    g_stream_title_start_ms.reset();
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
