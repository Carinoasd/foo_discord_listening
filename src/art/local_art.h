// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace fdl::art {

/// 取出曲目的封面（外部圖檔或內嵌），縮成最長邊 max_size 的 JPEG 並寫入唯一的暫存檔。
/// 沒有封面時回傳 nullopt。需在已初始化 COM 的背景執行緒呼叫。
std::optional<std::filesystem::path> ExportCoverArt(const metadb_handle_ptr& track, unsigned max_size, abort_callback& abort);

} // namespace fdl::art
