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

int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::optional<discord::Activity> Build() {
    auto pc = playback_control::get();
    metadb_handle_ptr track;
    if (!pc->is_playing() || !pc->get_now_playing(track)) {
        return std::nullopt;
    }
    const bool paused = pc->is_paused();
    if (paused && config::GetPauseMode() == config::PauseMode::clear) {
        return std::nullopt;
    }

    discord::Activity a;
    a.type = static_cast<discord::ActivityType>(config::ActivityType());
    a.status_display = static_cast<discord::StatusDisplay>(config::StatusDisplay());
    a.details = FormatTitle(track, config::details_format.get());
    a.state = FormatTitle(track, config::state_format.get());
    a.large_text = FormatTitle(track, config::large_text_format.get());
    if (config::art_enabled) {
        if (auto url = ResolveArt(track)) {
            a.large_image = std::move(*url);
        }
    }

    if (paused) {
        a.small_text = "Paused";
    } else if (config::show_time) {
        const double position = pc->playback_get_position();
        const double length = pc->playback_get_length_ex();
        const int64_t start = NowMs() - static_cast<int64_t>(position * 1000.0);
        a.start_ms = start;
        if (length > 0 && pc->playback_can_seek()) {
            a.end_ms = start + static_cast<int64_t>(length * 1000.0);
        }
    }
    return a;
}

} // namespace

std::vector<std::string> ArtKeysFor(const metadb_handle_ptr& track) {
    const auto req = BuildArtRequest(track, false);
    std::vector<std::string> keys;
    for (const auto& key : { req.musicbrainz_key, req.upload_key }) {
        if (!key.empty()) {
            keys.push_back(key);
        }
    }
    return keys;
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
