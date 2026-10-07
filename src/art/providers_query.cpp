// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

// 各封面來源的純邏輯部分：不碰網路與 Windows API，可在 Linux 上單元測試。

#include "stdafx.h"

#include "art/http.h"
#include "art/providers.h"
#include "json_util.h"

namespace fdl::art {
namespace {

std::string TrimServer(std::string_view server) {
    std::string s(server);
    while (!s.empty() && s.back() == '/') {
        s.pop_back();
    }
    return s.empty() ? std::string("https://musicbrainz.org") : s;
}

bool ArtistMatches(std::string_view wanted, std::string_view candidate) {
    const auto a = NormalizeTitle(wanted);
    const auto b = NormalizeTitle(candidate);
    if (a.empty() || b.empty()) {
        return false;
    }
    // 合作曲常寫成「A & B」，只要一方包含另一方就算相符。
    return a == b || a.find(b) != std::string::npos || b.find(a) != std::string::npos;
}

} // namespace

std::string NormalizeAlbumForMatch(std::string_view album) {
    auto s = NormalizeTitle(album);
    for (const std::string_view suffix : { " - single", " - ep" }) {
        if (s.size() > suffix.size() && s.ends_with(suffix)) {
            s.resize(s.size() - suffix.size());
        }
    }
    return s;
}

std::string BuildITunesSearchUrl(std::string_view artist, std::string_view album, std::string_view country) {
    const std::string term = std::string(artist) + " " + std::string(album);
    std::string url = "https://itunes.apple.com/search?entity=album&limit=10&term=" + UrlEncode(term);
    if (!country.empty()) {
        url += "&country=" + UrlEncode(country);
    }
    return url;
}

std::optional<std::string> PickITunesArtwork(const nlohmann::json& search, std::string_view artist, std::string_view album) {
    const auto results = search.find("results");
    if (results == search.end() || !results->is_array()) {
        return std::nullopt;
    }
    const auto wanted = NormalizeAlbumForMatch(album);
    for (const auto& r : *results) {
        const auto name = json::GetString(r, "collectionName");
        const auto art = json::GetString(r, "artworkUrl100");
        if (name.empty() || !art.starts_with("https://") || NormalizeAlbumForMatch(name) != wanted
            || !ArtistMatches(artist, json::GetString(r, "artistName"))) {
            continue;
        }
        // 縮圖網址的尺寸可以直接改：100x100bb → 600x600bb。
        auto url = art;
        if (const auto pos = url.rfind("100x100bb"); pos != std::string::npos) {
            url.replace(pos, 9, "600x600bb");
        }
        return url;
    }
    return std::nullopt;
}

std::string BuildLastFmAlbumUrl(std::string_view artist, std::string_view album, std::string_view api_key) {
    return "https://ws.audioscrobbler.com/2.0/?method=album.getinfo&format=json&autocorrect=1&artist=" + UrlEncode(artist)
        + "&album=" + UrlEncode(album) + "&api_key=" + UrlEncode(api_key);
}

std::optional<std::string> PickLastFmImage(const nlohmann::json& info) {
    const auto& album = json::GetObject(info, "album");
    const auto images = album.find("image");
    if (images == album.end() || !images->is_array()) {
        return std::nullopt;
    }
    // 由大到小找第一個可用的尺寸。
    for (const std::string_view size : { "mega", "extralarge", "large" }) {
        for (const auto& img : *images) {
            const auto url = json::GetString(img, "#text");
            // Last.fm 對沒有封面的專輯會回傳固定的星星預設圖。
            if (json::GetString(img, "size") == size && url.starts_with("https://")
                && url.find("2a96cbd8b46e442fc41c2b86b821562f") == std::string::npos) {
                return url;
            }
        }
    }
    return std::nullopt;
}

std::string BuildReleaseBrowseUrl(std::string_view server, std::string_view release_group_mbid) {
    return TrimServer(server) + "/ws/2/release?fmt=json&limit=50&release-group=" + AsciiLower(release_group_mbid);
}

std::optional<std::string> PickReleaseWithFront(const nlohmann::json& browse) {
    const auto releases = browse.find("releases");
    if (releases == browse.end() || !releases->is_array()) {
        return std::nullopt;
    }
    for (const auto& r : *releases) {
        const auto id = json::GetString(r, "id");
        const auto& caa = json::GetObject(r, "cover-art-archive");
        const auto front = caa.find("front");
        if (IsMbid(id) && front != caa.end() && front->is_boolean() && front->get<bool>()) {
            return AsciiLower(id);
        }
    }
    return std::nullopt;
}

std::string BuildReleaseLookupUrl(std::string_view server, std::string_view release_mbid) {
    return TrimServer(server) + "/ws/2/release/" + AsciiLower(release_mbid) + "?fmt=json&inc=release-groups";
}

std::optional<std::string> PickReleaseGroupOfRelease(const nlohmann::json& release) {
    const auto id = json::GetString(json::GetObject(release, "release-group"), "id");
    return IsMbid(id) ? std::optional(AsciiLower(id)) : std::nullopt;
}

} // namespace fdl::art
