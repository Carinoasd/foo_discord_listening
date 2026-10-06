// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <filesystem>
#include <string>
#include <string_view>

namespace fdl::art {

struct UploadResult {
    std::string url;    ///< 成功時為 https 網址
    std::string error;  ///< 失敗原因
};

/// 執行使用者設定的上傳指令。指令中的 {path} 會替換成加上引號的圖片路徑；
/// 沒有 {path} 時改從 stdin 傳入路徑（與 foo_discord_rich 的上傳腳本相容）。
/// 從 stdout 中取第一個合法的 https 網址。阻塞，只能在背景執行緒呼叫。
UploadResult RunUploader(const std::string& command, const std::filesystem::path& image, const std::atomic<bool>& cancel);

/// 從上傳程式的輸出中找出第一個可用的網址（https、不含空白、長度不超過 Discord 上限）。
std::string ExtractUploadedUrl(std::string_view output);

} // namespace fdl::art
