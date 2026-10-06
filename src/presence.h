// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

namespace fdl::presence {

/// 依目前播放狀態與設定重新產生 activity 並送給 Discord。只能在主執行緒呼叫。
void Refresh();

/// 曲目在封面快取中可能使用的 key（MusicBrainz 與上傳），用於清除單一曲目的快取。
std::vector<std::string> ArtKeysFor(const metadb_handle_ptr& track);

} // namespace fdl::presence
