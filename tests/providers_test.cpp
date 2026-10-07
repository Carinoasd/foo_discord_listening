// foo_discord_listening — iTunes / Last.fm / MusicBrainz 後援邏輯的單元測試
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "art/providers.h"

#include <cstdio>
#include <cstdlib>

using namespace fdl::art;
using nlohmann::json;

static int g_failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failed;                                                 \
        }                                                               \
    } while (0)

int main() {
    // ---- iTunes ----
    const auto url = BuildITunesSearchUrl("Aimer", "Sleepless Nights", "JP");
    CHECK(url == "https://itunes.apple.com/search?entity=album&limit=10&term=Aimer%20Sleepless%20Nights&country=JP");

    const json itunes = json::parse(R"({"resultCount":3,"results":[
        {"artistName":"エバン・コール","collectionName":"TVアニメ『葬送のフリーレン』Original Soundtrack",
         "artworkUrl100":"https://is1-ssl.mzstatic.com/image/thumb/Music/x/100x100bb.jpg"},
        {"artistName":"vα-liv & 灯里愛夏","collectionName":"WANNABE EP COLLECTION 3 - Single",
         "artworkUrl100":"https://is1-ssl.mzstatic.com/image/thumb/Music/a/100x100bb.jpg"},
        {"artistName":"HoneyWorks","collectionName":"ねぇ、好きって痛いよ。～告白実行委員会キャラクターソング集～",
         "artworkUrl100":"https://is1-ssl.mzstatic.com/image/thumb/Music/h/100x100bb.jpg"}
    ]})");
    // 「- Single」後綴、合作曲的歌手名稱
    CHECK(PickITunesArtwork(itunes, "vα-liv", "WANNABE EP COLLECTION 3") == "https://is1-ssl.mzstatic.com/image/thumb/Music/a/600x600bb.jpg");
    // 〜 與 ～ 視為相同
    CHECK(PickITunesArtwork(itunes, "HoneyWorks", "ねぇ、好きって痛いよ。〜告白実行委員会キャラクターソング集〜").has_value());
    // 專輯名稱相同但歌手不同時不能採用
    CHECK(!PickITunesArtwork(itunes, "Someone Else", "WANNABE EP COLLECTION 3"));
    CHECK(!PickITunesArtwork(itunes, "vα-liv", "WANNABE EP COLLECTION"));
    CHECK(!PickITunesArtwork(json::parse(R"({"results":[{"collectionName":"A","artistName":"B","artworkUrl100":"http://insecure/100x100bb.jpg"}]})"), "B", "A"));
    CHECK(!PickITunesArtwork(json::parse(R"({"errorMessage":"x"})"), "A", "B"));
    CHECK(!PickITunesArtwork(json::parse(R"({"results":[{"collectionName":5,"artistName":null}]})"), "A", "B"));

    // ---- Last.fm ----
    CHECK(BuildLastFmAlbumUrl("A B", "C", "key").find("artist=A%20B&album=C&api_key=key") != std::string::npos);
    const json lastfm = json::parse(R"({"album":{"image":[
        {"#text":"https://lastfm.freetls.fastly.net/i/u/34s/a.png","size":"small"},
        {"#text":"https://lastfm.freetls.fastly.net/i/u/300x300/a.png","size":"extralarge"},
        {"#text":"","size":"mega"}]}})");
    CHECK(PickLastFmImage(lastfm) == "https://lastfm.freetls.fastly.net/i/u/300x300/a.png");
    // Last.fm 對沒有封面的專輯回傳的星星預設圖要排除
    CHECK(!PickLastFmImage(json::parse(R"({"album":{"image":[{"#text":"https://lastfm.freetls.fastly.net/i/u/300x300/2a96cbd8b46e442fc41c2b86b821562f.png","size":"extralarge"}]}})")));
    CHECK(!PickLastFmImage(json::parse(R"({"error":6,"message":"Album not found"})")));
    // key 錯誤、限流是服務錯誤（稍後重試），找不到專輯不是
    CHECK(IsLastFmServiceError(json::parse(R"({"error":10,"message":"Invalid API key"})")));
    CHECK(IsLastFmServiceError(json::parse(R"({"error":29,"message":"Rate limit exceeded"})")));
    CHECK(!IsLastFmServiceError(json::parse(R"({"error":6,"message":"Album not found"})")));
    CHECK(!IsLastFmServiceError(lastfm));

    // ---- MusicBrainz release 後援 ----
    CHECK(BuildReleaseBrowseUrl("", "9162580E-5DF4-32DE-80CC-F45A8D8A9B1D")
          == "https://musicbrainz.org/ws/2/release?fmt=json&limit=50&release-group=9162580e-5df4-32de-80cc-f45a8d8a9b1d");
    const json browse = json::parse(R"({"releases":[
        {"id":"11111111-1111-1111-1111-111111111111","cover-art-archive":{"front":false,"count":0}},
        {"id":"22222222-2222-2222-2222-222222222222","cover-art-archive":{"front":true,"count":3}}]})");
    CHECK(PickReleaseWithFront(browse) == "22222222-2222-2222-2222-222222222222");
    CHECK(!PickReleaseWithFront(json::parse(R"({"releases":[{"id":"x","cover-art-archive":{"front":true}}]})")));
    CHECK(!PickReleaseWithFront(json::parse(R"({"releases":[{"id":"22222222-2222-2222-2222-222222222222","cover-art-archive":{"front":"yes"}}]})")));
    CHECK(PickReleaseGroupOfRelease(json::parse(R"({"release-group":{"id":"9162580E-5DF4-32DE-80CC-F45A8D8A9B1D"}})"))
          == "9162580e-5df4-32de-80cc-f45a8d8a9b1d");
    CHECK(!PickReleaseGroupOfRelease(json::parse(R"({"title":"x"})")));

    // ---- 比對用正規化 ----
    CHECK(NormalizeAlbumForMatch("ALIVE - EP") == "alive");
    CHECK(NormalizeAlbumForMatch("ALIVE") == "alive");
    CHECK(NormalizeAlbumForMatch(" - Single") == "- single");

    if (g_failed == 0) {
        std::printf("all tests passed\n");
    }
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
