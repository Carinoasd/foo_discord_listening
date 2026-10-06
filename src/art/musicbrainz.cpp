// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/musicbrainz.h"

#include "art/http.h"

#include <cctype>
#include <chrono>
#include <thread>

namespace fdl::art {
namespace {

/// MusicBrainz 要求每秒最多 1 個請求。
void Throttle() {
    static std::chrono::steady_clock::time_point last{};
    const auto next = last + std::chrono::milliseconds(1100);
    const auto now = std::chrono::steady_clock::now();
    if (now < next) {
        std::this_thread::sleep_for(next - now);
    }
    last = std::chrono::steady_clock::now();
}

/// 確認 CAA 上有這張封面：不跟隨轉址，3xx 代表存在、404 代表沒有。
LookupResult CheckCoverArt(HttpClient& http, std::string_view entity, std::string_view mbid) {
    const auto url = CoverArtUrl(entity, mbid);
    const auto res = http.Head(url, false);
    if (res.status == 0 || res.status >= 500 || res.status == 429) {
        return { LookupStatus::network_error, {} };
    }
    if ((res.status >= 300 && res.status < 400) || res.status == 200) {
        return { LookupStatus::found, url };
    }
    return { LookupStatus::not_found, {} };
}

} // namespace

LookupResult LookupCoverArt(HttpClient& http, const TrackInfo& track) {
    bool had_error = false;

    if (IsMbid(track.release_mbid)) {
        auto res = CheckCoverArt(http, "release", track.release_mbid);
        if (res.status == LookupStatus::found) {
            return res;
        }
        had_error |= res.status == LookupStatus::network_error;
    }
    if (IsMbid(track.release_group_mbid)) {
        auto res = CheckCoverArt(http, "release-group", track.release_group_mbid);
        if (res.status == LookupStatus::found) {
            return res;
        }
        had_error |= res.status == LookupStatus::network_error;
    }

    if (!track.artist.empty() && !track.album.empty()) {
        Throttle();
        const auto res = http.Get(BuildReleaseGroupSearchUrl(track.artist, track.album));
        if (res.status == 200) {
            const auto doc = nlohmann::json::parse(res.body, nullptr, false);
            if (!doc.is_discarded()) {
                if (auto group = PickReleaseGroup(doc, track.album)) {
                    auto art = CheckCoverArt(http, "release-group", *group);
                    if (art.status != LookupStatus::not_found) {
                        return art;
                    }
                }
            }
        } else if (res.status == 0 || res.status >= 500 || res.status == 429 || res.status == 503) {
            had_error = true;
        }
    }

    return { had_error ? LookupStatus::network_error : LookupStatus::not_found, {} };
}

} // namespace fdl::art
