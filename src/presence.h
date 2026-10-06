// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

namespace fdl::presence {

/// 依目前播放狀態與設定重新產生 activity 並送給 Discord。只能在主執行緒呼叫。
void Refresh();

} // namespace fdl::presence
