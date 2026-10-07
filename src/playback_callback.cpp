// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "log.h"
#include "presence.h"

namespace fdl {
namespace {

void Refresh() {
    Guarded("playback callback", [] { presence::Refresh(); });
}

class PlaybackCallback : public play_callback_static {
public:
    unsigned get_flags() override {
        return flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_seek | flag_on_playback_pause
            | flag_on_playback_edited | flag_on_playback_dynamic_info_track;
    }

    void on_playback_new_track(metadb_handle_ptr) override {
        Guarded("playback callback", [] { presence::OnNewTrack(); });
        Refresh();
        // 啟動時恢復成暫停狀態的情況下，new_track 當下 is_paused() 未必已經正確，稍後再確認一次。
        // 狀態沒變的話算出的 activity 相同，不會多送給 Discord。
        fb2k::callLater(0.5, [] { Refresh(); });
    }
    void on_playback_stop(play_control::t_stop_reason reason) override {
        if (reason != play_control::stop_reason_starting_another) {
            Guarded("playback callback", [] { presence::OnStop(); });
            Refresh();
            return;
        }
        // 換曲時會先收到 starting_another，接著才是 new_track，這時立刻清除會讓狀態閃一下。
        // 但若下一首其實沒開始（例如清單最後一首按下一首），就不會有 new_track，
        // 所以稍後再依實際播放狀態刷新一次，避免狀態永遠卡住（上游 #85）。
        fb2k::callLater(1.0, [] { Refresh(); });
    }
    void on_playback_seek(double) override { Refresh(); }
    void on_playback_pause(bool paused) override {
        Guarded("playback callback", [paused] { presence::OnPause(paused); });
        Refresh();
    }
    void on_playback_edited(metadb_handle_ptr) override { Refresh(); }
    void on_playback_dynamic_info_track(const file_info&) override {
        Guarded("playback callback", [] { presence::OnStreamTitleChanged(); });
        Refresh();
    }

    void on_playback_starting(play_control::t_track_command, bool) override {}
    void on_playback_dynamic_info(const file_info&) override {}
    void on_playback_time(double) override {}
    void on_volume_change(float) override {}
};

FB2K_SERVICE_FACTORY(PlaybackCallback);

} // namespace
} // namespace fdl
