// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#pragma once

// foo_acfu（Auto Check For Updates，作者 3dyd）的服務介面。
// 只宣告與 foo_acfu 互通所必需的部分：介面 GUID 與虛擬函式的順序必須與 foo_acfu 一致。
// acfu-sdk 沒有附授權條款，所以不引入它的程式碼，介面在此依其公開文件自行宣告。

namespace acfu {

/// foo_acfu 在背景執行緒呼叫 run()，由我們填入最新版本資訊（meta：version、download_page、download_url）。
class NOVTABLE request : public service_base {
    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(request);

public:
    virtual void run(file_info& info, abort_callback& abort) = 0;
};

/// 向 foo_acfu 登記的更新來源。
class NOVTABLE source : public service_base {
    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(source);

public:
    virtual GUID get_guid() = 0;
    virtual void get_info(file_info& info) = 0;
    virtual bool is_newer(const file_info& info) = 0;
    virtual request::ptr create_request() = 0;
    virtual void context_menu_build(HMENU, unsigned) {}
    virtual void context_menu_command(unsigned, unsigned) {}
};

// {4E88EA57-ABDD-49AD-B72B-7C198DA27DBE}
FOOGUIDDECL const GUID request::class_guid = { 0x4e88ea57, 0xabdd, 0x49ad, { 0xb7, 0x2b, 0x7c, 0x19, 0x8d, 0xa2, 0x7d, 0xbe } };
// {9A5442D9-77F9-4918-BAE3-F9D059F4681B}
FOOGUIDDECL const GUID source::class_guid = { 0x9a5442d9, 0x77f9, 0x4918, { 0xba, 0xe3, 0xf9, 0xd0, 0x59, 0xf4, 0x68, 0x1b } };

} // namespace acfu
