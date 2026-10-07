// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace fdl::art {

class HttpClient;

struct TrackInfo {
    std::string artist;            ///< 優先用 album artist
    std::string album;
    std::string release_mbid;      ///< MUSICBRAINZ_ALBUMID
    std::string release_group_mbid; ///< MUSICBRAINZ_RELEASEGROUPID
};

enum class LookupStatus {
    found,
    not_found,      ///< 確定沒有封面（可以快取）
    network_error,  ///< 暫時性錯誤（不該長期快取）
};

struct LookupResult {
    LookupStatus status = LookupStatus::not_found;
    std::string url;
};

/// 查詢 Cover Art Archive 的封面網址。阻塞，只能在背景執行緒呼叫。
/// server 為 MusicBrainz 伺服器（可用鏡像站），空字串表示 https://musicbrainz.org。
LookupResult LookupCoverArt(HttpClient& http, const TrackInfo& track, std::string_view server = {});

// 以下為可單獨測試的純函式（實作於 musicbrainz_query.cpp）。

/// 只轉換 ASCII 字母的小寫，其他位元組（含 UTF-8）原樣保留。
std::string AsciiLower(std::string_view text);

/// 是否為合法的 MBID（UUID 格式），大小寫皆可。
bool IsMbid(std::string_view text);
/// 跳脫 Lucene 查詢語法的特殊字元，用於雙引號內的片語。
std::string EscapeLucene(std::string_view text);
/// 用於比對標題：轉小寫、統一引號與破折號、壓縮空白。
std::string NormalizeTitle(std::string_view text);
std::string BuildReleaseGroupSearchUrl(std::string_view server, std::string_view artist, std::string_view album);
/// 從 release-group 搜尋結果挑出可信的一筆：score 夠高且標題相符。找不到回傳 nullopt。
std::optional<std::string> PickReleaseGroup(const nlohmann::json& search, std::string_view album);
/// Cover Art Archive 的穩定網址（Discord 會自行跟隨轉址，不要存轉址後的 archive.org 節點網址）。
std::string CoverArtUrl(std::string_view entity, std::string_view mbid);

} // namespace fdl::art
