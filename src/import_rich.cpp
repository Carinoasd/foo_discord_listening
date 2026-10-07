// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

// foo_discord_rich 設定檔的解析：純邏輯，可在 Linux 上單元測試。

#include <stdafx.h> // 角括號：Linux 測試會改用 tests/stub 的版本

#include "import_rich.h"

#include <array>
#include <cstring>

namespace fdl::import_rich {
namespace {

using RawGuid = std::array<uint8_t, 16>;

/// 依 Windows GUID 的記憶體排列（前三段 little-endian）產生 16 bytes。
constexpr RawGuid MakeGuid(uint32_t d1, uint16_t d2, uint16_t d3, std::array<uint8_t, 8> d4) {
    return { static_cast<uint8_t>(d1), static_cast<uint8_t>(d1 >> 8), static_cast<uint8_t>(d1 >> 16), static_cast<uint8_t>(d1 >> 24),
             static_cast<uint8_t>(d2), static_cast<uint8_t>(d2 >> 8), static_cast<uint8_t>(d3), static_cast<uint8_t>(d3 >> 8),
             d4[0], d4[1], d4[2], d4[3], d4[4], d4[5], d4[6], d4[7] };
}

// GUID 與預設值取自 foo_discord_rich 2.0.x 的 component_guids.h / fb2k/config.cpp。
constexpr RawGuid kIsEnabled = MakeGuid(0x13ff3bbd, 0x1797, 0x42f1, { 0x96, 0xf4, 0x9d, 0xaf, 0x57, 0x31, 0x8d, 0xd2 });
constexpr RawGuid kTopText = MakeGuid(0xcdf5f57f, 0x1ad2, 0x4a87, { 0xa4, 0xcd, 0x29, 0x4a, 0x8f, 0x51, 0x15, 0xb0 });
constexpr RawGuid kMiddleText = MakeGuid(0xdad72a56, 0x3388, 0x475c, { 0xa8, 0xc0, 0x16, 0x9d, 0x50, 0x89, 0x3e, 0x11 });
constexpr RawGuid kBottomText = MakeGuid(0xa2fd7075, 0x287b, 0x4b0c, { 0xa7, 0x87, 0x30, 0x48, 0xc0, 0x0e, 0xa9, 0xe0 });
constexpr RawGuid kAppToken = MakeGuid(0x81e899fe, 0x79f2, 0x484a, { 0xa1, 0xbe, 0x25, 0x7c, 0x8d, 0x59, 0x0c, 0x9b });
constexpr RawGuid kDisableWhenPaused = MakeGuid(0x583c1ad6, 0x1865, 0x4fdc, { 0xb9, 0x46, 0x6a, 0x3c, 0x3d, 0xc1, 0xca, 0x79 });
constexpr RawGuid kEnableArtFetch = MakeGuid(0x22cd8898, 0xfe4a, 0x4d1b, { 0x87, 0x75, 0x5a, 0x36, 0x9e, 0x17, 0xa1, 0x74 });
constexpr RawGuid kEnableArtUpload = MakeGuid(0x72a7b301, 0xdffe, 0x4566, { 0x8f, 0x39, 0xaf, 0x34, 0xe0, 0x91, 0x6a, 0x43 });
constexpr RawGuid kArtUploadCmd = MakeGuid(0x09d4c88b, 0x2706, 0x4e98, { 0x86, 0x4a, 0xda, 0xa1, 0x70, 0x95, 0x2f, 0x4c });
constexpr RawGuid kArtUploadPin = MakeGuid(0x717c4690, 0x124d, 0x4596, { 0xb8, 0x8c, 0xda, 0xb6, 0x40, 0x7d, 0x35, 0xfb });

constexpr std::string_view kDefaultTop = "[%title%]";
constexpr std::string_view kDefaultMiddle = "[by %album artist%]";
constexpr std::string_view kDefaultBottom = "[on %album%]";
constexpr std::string_view kDefaultAppToken = "507982587416018945"; // foo_discord_rich 自己的 Discord 應用程式
constexpr std::string_view kDefaultPin = "%artist%|%album%";

/// foo_discord_rich 2.0.2 的 pin query 誤用了上傳指令的 GUID，兩者會寫在同一個 GUID 底下。
/// 像 title formatting（含 % 而沒有空白）的值是 pin query，不能當成要執行的指令。
bool LooksLikePattern(std::string_view value) {
    return value.find('%') != std::string_view::npos && value.find(' ') == std::string_view::npos;
}

void SetIfChanged(std::optional<std::string>& out, std::string_view value, std::string_view upstream_default) {
    if (value != upstream_default) {
        out = std::string(value);
    }
}

} // namespace

bool Settings::Empty() const {
    return !enabled && !line1 && !line2 && !line3 && !app_id && !clear_when_paused && !use_musicbrainz && !use_upload && !upload_command
        && !album_key;
}

Settings Parse(std::string_view data) {
    Settings s;
    size_t pos = 0;
    while (pos + 20 <= data.size()) {
        RawGuid guid;
        std::memcpy(guid.data(), data.data() + pos, 16);
        uint32_t size = 0;
        for (int i = 3; i >= 0; --i) {
            size = (size << 8) | static_cast<uint8_t>(data[pos + 16 + i]);
        }
        pos += 20;
        if (size > data.size() - pos) {
            break; // 檔案被截斷
        }
        const std::string_view value = data.substr(pos, size);
        pos += size;

        const bool flag = !value.empty() && value[0] != 0;
        if (guid == kIsEnabled && !value.empty()) {
            if (!flag) s.enabled = false;
        } else if (guid == kTopText) {
            SetIfChanged(s.line1, value, kDefaultTop);
        } else if (guid == kMiddleText) {
            SetIfChanged(s.line2, value, kDefaultMiddle);
        } else if (guid == kBottomText) {
            SetIfChanged(s.line3, value, kDefaultBottom);
        } else if (guid == kAppToken) {
            if (!value.empty()) SetIfChanged(s.app_id, value, kDefaultAppToken);
        } else if (guid == kDisableWhenPaused && !value.empty()) {
            if (flag) s.clear_when_paused = true;
        } else if (guid == kEnableArtFetch && !value.empty()) {
            if (!flag) s.use_musicbrainz = false;
        } else if (guid == kEnableArtUpload && !value.empty()) {
            if (flag) s.use_upload = true;
        } else if (guid == kArtUploadCmd) {
            if (!value.empty() && !LooksLikePattern(value)) s.upload_command = std::string(value);
        } else if (guid == kArtUploadPin) {
            if (!value.empty()) SetIfChanged(s.album_key, value, kDefaultPin);
        }
    }
    return s;
}

} // namespace fdl::import_rich
