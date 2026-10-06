// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "presence.h"

#include "config.h"
#include "discord/client.h"

#include <chrono>

namespace fdl::presence {
namespace {

std::string FormatTitle(const metadb_handle_ptr& track, const char* pattern) {
    titleformat_object::ptr script;
    titleformat_compiler::get()->compile_safe_ex(script, pattern);
    pfc::string8 out;
    // playback_format_title 會一併處理串流的動態資訊（電台目前播放的曲名）。
    if (!playback_control::get()->playback_format_title_ex(track, nullptr, out, script, nullptr, playback_control::display_level_all)) {
        track->format_title(nullptr, out, script, nullptr);
    }
    return out.c_str();
}

int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::optional<discord::Activity> Build() {
    auto pc = playback_control::get();
    metadb_handle_ptr track;
    if (!pc->is_playing() || !pc->get_now_playing(track)) {
        return std::nullopt;
    }
    const bool paused = pc->is_paused();
    if (paused && static_cast<config::PauseMode>(config::pause_mode.get()) == config::PauseMode::clear) {
        return std::nullopt;
    }

    discord::Activity a;
    a.type = static_cast<discord::ActivityType>(config::activity_type.get());
    a.status_display = static_cast<discord::StatusDisplay>(config::status_display.get());
    a.details = FormatTitle(track, config::details_format.get());
    a.state = FormatTitle(track, config::state_format.get());
    a.large_text = FormatTitle(track, config::large_text_format.get());

    if (paused) {
        a.small_text = "Paused";
    } else if (config::show_time) {
        const double position = pc->playback_get_position();
        const double length = pc->playback_get_length_ex();
        const int64_t start = NowMs() - static_cast<int64_t>(position * 1000.0);
        a.start_ms = start;
        if (length > 0 && pc->playback_can_seek()) {
            a.end_ms = start + static_cast<int64_t>(length * 1000.0);
        }
    }
    return a;
}

} // namespace

void Refresh() {
    auto& client = discord::Client::Get();
    if (!config::enabled) {
        client.SetActivity(std::nullopt);
        return;
    }
    client.SetActivity(Build());
}

} // namespace fdl::presence
