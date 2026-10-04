#pragma once
#include "build.hpp"

namespace h5 {
// The caller supplies provenance from its own launch. This function validates
// identity; it never searches for or authorizes attachment to someone else's game.
inline HANDLE OpenOwnedProcess(DWORD pid, uint64_t creation, const std::filesystem::path& executable,
    DWORD access) {
    const auto process = OpenProcess(access | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) { throw std::runtime_error("owned_process_missing"); }
    try {
        FILETIME created{}, exited{}, kernel{}, user{};
        if (!GetProcessTimes(process, &created, &exited, &kernel, &user)) {
            throw std::runtime_error("creation_read_failed");
        }
        const auto actual = (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
        if (actual != creation) { throw std::runtime_error("pid_creation_mismatch"); }
        wchar_t path[32768]{}; DWORD length = 32768;
        if (!QueryFullProcessImageNameW(process, 0, path, &length)) { throw std::runtime_error("game_path_read_failed"); }
        if (!std::filesystem::equivalent(path, executable)) { throw std::runtime_error("game_path_mismatch"); }
        VerifyGame(path);
        return process; // Caller owns CloseHandle.
    } catch (...) {
        CloseHandle(process);
        throw;
    }
}
}
