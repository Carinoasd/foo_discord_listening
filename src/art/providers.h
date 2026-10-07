// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include "art/musicbrainz.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace fdl::art {

class HttpClient;

// ---- iTunes Search API（免 key；對日本動畫、遊戲音樂收錄齊全）----

LookupResult LookupITunes(HttpClient& http, const TrackInfo& track, std::string_view country);

std::string BuildITunesSearchUrl(std::string_view artist, std::string_view album, std::string_view country);
/// 從搜尋結果挑出專輯名稱相符（忽略「- Single」「- EP」）且歌手相符的一筆，回傳 1000x1000 封面網址。
std::optional<std::string> PickITunesArtwork(const nlohmann::json& search, std::string_view artist, std::string_view album);

// ---- Last.fm（需要使用者自己的 API key）----

LookupResult LookupLastFm(HttpClient& http, const TrackInfo& track, std::string_view api_key);

std::string BuildLastFmAlbumUrl(std::string_view artist, std::string_view album, std::string_view api_key);
/// Last.fm 回傳的是服務層級的錯誤（key 無效、被停權、限流、暫時故障），而不是「找不到這張專輯」。
bool IsLastFmServiceError(const nlohmann::json& doc);
/// 取最大尺寸的封面；Last.fm 的「沒有圖片」預設星星圖會被排除。
std::optional<std::string> PickLastFmImage(const nlohmann::json& info);

// ---- MusicBrainz release 層級的後援（release-group 沒有封面時，逐一找有封面的 release）----

std::string BuildReleaseBrowseUrl(std::string_view server, std::string_view release_group_mbid);
/// 回傳第一個在 Cover Art Archive 有正面封面的 release MBID。
std::optional<std::string> PickReleaseWithFront(const nlohmann::json& browse);
std::string BuildReleaseLookupUrl(std::string_view server, std::string_view release_mbid);
/// 從 release 查詢結果取出所屬 release-group 的 MBID。
std::optional<std::string> PickReleaseGroupOfRelease(const nlohmann::json& release);

/// 比對用：NormalizeTitle 之後再去掉商店常見的「- Single」「- EP」後綴。
std::string NormalizeAlbumForMatch(std::string_view album);

} // namespace fdl::art
