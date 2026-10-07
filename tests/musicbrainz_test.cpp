// foo_discord_listening — MusicBrainz 查詢邏輯的單元測試
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "art/http.h"
#include "art/musicbrainz.h"

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
    // MBID 驗證
    CHECK(IsMbid("9162580e-5df4-32de-80cc-f45a8d8a9b1d"));
    CHECK(IsMbid("9162580E-5DF4-32DE-80CC-F45A8D8A9B1D"));
    CHECK(!IsMbid(""));
    CHECK(!IsMbid("?"));
    CHECK(!IsMbid("9162580e5df432de80ccf45a8d8a9b1d"));
    CHECK(!IsMbid("9162580e-5df4-32de-80cc-f45a8d8a9b1g"));

    // Lucene 跳脫：特殊字元與引號不能破壞查詢
    CHECK(EscapeLucene(R"(AC/DC)") == R"(AC\/DC)");
    CHECK(EscapeLucene(R"(Say "Hi"!)") == R"(Say \"Hi\"\!)");
    CHECK(EscapeLucene("(What's the Story) Morning Glory?") == R"(\(What's the Story\) Morning Glory\?)");

    // 標題正規化
    CHECK(NormalizeTitle("  Abbey   Road ") == "abbey road");
    CHECK(NormalizeTitle("Don\xE2\x80\x99t Stop") == "don't stop");
    CHECK(NormalizeTitle("A \xE2\x80\x93 B") == "a - b");
    CHECK(NormalizeTitle("東京事変") == "東京事変");

    // 搜尋網址必須完整編碼
    const auto url = BuildReleaseGroupSearchUrl("", "The Beatles", "Abbey Road");
    CHECK(url.starts_with("https://musicbrainz.org/ws/2/release-group/?fmt=json&limit=10&query="));
    CHECK(url.find(' ') == std::string::npos);
    CHECK(url.find('"') == std::string::npos);
    CHECK(url.find("releasegroup%3A%22Abbey%20Road%22%20AND%20artist%3A%22The%20Beatles%22") != std::string::npos);
    CHECK(UrlEncode("東") == "%E6%9D%B1");
    // 自訂伺服器（鏡像站），結尾斜線會被去掉
    CHECK(BuildReleaseGroupSearchUrl("https://mb.example.org/", "A", "B").starts_with("https://mb.example.org/ws/2/release-group/?"));

    // 全形與半形、兩種波浪號視為相同
    CHECK(NormalizeTitle("ＡＢＣ　１２３") == "abc 123");
    CHECK(NormalizeTitle("〜告白〜") == NormalizeTitle("～告白～"));

    // 挑選 release-group：只接受高分且標題相符的結果
    const json search = json::parse(R"({"release-groups":[
        {"id":"42ef6001-3113-440c-ae7c-3667c7351e35","title":"SAILORWAVE III","score":100},
        {"id":"9162580E-5DF4-32DE-80CC-F45A8D8A9B1D","title":"Sailorwave","score":95},
        {"id":"99a0636c-e69f-3099-85f1-e6049b24946c","title":"SAILORWAVE","score":80}
    ]})");
    const auto picked = PickReleaseGroup(search, "SAILORWAVE");
    CHECK(picked && *picked == "9162580e-5df4-32de-80cc-f45a8d8a9b1d");
    CHECK(!PickReleaseGroup(search, "Something Else"));
    CHECK(!PickReleaseGroup(json::parse(R"({"release-groups":[{"id":"x","title":"A","score":100}]})"), "A"));
    CHECK(!PickReleaseGroup(json::parse(R"({"error":"bad"})"), "A"));
    CHECK(!PickReleaseGroup(json::parse(R"({"release-groups":[1,"x",null]})"), "A"));
    // 型別錯誤的欄位不能讓程式丟例外（上游當機的候選原因）
    CHECK(!PickReleaseGroup(json::parse(R"({"release-groups":[{"id":5,"title":["A"],"score":"100"}]})"), "A"));

    CHECK(CoverArtUrl("release", "9162580E-5DF4-32DE-80CC-F45A8D8A9B1D")
          == "https://coverartarchive.org/release/9162580e-5df4-32de-80cc-f45a8d8a9b1d/front-500");

    if (g_failed == 0) {
        std::printf("all tests passed\n");
    }
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
