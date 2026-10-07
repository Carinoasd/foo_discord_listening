// foo_discord_listening — foo_discord_rich 設定匯入的單元測試
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "import_rich.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace fdl::import_rich;

static int g_failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failed;                                                 \
        }                                                               \
    } while (0)

// 依 cfg 檔格式組出一筆記錄：GUID（Windows 記憶體排列）+ uint32 長度 + 資料
static std::string Record(uint32_t d1, uint16_t d2, uint16_t d3, std::initializer_list<uint8_t> d4, const std::string& value) {
    std::string r;
    for (int i = 0; i < 4; ++i) r += static_cast<char>(d1 >> (8 * i));
    for (int i = 0; i < 2; ++i) r += static_cast<char>(d2 >> (8 * i));
    for (int i = 0; i < 2; ++i) r += static_cast<char>(d3 >> (8 * i));
    for (auto b : d4) r += static_cast<char>(b);
    const auto n = static_cast<uint32_t>(value.size());
    for (int i = 0; i < 4; ++i) r += static_cast<char>(n >> (8 * i));
    return r + value;
}

static std::string Top(const std::string& v) { return Record(0xcdf5f57f, 0x1ad2, 0x4a87, { 0xa4, 0xcd, 0x29, 0x4a, 0x8f, 0x51, 0x15, 0xb0 }, v); }
static std::string Middle(const std::string& v) { return Record(0xdad72a56, 0x3388, 0x475c, { 0xa8, 0xc0, 0x16, 0x9d, 0x50, 0x89, 0x3e, 0x11 }, v); }
static std::string Bottom(const std::string& v) { return Record(0xa2fd7075, 0x287b, 0x4b0c, { 0xa7, 0x87, 0x30, 0x48, 0xc0, 0x0e, 0xa9, 0xe0 }, v); }
static std::string App(const std::string& v) { return Record(0x81e899fe, 0x79f2, 0x484a, { 0xa1, 0xbe, 0x25, 0x7c, 0x8d, 0x59, 0x0c, 0x9b }, v); }
static std::string Cmd(const std::string& v) { return Record(0x09d4c88b, 0x2706, 0x4e98, { 0x86, 0x4a, 0xda, 0xa1, 0x70, 0x95, 0x2f, 0x4c }, v); }
static std::string Pin(const std::string& v) { return Record(0x717c4690, 0x124d, 0x4596, { 0xb8, 0x8c, 0xda, 0xb6, 0x40, 0x7d, 0x35, 0xfb }, v); }
static std::string Bool(uint32_t d1, uint16_t d2, uint16_t d3, std::initializer_list<uint8_t> d4, bool v) { return Record(d1, d2, d3, d4, std::string(1, v ? 1 : 0)); }
static std::string Enabled(bool v) { return Bool(0x13ff3bbd, 0x1797, 0x42f1, { 0x96, 0xf4, 0x9d, 0xaf, 0x57, 0x31, 0x8d, 0xd2 }, v); }
static std::string DisableWhenPaused(bool v) { return Bool(0x583c1ad6, 0x1865, 0x4fdc, { 0xb9, 0x46, 0x6a, 0x3c, 0x3d, 0xc1, 0xca, 0x79 }, v); }
static std::string Upload(bool v) { return Bool(0x72a7b301, 0xdffe, 0x4566, { 0x8f, 0x39, 0xaf, 0x34, 0xe0, 0x91, 0x6a, 0x43 }, v); }

int main() {
    // 全是 foo_discord_rich 的預設值：不匯入任何東西（本元件的預設比較好）
    auto s = Parse(Enabled(true) + Top("[%title%]") + Middle("[by %album artist%]") + Bottom("[on %album%]") + App("507982587416018945")
                   + Pin("%artist%|%album%") + DisableWhenPaused(false) + Upload(false));
    CHECK(s.Empty());

    // 使用者改過的值會被匯入
    s = Parse(Top("%title% ♪") + Middle("[%artist%]") + App("1234567890") + DisableWhenPaused(true) + Upload(true)
              + Cmd("python C:\\tools\\catbox.py") + Pin("%album%") + Enabled(false));
    CHECK(s.line1 == "%title% ♪");
    CHECK(s.line2 == "[%artist%]");
    CHECK(!s.line3);
    CHECK(s.app_id == "1234567890");
    CHECK(s.clear_when_paused == true);
    CHECK(s.use_upload == true);
    CHECK(s.upload_command == "python C:\\tools\\catbox.py");
    CHECK(s.album_key == "%album%");
    CHECK(s.enabled == false);

    // 2.0.2 的 GUID 衝突：同一個 GUID 底下的 pin query 不能被當成指令
    s = Parse(Cmd("upload.exe --catbox") + Cmd("%artist%|%album%"));
    CHECK(s.upload_command == "upload.exe --catbox");
    s = Parse(Cmd("%artist%|%album%"));
    CHECK(!s.upload_command);

    // 未知的 GUID 略過；截斷的檔案不會讀超出範圍
    s = Parse(Record(0x11111111, 0x2222, 0x3333, { 1, 2, 3, 4, 5, 6, 7, 8 }, "whatever") + Top("A"));
    CHECK(s.line1 == "A");
    auto broken = Top("Hello");
    broken.resize(broken.size() - 2);
    CHECK(Parse(broken).Empty());
    CHECK(Parse("").Empty());
    CHECK(Parse(std::string(7, '\0')).Empty());

    if (g_failed == 0) {
        std::printf("all tests passed\n");
    }
    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
