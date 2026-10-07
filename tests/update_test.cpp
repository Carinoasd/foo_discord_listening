// foo_discord_listening — 版本比較的單元測試
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "update_check.h"

#include <cstdio>
#include <cstdlib>

using namespace fdl::update;

static int g_failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failed;                                                 \
        }                                                               \
    } while (0)

int main() {
    CHECK(ParseVersion("v0.1.0") == (std::array<int, 3>{ 0, 1, 0 }));
    CHECK(ParseVersion("1.12.3") == (std::array<int, 3>{ 1, 12, 3 }));
    CHECK(!ParseVersion(""));
    CHECK(!ParseVersion("v1.2"));
    CHECK(!ParseVersion("v1.2.3.4"));
    CHECK(!ParseVersion("v1..3"));
    CHECK(!ParseVersion("v1.2.3-beta"));
    CHECK(!ParseVersion("latest"));

    CHECK(IsNewer("v0.2.0", "0.1.0"));
    CHECK(IsNewer("v0.10.0", "0.9.9")); // 數字比較，不是字串比較
    CHECK(IsNewer("v1.0.0", "0.99.99"));
    CHECK(!IsNewer("v0.1.0", "0.1.0"));
    CHECK(!IsNewer("v0.0.9", "0.1.0"));
    CHECK(!IsNewer("garbage", "0.1.0"));

    if (g_failed == 0) {
        std::printf("all tests passed\n");
    }
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
