// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/musicbrainz.h"

#include "art/http.h"
#include "art/providers.h"

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

LookupResult LookupCoverArt(HttpClient& http, const TrackInfo& track, std::string_view server) {
    bool had_error = false;
    auto note = [&](const LookupResult& r) { had_error |= r.status == LookupStatus::network_error; };

    // GET 並解析 JSON；遇到暫時性錯誤時記下來，讓呼叫端稍後重試。
    auto get_json = [&](const std::string& url) -> std::optional<nlohmann::json> {
        Throttle();
        const auto res = http.Get(url);
        if (res.status == 200) {
            auto doc = nlohmann::json::parse(res.body, nullptr, false);
            if (!doc.is_discarded()) {
                return doc;
            }
        } else if (res.status == 0 || res.status >= 500 || res.status == 429) {
            had_error = true;
        }
        return std::nullopt;
    };

    // release-group 本身沒有封面時，逐一找同一個 release-group 底下有封面的 release（上游 #110）。
    auto try_group = [&](const std::string& group) -> std::optional<LookupResult> {
        auto art = CheckCoverArt(http, "release-group", group);
        if (art.status == LookupStatus::found) {
            return art;
        }
        note(art);
        if (art.status == LookupStatus::not_found) {
            if (const auto browse = get_json(BuildReleaseBrowseUrl(server, group))) {
                if (const auto release = PickReleaseWithFront(*browse)) {
                    auto r = CheckCoverArt(http, "release", *release);
                    if (r.status == LookupStatus::found) {
                        return r;
                    }
                    note(r);
                }
            }
        }
        return std::nullopt;
    };

    std::string group = IsMbid(track.release_group_mbid) ? AsciiLower(track.release_group_mbid) : std::string{};
    if (IsMbid(track.release_mbid)) {
        auto res = CheckCoverArt(http, "release", track.release_mbid);
        if (res.status == LookupStatus::found) {
            return res;
        }
        note(res);
        // 這個 release 沒有封面：改查它所屬的 release-group（同一張專輯的其他版本可能有）。
        if (group.empty()) {
            if (const auto release = get_json(BuildReleaseLookupUrl(server, track.release_mbid))) {
                group = PickReleaseGroupOfRelease(*release).value_or(std::string{});
            }
        }
    }
    if (!group.empty()) {
        if (auto res = try_group(group)) {
            return *res;
        }
    }

    if (!track.artist.empty() && !track.album.empty()) {
        if (const auto doc = get_json(BuildReleaseGroupSearchUrl(server, track.artist, track.album))) {
            if (const auto found = PickReleaseGroup(*doc, track.album); found && *found != group) {
                if (auto res = try_group(*found)) {
                    return *res;
                }
            }
        }
    }

    return { had_error ? LookupStatus::network_error : LookupStatus::not_found, {} };
}

} // namespace fdl::art
