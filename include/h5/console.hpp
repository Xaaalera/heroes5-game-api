#pragma once
#include "script_observers.hpp"
#include <cwchar>

namespace h5 {
// Pinned x86 game ABI. The caller verifies the supported game build first.
// Returned true means the inline dispatcher returned, not that the command succeeded.
struct ConsoleCommandRequest {
    uint32_t size = sizeof(ConsoleCommandRequest);
    uint32_t version = 1;
    wchar_t text[4096]{};
};

namespace detail {
inline constexpr hooks::Site<16> ConsoleStringConstructor{0x456220,
    {0x56,0x57,0x8b,0x7c,0x24,0x0c,0x8b,0xf1,0x33,0xc0,0x57,0x89,0x06,0x89,0x46,0x04},
    "game_control native wide-string path; cached listing confirms thiscall and RET4"};
inline constexpr hooks::Site<16> ConsoleDispatcher{0xc125f0,
    {0x6a,0xff,0x68,0x89,0xe9,0xd9,0x00,0x64,0xa1,0,0,0,0,0x50,0x64,0x89},
    "game_control stock console path; inline parser/listener dispatch, not enqueue"};
inline constexpr hooks::Site<16> ConsoleStringRelease{0x879240,
    {0xe9,0x6b,0xfe,0xff,0xff,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc},
    "game_control game allocator release; same-thread TLS required"};
template<size_t Size> inline bool MatchesConsoleSite(const hooks::Site<Size>& site) {
    return hooks::ObserverMemory(site.address, Size, PAGE_EXECUTE_READ) &&
        std::memcmp(reinterpret_cast<const void*>(site.address), site.expected, Size) == 0;
}
// Admission alone does not verify instruction sites or execute game code.
inline bool AllowsConsoleDispatch(HWND gameWindow, const ConsoleCommandRequest& request) {
    DWORD owner = 0;
    const auto thread = GetWindowThreadProcessId(gameWindow, &owner);
    return thread && thread == GetCurrentThreadId() && owner == GetCurrentProcessId() &&
        request.size == sizeof(request) && request.version == 1 && request.text[0] &&
        wcsnlen_s(request.text, 4096) < 4096;
}
}

inline bool DispatchConsoleCommand(HWND gameWindow, const ConsoleCommandRequest& request) {
    if (!detail::AllowsConsoleDispatch(gameWindow, request)) { return false; }
    if (!detail::MatchesConsoleSite(detail::ConsoleStringConstructor) ||
        !detail::MatchesConsoleSite(detail::ConsoleDispatcher) ||
        !detail::MatchesConsoleSite(detail::ConsoleStringRelease)) { return false; }
    // Construction, inline dispatch and release all use the game's own GUI thread.
    struct NativeWideString { void* begin; void* end; void* capacity; } native{};
    static_assert(sizeof(native) == 12, "Console binding requires x86");
    reinterpret_cast<NativeWideString* (__thiscall*)(NativeWideString*, const wchar_t*)>(
        detail::ConsoleStringConstructor.address)(&native, request.text);
    try {
        reinterpret_cast<void (__thiscall*)(NativeWideString*)>(detail::ConsoleDispatcher.address)(&native);
    } catch (...) {
        reinterpret_cast<void (__cdecl*)(void*)>(detail::ConsoleStringRelease.address)(native.begin);
        return false;
    }
    reinterpret_cast<void (__cdecl*)(void*)>(detail::ConsoleStringRelease.address)(native.begin);
    return true;
}
}
