// foo_discord_listening — activity 序列化的單元測試（可在 Linux 以 g++ 執行，不需 foobar2000）
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "discord/activity.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace fdl::discord;

static int g_failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failed;                                                 \
        }                                                               \
    } while (0)

// 以 Discord 的方式計算長度：UTF-16 code unit。
static size_t Units(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) {
            n += c >= 0xF0 ? 2 : 1;
        }
    }
    return n;
}

int main() {
    const std::string zwsp = "\xE2\x80\x8B";

    // 空白與空字串
    CHECK(FitText("", 128).empty());
    CHECK(FitText("   ", 128).empty());
    CHECK(FitText("  abc  ", 128) == "abc");

    // 太短補到 2 字元
    CHECK(FitText("a", 128) == "a" + zwsp);
    CHECK(FitText("中", 128) == "中" + zwsp);
    CHECK(FitText("ab", 128) == "ab");

    // 依 code point 截斷，不切斷多位元組字元，最後一字換成刪節號
    std::string cjk;
    for (int i = 0; i < 200; ++i) cjk += "歌";
    const auto cut = FitText(cjk, 128);
    CHECK(Units(cut) == 128);
    CHECK(cut.ends_with("\xE2\x80\xA6"));
    CHECK(FitText(std::string(128, 'x'), 128) == std::string(128, 'x'));
    CHECK(Units(FitText(std::string(129, 'x'), 128)) == 128);

    // emoji 在 UTF-16 佔 2 個 unit：64 個 emoji 剛好 128，65 個就要截斷，且不能切開 surrogate pair
    const std::string emoji = "\xF0\x9F\x8E\xB5"; // 🎵
    std::string e64, e65;
    for (int i = 0; i < 64; ++i) e64 += emoji;
    e65 = e64 + emoji;
    CHECK(FitText(e64, 128) == e64);
    const auto e65cut = FitText(e65, 128);
    CHECK(Units(e65cut) <= 128);
    CHECK(e65cut.ends_with("\xE2\x80\xA6"));
    CHECK(e65cut.size() == 63 * 4 + 3); // 63 個 emoji + 刪節號
    CHECK(FitText(emoji, 128) == emoji); // 單一 emoji 已是 2 unit，不需補白

    // ToJson：基本欄位
    Activity a;
    a.details = "Song";
    a.state = "Artist";
    a.start_ms = 1000;
    a.end_ms = 5000;
    auto j = ToJson(a);
    CHECK(j["type"] == 2);
    CHECK(j["status_display_type"] == 2);
    CHECK(j["details"] == "Song");
    CHECK(j["state"] == "Artist");
    CHECK(j["timestamps"]["start"] == 1000);
    CHECK(j["timestamps"]["end"] == 5000);
    CHECK(!j.contains("assets"));
    CHECK(!j.contains("buttons"));

    // 空欄位應省略，不送空字串給 Discord
    Activity empty;
    j = ToJson(empty);
    CHECK(!j.contains("details"));
    CHECK(!j.contains("state"));
    CHECK(!j.contains("timestamps"));

    // 沒有 large_image 時 large_text 不送
    Activity t;
    t.large_text = "Album";
    CHECK(!ToJson(t).contains("assets"));
    t.large_image = "https://example.com/a.jpg";
    j = ToJson(t);
    CHECK(j["assets"]["large_image"] == "https://example.com/a.jpg");
    CHECK(j["assets"]["large_text"] == "Album");

    // 按鈕：最多兩個、URL 必須是 http(s)、label 截到 32
    Activity b;
    b.buttons = { { "One", "https://a" }, { "Bad", "ftp://x" }, { "Two", "https://b" }, { "Three", "https://c" } };
    j = ToJson(b);
    CHECK(j["buttons"].size() == 2);
    CHECK(j["buttons"][1]["label"] == "Two");
    b.buttons = { { std::string(40, 'L'), "https://a" } };
    CHECK(Units(ToJson(b)["buttons"][0]["label"]) == 32);

    // details_url 只在有 details 時送出，且必須是 http(s)
    Activity u;
    u.details_url = "https://x";
    CHECK(!ToJson(u).contains("details_url"));
    u.details = "Song";
    CHECK(ToJson(u)["details_url"] == "https://x");
    u.details_url = "javascript:alert(1)";
    CHECK(!ToJson(u).contains("details_url"));

    // 使用者用 title formatting 組出的網址：空白與中日文要編碼，已編碼的部分不重複編碼
    CHECK(NormalizeUrl("https://www.youtube.com/results?search_query=Aimer 夜行列車")
          == "https://www.youtube.com/results?search_query=Aimer%20%E5%A4%9C%E8%A1%8C%E5%88%97%E8%BB%8A");
    CHECK(NormalizeUrl("  https://a.b/c?q=x%20y  ") == "https://a.b/c?q=x%20y");
    CHECK(NormalizeUrl("https://a.b/100%") == "https://a.b/100%25");
    CHECK(NormalizeUrl("https://a.b/%zz") == "https://a.b/%25zz");
    CHECK(NormalizeUrl("https://a.b/x\"y") == "https://a.b/x%22y");
    CHECK(NormalizeUrl("") == "");
    Activity link;
    link.details = "Song";
    link.details_url = "https://www.youtube.com/results?search_query=A B";
    link.buttons = { { "Search", "https://www.youtube.com/results?search_query=歌" } };
    j = ToJson(link);
    CHECK(j["details_url"] == "https://www.youtube.com/results?search_query=A%20B");
    CHECK(j["buttons"][0]["url"] == "https://www.youtube.com/results?search_query=%E6%AD%8C");

    if (g_failed == 0) {
        std::printf("all tests passed\n");
    }
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
