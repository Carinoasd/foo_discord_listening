// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace fdl::import_rich {

/// 從 foo_discord_rich 的設定檔（configuration\foo_discord_rich.dll.cfg）讀出、值得沿用的設定。
/// 只收錄使用者改過的值：與 foo_discord_rich 預設相同的項目保持 nullopt，以免蓋掉本元件的預設。
struct Settings {
    std::optional<bool> enabled;
    std::optional<std::string> line1;
    std::optional<std::string> line2;
    std::optional<std::string> line3;
    std::optional<std::string> app_id;
    std::optional<bool> clear_when_paused;
    std::optional<bool> use_musicbrainz;
    std::optional<bool> use_upload;
    std::optional<std::string> upload_command;
    std::optional<std::string> album_key;

    bool Empty() const;
};

/// 解析舊版 cfg 檔：一連串 [GUID 16 bytes][uint32 little-endian 長度][資料]。格式錯誤時回傳已讀到的部分。
Settings Parse(std::string_view data);

} // namespace fdl::import_rich
