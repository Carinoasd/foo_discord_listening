// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "presence.h"

namespace fdl {
namespace {

class PlaybackCallback : public play_callback_static {
public:
    unsigned get_flags() override {
        return flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_seek | flag_on_playback_pause
            | flag_on_playback_edited | flag_on_playback_dynamic_info_track;
    }

    void on_playback_new_track(metadb_handle_ptr) override { presence::Refresh(); }
    void on_playback_stop(play_control::t_stop_reason reason) override {
        // 換曲時 foobar2000 會先送 starting_another，接著馬上送 new_track，略過可避免狀態閃一下消失。
        if (reason != play_control::stop_reason_starting_another) {
            presence::Refresh();
        }
    }
    void on_playback_seek(double) override { presence::Refresh(); }
    void on_playback_pause(bool) override { presence::Refresh(); }
    void on_playback_edited(metadb_handle_ptr) override { presence::Refresh(); }
    void on_playback_dynamic_info_track(const file_info&) override { presence::Refresh(); }

    void on_playback_starting(play_control::t_track_command, bool) override {}
    void on_playback_dynamic_info(const file_info&) override {}
    void on_playback_time(double) override {}
    void on_volume_change(float) override {}
};

FB2K_SERVICE_FACTORY(PlaybackCallback);

} // namespace
} // namespace fdl
