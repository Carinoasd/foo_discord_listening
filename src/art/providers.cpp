// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/providers.h"

#include "art/http.h"
#include "json_util.h"
#include "log.h"

#include <chrono>
#include <thread>

namespace fdl::art {
namespace {

/// 對同一個服務的請求間隔。iTunes 約每分鐘 20 次，Last.fm 每秒 5 次。
void Throttle(std::chrono::steady_clock::time_point& last, std::chrono::milliseconds interval) {
    const auto next = last + interval;
    const auto now = std::chrono::steady_clock::now();
    if (now < next) {
        std::this_thread::sleep_for(next - now);
    }
    last = std::chrono::steady_clock::now();
}

bool IsTransient(int status) {
    return status == 0 || status == 429 || status >= 500;
}

} // namespace

LookupResult LookupITunes(HttpClient& http, const TrackInfo& track, std::string_view country) {
    if (track.artist.empty() || track.album.empty()) {
        return { LookupStatus::not_found, {} };
    }
    static std::chrono::steady_clock::time_point last{};
    Throttle(last, std::chrono::milliseconds(3000));
    const auto res = http.Get(BuildITunesSearchUrl(track.artist, track.album, country));
    if (IsTransient(res.status)) {
        return { LookupStatus::network_error, {} };
    }
    if (res.status == 200) {
        const auto doc = nlohmann::json::parse(res.body, nullptr, false);
        if (!doc.is_discarded()) {
            if (auto url = PickITunesArtwork(doc, track.artist, track.album)) {
                return { LookupStatus::found, std::move(*url) };
            }
        }
    }
    return { LookupStatus::not_found, {} };
}

LookupResult LookupLastFm(HttpClient& http, const TrackInfo& track, std::string_view api_key) {
    if (track.artist.empty() || track.album.empty() || api_key.empty()) {
        return { LookupStatus::not_found, {} };
    }
    static std::chrono::steady_clock::time_point last{};
    Throttle(last, std::chrono::milliseconds(250));
    const auto res = http.Get(BuildLastFmAlbumUrl(track.artist, track.album, api_key));
    if (IsTransient(res.status)) {
        return { LookupStatus::network_error, {} };
    }
    const auto doc = nlohmann::json::parse(res.body, nullptr, false);
    if (doc.is_discarded()) {
        return { LookupStatus::network_error, {} };
    }
    if (IsLastFmServiceError(doc)) {
        // API key 錯誤、被停權或超過限流：不是「這張專輯沒有封面」，不能快取成沒有。
        Log("Last.fm: {} (error {})", json::GetString(doc, "message"), json::GetInt(doc, "error"));
        return { LookupStatus::network_error, {} };
    }
    if (auto url = PickLastFmImage(doc)) {
        return { LookupStatus::found, std::move(*url) };
    }
    return { LookupStatus::not_found, {} };
}

} // namespace fdl::art
