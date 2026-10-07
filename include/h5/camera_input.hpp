#pragma once
#include "script_observers.hpp"
#include <vector>

namespace h5::hooks {
// Main-thread subscribers vote to suppress camera input. The resident code
// preserves the original virtual dispatch and never owns a DLL callback.
struct CameraInputObservers {
    ScriptObservers observers;
    LONG mouseCapture;
    LONG keyboardCapture;
};
static_assert(offsetof(CameraInputObservers, mouseCapture) == sizeof(ScriptObservers));

inline std::vector<unsigned char> CameraInputCode(uintptr_t code) {
    const auto descriptor = code + ScriptObserverDescriptorOffset;
    const uint32_t mouse = static_cast<uint32_t>(descriptor + offsetof(CameraInputObservers, mouseCapture));
    const uint32_t keyboard = static_cast<uint32_t>(descriptor + offsetof(CameraInputObservers, keyboardCapture));
    const auto observerPrefix = ScriptObserverPrefix(code);
    std::vector<unsigned char> bytes(observerPrefix.begin(), observerPrefix.begin() + 2);
    for (const auto capture : {mouse, keyboard}) {
        const unsigned char reset[]{0xc7, 0x05, 0, 0, 0, 0, 0, 0, 0, 0};
        const auto offset = bytes.size();
        bytes.insert(bytes.end(), std::begin(reset), std::end(reset));
        std::memcpy(bytes.data() + offset + 2, &capture, sizeof(capture));
    }
    bytes.insert(bytes.end(), observerPrefix.begin() + 2, observerPrefix.end());
    bytes.push_back(0x9c); // Preserve flags around capture tests.
    for (const auto capture : {mouse, keyboard}) {
        // CMP [capture],0; JE skip; OR [ESP+argument],1. ESP includes PUSHFD.
        const unsigned char vote[]{0x83, 0x3d, 0, 0, 0, 0, 0, 0x74, 0x05,
                                  0x83, 0x4c, 0x24, 0, 0x01};
        const auto offset = bytes.size();
        bytes.insert(bytes.end(), std::begin(vote), std::end(vote));
        std::memcpy(bytes.data() + offset + 2, &capture, sizeof(capture));
        bytes[offset + 12] = capture == mouse ? 12 : 16;
    }
    const unsigned char tail[]{0x9d, 0xff, 0xa0, 0x90, 0, 0, 0};
    bytes.insert(bytes.end(), std::begin(tail), std::end(tail));
    return bytes;
}

inline CameraInputObservers* CreateCameraInputObservers(uintptr_t site) {
    auto* memory = static_cast<unsigned char*>(VirtualAlloc(nullptr, 16384, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!memory) { return nullptr; }
    const auto code = reinterpret_cast<uintptr_t>(memory);
    const auto bytes = CameraInputCode(code);
    std::memcpy(memory, bytes.data(), bytes.size());
    auto* storage = reinterpret_cast<CameraInputObservers*>(memory + ScriptObserverDescriptorOffset);
    *storage = {{{'H', '5', 'C', 'A', 'M', '0', '0', '1'}, 1, sizeof(CameraInputObservers),
        static_cast<uint32_t>(site), 0x90, static_cast<uint32_t>(bytes.size()), 71, {}, {}}, 0, 0};
    DWORD protection = 0;
    if (!ObserverCodeDigest(code, bytes.size(), storage->observers.codeDigest) ||
        !VirtualProtect(memory, 4096, PAGE_EXECUTE_READ, &protection) ||
        !FlushInstructionCache(GetCurrentProcess(), memory, bytes.size())) {
        VirtualFree(memory, 0, MEM_RELEASE); return nullptr;
    }
    return storage;
}

inline CameraInputObservers* FindCameraInputObservers(uintptr_t site) {
    if (!ObserverMemory(site, 6, PAGE_EXECUTE_READ)) { return nullptr; }
    const auto* call = reinterpret_cast<const unsigned char*>(site);
    if (call[0] != 0xe8 || call[5] != 0x90) { return nullptr; }
    int32_t displacement = 0;
    std::memcpy(&displacement, call + 1, sizeof(displacement));
    const uintptr_t code = site + 5 + displacement;
    if (!code || !ObserverMemory(code, 1024, PAGE_EXECUTE_READ, reinterpret_cast<void*>(code)) ||
        !ObserverMemory(code + ScriptObserverDescriptorOffset, sizeof(CameraInputObservers),
                        PAGE_READWRITE, reinterpret_cast<void*>(code))) { return nullptr; }
    auto* storage = reinterpret_cast<CameraInputObservers*>(code + ScriptObserverDescriptorOffset);
    const auto& descriptor = storage->observers;
    constexpr std::array<unsigned char, 8> magic{'H', '5', 'C', 'A', 'M', '0', '0', '1'};
    const auto expected = CameraInputCode(code);
    if (descriptor.magic != magic || descriptor.version != 1 || descriptor.size != sizeof(CameraInputObservers) ||
        descriptor.callSite != site || descriptor.originalTarget != 0x90 || descriptor.originalCallOffset != 71 ||
        descriptor.codeBytes != expected.size() || std::memcmp(reinterpret_cast<void*>(code), expected.data(), expected.size())) {
        return nullptr;
    }
    std::array<unsigned char, 32> digest{};
    return ObserverCodeDigest(code, expected.size(), digest) && digest == descriptor.codeDigest ? storage : nullptr;
}

inline void CaptureCameraInput(CameraInputObservers& storage, bool mouse, bool keyboard) {
    if (mouse) { InterlockedOr(&storage.mouseCapture, 1); }
    if (keyboard) { InterlockedOr(&storage.keyboardCapture, 1); }
}
}
