// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

namespace fdl {

/// 設定變更後呼叫：更新連線用的 app ID 並重送目前狀態。只能在主執行緒呼叫。
void ApplySettings();

} // namespace fdl
