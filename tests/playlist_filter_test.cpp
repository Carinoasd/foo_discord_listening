// foo_discord_listening — 播放清單過濾的單元測試
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "playlist_filter.h"

#include <cstdio>
#include <cstdlib>

using namespace fdl::playlist_filter;

static int g_failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failed;                                                 \
        }                                                               \
    } while (0)

int main() {
    CHECK(WildcardMatch("Podcast*", "Podcasts 2026"));
    CHECK(WildcardMatch("podcast*", "PODCAST"));
    CHECK(WildcardMatch("*私人*", "我的私人清單"));
    CHECK(WildcardMatch("a*b*c", "a123b456c"));
    CHECK(!WildcardMatch("a*b*c", "a123b456"));
    CHECK(WildcardMatch("*", ""));
    CHECK(!WildcardMatch("x", ""));
    CHECK(WildcardMatch("Default", "default"));
    CHECK(!WildcardMatch("Default", "Default 2"));

    CHECK(MatchesAny("私人; Podcast* ;", "Podcast 2026"));
    CHECK(MatchesAny(" 私人 ", "私人"));
    CHECK(!MatchesAny("", "anything"));
    CHECK(!MatchesAny(";;", "anything"));

    // 隱藏清單
    CHECK(IsHidden("私人", "私人;工作", ""));
    CHECK(!IsHidden("公開", "私人;工作", ""));
    // 只顯示清單
    CHECK(!IsHidden("公開", "", "公開*"));
    CHECK(IsHidden("其他", "", "公開*"));
    CHECK(!IsHidden("其他", "", " ; "));
    // 兩者同時設定時，隱藏優先
    CHECK(IsHidden("公開-私人", "*私人", "公開*"));

    if (g_failed == 0) {
        std::printf("all tests passed\n");
    }
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
