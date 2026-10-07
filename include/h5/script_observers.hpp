#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "hooks.hpp"

namespace h5::hooks {
// Published by the owned SDK mailbox or a native owner on the game thread.
// Observers are cdecl void(), main-thread only, non-reentrant and resident until
// removal is confirmed. This is a trusted in-process contract, not authentication
// against other code with permission to modify the same process.
inline constexpr size_t ScriptObserverCapacity = 64;
inline constexpr uintptr_t ScriptObserverDescriptorOffset = 4096 + 3072;
struct ScriptObservers {
    std::array<unsigned char, 8> magic;
    uint32_t version;
    uint32_t size;
    uint32_t callSite;
    uint32_t originalTarget;
    uint32_t codeBytes;
    uint32_t originalCallOffset;
    std::array<unsigned char, 32> codeDigest;
    std::array<LONG, ScriptObserverCapacity> callbacks;
};
static_assert(offsetof(ScriptObservers, callbacks) == 64);
static_assert(sizeof(ScriptObservers) == 320);

inline std::array<unsigned char, 51> ScriptObserverPrefix(uintptr_t code) {
    std::array<unsigned char, 51> prefix{0x9c, 0x60, 0x89, 0xe5, 0x81, 0xec, 0x10, 0x02, 0, 0,
        0x83, 0xe4, 0xf0, 0x0f, 0xae, 0x04, 0x24, 0xfc, 0xbe, 0, 0, 0, 0,
        0xbf, 0x40, 0, 0, 0, 0x8b, 0x06, 0x85, 0xc0, 0x74, 0x03, 0xff, 0xd0, 0xfc,
        0x83, 0xc6, 0x04, 0x4f, 0x75, 0xf1, 0x0f, 0xae, 0x0c, 0x24, 0x89, 0xec, 0x61, 0x9d};
    const auto slots = static_cast<uint32_t>(code + ScriptObserverDescriptorOffset + offsetof(ScriptObservers, callbacks));
    std::memcpy(prefix.data() + 19, &slots, sizeof(slots));
    return prefix;
}

inline bool ObserverMemory(uintptr_t address, size_t bytes, DWORD protection, void* allocation = nullptr) {
    MEMORY_BASIC_INFORMATION region{};
    return bytes && address + bytes >= address && VirtualQuery(reinterpret_cast<void*>(address),
        &region, sizeof(region)) && region.State == MEM_COMMIT && region.Protect == protection &&
        address + bytes <= reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize &&
        (!allocation || (region.Type == MEM_PRIVATE && region.AllocationBase == allocation));
}

inline bool ObserverCodeDigest(uintptr_t address, size_t bytes, std::array<unsigned char, 32>& digest) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (bytes > 1024 || BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        return false;
    }
    const bool valid = BCryptHash(algorithm, nullptr, 0, reinterpret_cast<PUCHAR>(address),
        static_cast<ULONG>(bytes), digest.data(), static_cast<ULONG>(digest.size())) == 0;
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return valid;
}

// Prepare self-contained resident code; the caller publishes the CALL only on
// the validated game thread after comparing the original signature. No DLL code
// pointer is retained by this dispatcher. Once published it lives until exit,
// including periods with zero subscribers, so core replacement can reuse it.
inline ScriptObservers* CreateScriptObserverStorage(uintptr_t site, uintptr_t target) {
    auto* memory = static_cast<unsigned char*>(VirtualAlloc(nullptr, 16384, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!memory) { return nullptr; }
    const auto code = reinterpret_cast<uintptr_t>(memory);
    const auto prefix = ScriptObserverPrefix(code);
    std::memcpy(memory, prefix.data(), prefix.size());
    // Tail jump preserves the original return address and stack arguments.
    memory[prefix.size()] = 0xe9;
    const auto displacement = static_cast<uint32_t>(target - code - prefix.size() - 5);
    std::memcpy(memory + prefix.size() + 1, &displacement, sizeof(displacement));
    auto* observers = reinterpret_cast<ScriptObservers*>(memory + ScriptObserverDescriptorOffset);
    *observers = {{'H', '5', 'O', 'B', 'S', '0', '0', '1'}, 1, sizeof(ScriptObservers),
        static_cast<uint32_t>(site), static_cast<uint32_t>(target),
        static_cast<uint32_t>(prefix.size() + 5), static_cast<uint32_t>(prefix.size()), {}, {}};
    DWORD protection = 0;
    if (!ObserverCodeDigest(code, observers->codeBytes, observers->codeDigest) ||
        !VirtualProtect(memory, 4096, PAGE_EXECUTE_READ, &protection) ||
        !FlushInstructionCache(GetCurrentProcess(), memory, observers->codeBytes)) {
        VirtualFree(memory, 0, MEM_RELEASE); return nullptr;
    }
    return observers;
}

inline ScriptObservers* FindScriptObservers(uintptr_t site, uintptr_t target) {
    if (!ObserverMemory(site, 5, PAGE_EXECUTE_READ)) { return nullptr; }
    const auto* call = reinterpret_cast<const unsigned char*>(site);
    if (call[0] != 0xe8) { return nullptr; }
    int32_t displacement = 0;
    std::memcpy(&displacement, call + 1, sizeof(displacement));
    const uintptr_t code = site + 5 + displacement;
    if (!code || !ObserverMemory(code, 1024, PAGE_EXECUTE_READ, reinterpret_cast<void*>(code))) { return nullptr; }
    const uintptr_t descriptor = code + ScriptObserverDescriptorOffset;
    if (!ObserverMemory(descriptor, sizeof(ScriptObservers), PAGE_READWRITE, reinterpret_cast<void*>(code))) {
        return nullptr;
    }
    auto* observers = reinterpret_cast<ScriptObservers*>(descriptor);
    constexpr std::array<unsigned char, 8> magic{'H', '5', 'O', 'B', 'S', '0', '0', '1'};
    if (observers->magic != magic || observers->version != 1 || observers->size != sizeof(ScriptObservers) ||
        observers->callSite != site || observers->originalTarget != target ||
        observers->codeBytes < 5 || observers->codeBytes > 1024 ||
        observers->originalCallOffset > observers->codeBytes - 5) { return nullptr; }
    const auto prefix = ScriptObserverPrefix(code);
    if (observers->originalCallOffset != prefix.size() ||
        std::memcmp(reinterpret_cast<void*>(code), prefix.data(), prefix.size()) != 0) { return nullptr; }
    const auto* original = reinterpret_cast<const unsigned char*>(code + observers->originalCallOffset);
    std::memcpy(&displacement, original + 1, sizeof(displacement));
    const bool nativeTailJump = original[0] == 0xe9 && observers->codeBytes == prefix.size() + 5;
    if ((!nativeTailJump && original[0] != 0xe8) ||
        reinterpret_cast<uintptr_t>(original) + 5 + displacement != target) { return nullptr; }
    std::array<unsigned char, 32> digest{};
    if (!ObserverCodeDigest(code, observers->codeBytes, digest) || digest != observers->codeDigest) { return nullptr; }
    return observers;
}

// Invoke and subscription changes run on the same validated game thread.
// Interlocked publication also keeps reads aligned and atomic on x86.
inline LONG* AddScriptObserver(ScriptObservers& observers, void (__cdecl* callback)()) {
    static_assert(sizeof(void*) == sizeof(LONG), "Script observers require x86");
    if (!callback) { return nullptr; }
    const auto value = static_cast<LONG>(reinterpret_cast<uintptr_t>(callback));
    for (auto& slot : observers.callbacks) {
        if (InterlockedCompareExchange(&slot, value, 0) == 0) { return &slot; }
    }
    return nullptr;
}
inline bool RemoveScriptObserver(LONG* slot, void (__cdecl* callback)()) {
    if (!slot || !callback) { return false; }
    const auto value = static_cast<LONG>(reinterpret_cast<uintptr_t>(callback));
    return InterlockedCompareExchange(slot, 0, value) == value;
}
}
