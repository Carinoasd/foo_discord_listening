// 測試工具用的 log 實作：輸出到 stderr，不依賴 foobar2000。
#include "stdafx.h"

#include "log.h"

#include <cstdio>
#include <mutex>

namespace fdl {

void WriteLog(const std::string& message, bool) {
    static std::mutex m;
    std::scoped_lock lock(m);
    std::fprintf(stderr, "  [log] %s\n", message.c_str());
}

bool DebugLogEnabled() {
    return false;
}

void InitLog() {}

} // namespace fdl
