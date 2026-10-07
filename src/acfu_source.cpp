// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

// 讓 foo_acfu 能在 foobar2000 裡檢查並下載本元件的更新。沒有安裝 foo_acfu 時這個服務不會被使用。

#include "stdafx.h"

#include "acfu_api.h"
#include "art/http.h"
#include "guids.h"
#include "json_util.h"
#include "log.h"
#include "update_check.h"

namespace fdl {
namespace {

constexpr char kLatestReleaseUrl[] = "https://api.github.com/repos/Carinoasd/foo_discord_listening/releases/latest";

class LatestReleaseRequest : public acfu::request {
public:
    void run(file_info& info, abort_callback& abort) override {
        art::HttpClient http;
        const auto res = http.Get(kLatestReleaseUrl);
        abort.check();
        if (res.status != 200) {
            throw exception_io(pfc::format("GitHub returned HTTP ", res.status, " ", res.error.c_str()));
        }
        const auto doc = nlohmann::json::parse(res.body, nullptr, false);
        const auto tag = json::GetString(doc, "tag_name");
        if (!update::ParseVersion(tag)) {
            throw exception_io_data("unexpected GitHub release data");
        }
        info.meta_set("version", tag.c_str());
        if (const auto page = json::GetString(doc, "html_url"); page.starts_with("https://")) {
            info.meta_set("download_page", page.c_str());
        }
        if (const auto assets = doc.find("assets"); assets != doc.end() && assets->is_array()) {
            for (const auto& asset : *assets) {
                const auto name = json::GetString(asset, "name");
                const auto url = json::GetString(asset, "browser_download_url");
                if (name.ends_with(".fb2k-component") && url.starts_with("https://")) {
                    info.meta_set("download_url", url.c_str());
                    break;
                }
            }
        }
    }
};

class Source : public acfu::source {
public:
    GUID get_guid() override { return guids::acfu_source; }

    void get_info(file_info& info) override {
        info.meta_set("version", FDL_VERSION);
        info.meta_set("name", "Discord Listening");
        info.meta_set("module", "foo_discord_listening");
    }

    bool is_newer(const file_info& info) override {
        const char* latest = info.meta_get("version", 0);
        return latest && update::IsNewer(latest, FDL_VERSION);
    }

    acfu::request::ptr create_request() override { return fb2k::service_new<LatestReleaseRequest>(); }
};

FB2K_SERVICE_FACTORY(Source);

} // namespace
} // namespace fdl
