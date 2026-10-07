// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "update_check.h"

#include "art/http.h"
#include "guids.h"
#include "json_util.h"
#include "log.h"
#include "i18n.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace fdl::update {
namespace {

constexpr char kLatestReleaseUrl[] = "https://api.github.com/repos/Carinoasd/foo_discord_listening/releases/latest";
constexpr int64_t kIntervalSeconds = 24 * 3600;

advconfig_checkbox_factory g_enabled("Check for updates once a day", "foo_discord_listening.check_updates", guids::cfg_check_updates,
                                     guids::advconfig_branch, 1, true);
cfg_int g_last_check(guids::cfg_last_update_check, 0);
cfg_string g_known_latest(guids::cfg_known_latest_version, "");

std::mutex g_mutex;
std::string g_newer;
std::shared_ptr<art::HttpClient> g_http;
std::jthread g_thread;

int64_t NowSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

void Remember(const std::string& latest) {
    if (IsNewer(latest, FDL_VERSION)) {
        {
            std::scoped_lock lock(g_mutex);
            g_newer = latest;
        }
        Log("{} https://github.com/Carinoasd/foo_discord_listening/releases/latest", Format(StringId::update_available, latest));
    }
}

} // namespace

void Start() {
    // 先用上次查到的結果，這樣即使今天不查，設定頁也能提示。
    Remember(g_known_latest.get().c_str());
    if (!g_enabled.get() || NowSeconds() - g_last_check.get() < kIntervalSeconds) {
        return;
    }
    g_last_check = NowSeconds();
    g_http = std::make_shared<art::HttpClient>();
    g_thread = std::jthread([http = g_http](std::stop_token stop) {
        Guarded("update check", [&] {
            // 啟動後稍等再查，不和 foobar2000 啟動時的其他工作搶資源。
            for (int i = 0; i < 50 && !stop.stop_requested(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if (stop.stop_requested()) {
                return;
            }
            const auto res = http->Get(kLatestReleaseUrl);
            if (res.status != 200) {
                DebugLog("update check failed: HTTP {} {}", res.status, res.error);
                return;
            }
            const auto doc = nlohmann::json::parse(res.body, nullptr, false);
            const auto tag = json::GetString(doc, "tag_name");
            DebugLog("update check: latest release is {}", tag);
            if (ParseVersion(tag)) {
                fb2k::inMainThread([tag] {
                    g_known_latest = tag.c_str();
                    Remember(tag);
                });
            }
        });
    });
}

void Stop() {
    if (g_thread.joinable()) {
        g_thread.request_stop();
        if (g_http) {
            g_http->Cancel();
        }
        g_thread.join();
    }
    g_http.reset();
}

std::string NewerVersion() {
    std::scoped_lock lock(g_mutex);
    return g_newer;
}

} // namespace fdl::update
