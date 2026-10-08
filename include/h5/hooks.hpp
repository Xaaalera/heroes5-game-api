#pragma once
#include <cstddef>
#include <cstdint>

namespace h5::hooks {
// Catalog entries describe bytes of the pinned Universe build, not a promise
// that arbitrary replacement code is safe at these sites.
template <size_t Size> struct Site {
    uintptr_t address;
    unsigned char expected[Size];
    const char* evidence;
    constexpr uintptr_t Resume() const { return address + Size; }
};
inline constexpr Site<6> BankLayout{0x5f8800, {0x8b, 0x2d, 0x68, 0x96, 0xfd, 0x00},
    "bank-reference InstallSelector: native installer test and live reference window"};
inline constexpr Site<6> PlacementRenderer{0x577010, {0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8},
    "deployment-preview InstallRendererHook: shipped legacy renderer hook"};
inline constexpr Site<5> AdventureAttack{0x433440, {0x83, 0xec, 0x50, 0x53, 0x55},
    "deployment-preview InstallRendererHook: pre-battle entry"};
inline constexpr Site<6> PlacementHover{0x5767a0, {0x81, 0xec, 0xbc, 0, 0, 0},
    "deployment-preview InstallRendererHook: pointer hover"};
inline constexpr Site<5> ScriptDispatchCall{0xd112db, {0xe8, 0xa0, 0xf6, 0xff, 0xff},
    "devkit game_control and native HMR: live main-thread CALL observer"};
inline constexpr uintptr_t ScriptDispatchTarget = 0xd10980;
inline constexpr Site<6> AdventureCameraInput{0x58c69b, {0xff, 0x90, 0x90, 0, 0, 0},
    "pinned adventure input handler: virtual camera input with mouse/keyboard suppression"};
inline constexpr Site<5> AdventureInputTail{0x5a1a03, {0xe9,0x08,0xac,0xfe,0xff},
    "validated adjustor tail before adventure binding dispatch; owned physical keyboard trace"};
inline constexpr uintptr_t AdventureInputTarget = 0x58c610;
inline constexpr Site<16> ExitRequest{0x838c10,
    {0x56, 0x8b, 0xf1, 0x85, 0xf6, 0x74, 0x04, 0x83, 0x46, 0x04, 0x01, 0x57, 0x8b, 0x3d, 0x98, 0x71},
    "devkit native exit: ECX=null queues game exit; verified owned mailbox exit0"};
}
