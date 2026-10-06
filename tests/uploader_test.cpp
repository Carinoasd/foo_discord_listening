// foo_discord_listening — 上傳程式輸出解析的單元測試
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "art/uploader.h"

#include <cstdio>
#include <cstdlib>

using namespace fdl::art;

static int g_failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failed;                                                 \
        }                                                               \
    } while (0)

int main() {
    CHECK(ExtractUploadedUrl("https://files.catbox.moe/abc123.jpg") == "https://files.catbox.moe/abc123.jpg");
    CHECK(ExtractUploadedUrl("https://files.catbox.moe/abc123.jpg\r\n") == "https://files.catbox.moe/abc123.jpg");
    CHECK(ExtractUploadedUrl("Uploading...\nDone: https://i.imgur.com/x.png\n") == "https://i.imgur.com/x.png");
    CHECK(ExtractUploadedUrl(R"({"link":"https://i.imgur.com/x.png"})") == "https://i.imgur.com/x.png");

    // 錯誤訊息不能被當成網址（上游 #79：imgur 的 503 HTML 被當成封面）
    CHECK(ExtractUploadedUrl("").empty());
    CHECK(ExtractUploadedUrl("Error 503 Service Unavailable").empty());
    CHECK(ExtractUploadedUrl("http://insecure.example.com/a.jpg").empty());
    CHECK(ExtractUploadedUrl("https://").empty());
    CHECK(ExtractUploadedUrl("<html>https://</html>").empty());

    // 超過 Discord 上限的網址略過，改取下一個合法的
    const std::string long_url = "https://example.com/" + std::string(300, 'a');
    CHECK(ExtractUploadedUrl(long_url + "\nhttps://ok.example.com/b.jpg") == "https://ok.example.com/b.jpg");

    if (g_failed == 0) {
        std::printf("all tests passed\n");
    }
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
