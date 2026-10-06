// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include "stdafx.h"

#include "art/uploader.h"

#include <chrono>
#include <vector>

namespace fdl::art {
namespace {

constexpr auto kTimeout = std::chrono::seconds(30);
constexpr size_t kMaxOutput = 64 * 1024;

struct Handle {
    HANDLE h = nullptr;
    Handle() = default;
    explicit Handle(HANDLE handle) : h(handle) {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { Reset(); }
    void Reset() {
        if (h && h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
        }
        h = nullptr;
    }
};

std::wstring Widen(std::string_view s) {
    return pfc::stringcvt::string_wide_from_utf8(std::string(s).c_str()).get_ptr();
}

} // namespace

UploadResult RunUploader(const std::string& command, const std::filesystem::path& image, const std::atomic<bool>& cancel) {
    UploadResult result;
    const std::wstring quoted = L"\"" + image.wstring() + L"\"";
    std::wstring cmdline = Widen(command);
    const bool path_in_args = cmdline.find(L"{path}") != std::wstring::npos;
    for (size_t pos; (pos = cmdline.find(L"{path}")) != std::wstring::npos;) {
        cmdline.replace(pos, 6, quoted);
    }

    SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
    Handle out_read, out_write, in_read, in_write;
    if (!CreatePipe(&out_read.h, &out_write.h, &sa, 0) || !CreatePipe(&in_read.h, &in_write.h, &sa, 0)) {
        result.error = "CreatePipe failed";
        return result;
    }
    SetHandleInformation(out_read.h, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(in_write.h, HANDLE_FLAG_INHERIT, 0);

    // 放進 job object：foobar2000 結束或逾時時，上傳程式與它的子行程會一起被終止。
    Handle job(CreateJobObjectW(nullptr, nullptr));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(job.h, JobObjectExtendedLimitInformation, &limits, sizeof(limits));

    // 只讓子行程繼承這兩個 pipe，不要把 foobar2000 裡其他元件的可繼承 handle 也帶過去。
    HANDLE inherit[] = { in_read.h, out_write.h };
    SIZE_T attr_size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_size);
    std::vector<char> attr_buffer(attr_size);
    auto* attrs = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attr_buffer.data());
    if (!InitializeProcThreadAttributeList(attrs, 1, 0, &attr_size)
        || !UpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherit, sizeof(inherit), nullptr, nullptr)) {
        result.error = "could not prepare uploader process";
        return result;
    }
    struct AttrGuard {
        LPPROC_THREAD_ATTRIBUTE_LIST list;
        ~AttrGuard() { DeleteProcThreadAttributeList(list); }
    } attr_guard{ attrs };

    STARTUPINFOEXW si{};
    si.StartupInfo.cb = sizeof(si);
    si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    si.StartupInfo.hStdInput = in_read.h;
    si.StartupInfo.hStdOutput = out_write.h;
    si.StartupInfo.hStdError = out_write.h;
    si.lpAttributeList = attrs;
    PROCESS_INFORMATION pi{};
    // 透過 cmd /c 執行，讓使用者可以直接填 python script.py 或 .bat 之類的指令。
    std::wstring full = L"cmd.exe /d /s /c \"" + cmdline + L"\"";
    const DWORD flags = CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT;
    if (!CreateProcessW(nullptr, full.data(), nullptr, nullptr, TRUE, flags, nullptr, nullptr, &si.StartupInfo, &pi)) {
        result.error = "could not start uploader (" + std::to_string(GetLastError()) + ")";
        return result;
    }
    Handle process(pi.hProcess);
    Handle thread(pi.hThread);
    const bool in_job = job.h && AssignProcessToJobObject(job.h, process.h);
    ResumeThread(thread.h);
    // 放不進 job 時（例如 foobar2000 本身已在不允許巢狀的 job 中），逾時或取消就直接終止行程。
    auto kill_if_needed = [&] {
        if (!in_job) {
            TerminateProcess(process.h, 1);
        }
    };
    in_read.Reset();
    out_write.Reset();

    if (!path_in_args) {
        const std::string path = pfc::stringcvt::string_utf8_from_wide(image.c_str()).get_ptr();
        DWORD written = 0;
        WriteFile(in_write.h, path.data(), static_cast<DWORD>(path.size()), &written, nullptr);
    }
    in_write.Reset();

    // 邊等邊讀 stdout：輸出量超過 pipe 緩衝區時，不讀的話子行程會卡住（上游的死鎖問題）。
    std::string output;
    const auto deadline = std::chrono::steady_clock::now() + kTimeout;
    bool exited = false;
    for (;;) {
        DWORD available = 0;
        while (PeekNamedPipe(out_read.h, nullptr, 0, nullptr, &available, nullptr) && available > 0) {
            char buffer[4096];
            DWORD read = 0;
            if (!ReadFile(out_read.h, buffer, std::min<DWORD>(available, sizeof(buffer)), &read, nullptr) || read == 0) {
                break;
            }
            if (output.size() < kMaxOutput) {
                output.append(buffer, read);
            }
        }
        if (exited) {
            break;
        }
        if (cancel) {
            kill_if_needed();
            result.error = "cancelled";
            return result; // 在 job 中時，job handle 關閉會終止子行程
        }
        if (std::chrono::steady_clock::now() > deadline) {
            kill_if_needed();
            result.error = "uploader timed out";
            return result;
        }
        exited = WaitForSingleObject(process.h, 100) == WAIT_OBJECT_0;
    }

    DWORD exit_code = 0;
    GetExitCodeProcess(process.h, &exit_code);
    result.url = ExtractUploadedUrl(output);
    if (result.url.empty()) {
        std::string snippet = output.substr(0, 200);
        result.error = "uploader exited with code " + std::to_string(exit_code) + " without printing an https URL"
            + (snippet.empty() ? std::string{} : ": " + snippet);
    }
    return result;
}

} // namespace fdl::art
