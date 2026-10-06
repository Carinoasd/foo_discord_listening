// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

// 上傳程式輸出的解析：純邏輯，可在 Linux 上單元測試。

#include "stdafx.h"

#include <atomic>

#include "art/uploader.h"

namespace fdl::art {

std::string ExtractUploadedUrl(std::string_view output) {
    constexpr size_t kMaxUrl = 256;
    size_t pos = 0;
    while ((pos = output.find("https://", pos)) != std::string_view::npos) {
        size_t end = pos;
        while (end < output.size() && static_cast<unsigned char>(output[end]) > ' ' && output[end] != '"' && output[end] != '\'' && output[end] != '<') {
            ++end;
        }
        const auto url = output.substr(pos, end - pos);
        if (url.size() > 8 && url.size() <= kMaxUrl) {
            return std::string(url);
        }
        pos = end;
    }
    return {};
}

} // namespace fdl::art
