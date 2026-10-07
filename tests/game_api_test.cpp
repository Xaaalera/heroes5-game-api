#include <h5/build.hpp>
#include <h5/hooks.hpp>
#include <h5/process.hpp>
#include <h5/image_placement.hpp>
#include <h5/graphics_lifetime.hpp>
#include <h5/script_observers.hpp>
#include <h5/camera_input.hpp>
#include <xmmintrin.h>
#include <iostream>

namespace {
void VerifyGraphicsTransaction() {
    struct FailureCase { const char* name; int commitFailure; int rollbackFailure; };
    const std::array<FailureCase, 10> cases{{
        {"success", 0, 0}, {"partial-write", 1, 0}, {"restore-protection", 2, 0},
        {"cache-flush", 3, 0}, {"readback", 4, 0}, {"rollback-writable", 1, 1},
        {"rollback-write", 1, 2}, {"rollback-protection", 1, 3},
        {"rollback-flush", 1, 4}, {"rollback-readback", 1, 5}
    }};
    for (const auto& scenario : cases) {
        std::array<unsigned char, 42> original{}, repaired{};
        original.fill(0x11); repaired.fill(0x22);
        auto bytes = original;
        bool writable = true, rollingBack = false;
        int writes = 0, protections = 0, flushes = 0, verifications = 0;
        auto write = [&](const auto& expected) {
            ++writes;
            if ((!rollingBack && scenario.commitFailure == 1) ||
                (rollingBack && scenario.rollbackFailure == 2)) {
                bytes[0] = expected[0]; rollingBack = true; return false;
            }
            bytes = expected; return true;
        };
        auto protect = [&](bool makeWritable) {
            ++protections;
            if (makeWritable) { rollingBack = true; }
            if ((!rollingBack && scenario.commitFailure == 2) ||
                (rollingBack && scenario.rollbackFailure == (makeWritable ? 1 : 3))) {
                rollingBack = true; return false;
            }
            writable = makeWritable; return true;
        };
        auto flush = [&]() {
            ++flushes;
            if ((!rollingBack && scenario.commitFailure == 3) ||
                (rollingBack && scenario.rollbackFailure == 4)) { rollingBack = true; return false; }
            return true;
        };
        auto verify = [&](const auto& expected) {
            ++verifications;
            if ((!rollingBack && scenario.commitFailure == 4) ||
                (rollingBack && scenario.rollbackFailure == 5)) { rollingBack = true; return false; }
            return bytes == expected && !writable;
        };
        std::string reason;
        try { h5::detail::CommitGraphicsRepair(original, repaired, write, protect, flush, verify); }
        catch (const std::runtime_error& error) { reason = error.what(); }
        const std::string expectedReason = scenario.commitFailure == 0 ? "" :
            scenario.rollbackFailure == 0 ? "graphics_proxy_repair_rejected" : "graphics_proxy_rollback_unconfirmed";
        if (reason != expectedReason || (reason.empty() && (bytes != repaired || writable)) ||
            (reason == "graphics_proxy_repair_rejected" && (bytes != original || writable)) ||
            (scenario.commitFailure && (protections < 2 || flushes < 1 || verifications < 1)) || writes < 1) {
            throw std::runtime_error(std::string("Graphics transaction failed: ") + scenario.name);
        }
    }
}
h5::hooks::CameraInputObservers* cameraFixtureStorage = nullptr;
struct CameraFixtureOwner {
    void** table;
    const void* event = nullptr;
    uint32_t mouse = 0, keyboard = 0, target = 0;
};
void __fastcall FirstCameraTarget(CameraFixtureOwner* owner, void*, const void* event, uint32_t mouse, uint32_t keyboard) {
    owner->event = event; owner->mouse = mouse; owner->keyboard = keyboard; owner->target = 1;
}
void __fastcall SecondCameraTarget(CameraFixtureOwner* owner, void*, const void* event, uint32_t mouse, uint32_t keyboard) {
    owner->event = event; owner->mouse = mouse; owner->keyboard = keyboard; owner->target = 2;
}
void __cdecl MouseCaptureFixture() {
    h5::hooks::CaptureCameraInput(*cameraFixtureStorage, true, false);
    _mm_setcsr((_mm_getcsr() & ~0x6000u) | 0x2000u);
}
void __cdecl KeyboardCaptureFixture() {
    h5::hooks::CaptureCameraInput(*cameraFixtureStorage, false, true);
}
void VerifyCameraInputDispatcher() {
    auto* caller = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!caller) { throw std::runtime_error("Camera fixture allocation failed"); }
    // Forward three arguments, then use the same virtual call as the game.
    const unsigned char body[]{0xff,0x74,0x24,0x0c, 0xff,0x74,0x24,0x0c, 0xff,0x74,0x24,0x0c,
        0x8b,0x01, 0xff,0x90,0x90,0,0,0, 0xc2,0x0c,0};
    std::memcpy(caller, body, sizeof(body));
    const auto site = reinterpret_cast<uintptr_t>(caller + 14);
    cameraFixtureStorage = h5::hooks::CreateCameraInputObservers(site);
    if (!cameraFixtureStorage) { throw std::runtime_error("Camera dispatcher allocation failed"); }
    const auto code = reinterpret_cast<uintptr_t>(cameraFixtureStorage) - h5::hooks::ScriptObserverDescriptorOffset;
    caller[14] = 0xe8;
    const auto displacement = static_cast<uint32_t>(code - site - 5);
    std::memcpy(caller + 15, &displacement, sizeof(displacement));
    caller[19] = 0x90;
    DWORD protection = 0;
    if (!VirtualProtect(caller, 4096, PAGE_EXECUTE_READ, &protection) ||
        !FlushInstructionCache(GetCurrentProcess(), caller, sizeof(body)) ||
        h5::hooks::FindCameraInputObservers(site) != cameraFixtureStorage) {
        throw std::runtime_error("Camera sealed dispatcher validation failed");
    }
    std::array<void*, 37> table{};
    table[36] = reinterpret_cast<void*>(FirstCameraTarget);
    CameraFixtureOwner owner{table.data()};
    const auto invoke = reinterpret_cast<void (__thiscall*)(CameraFixtureOwner*, const void*, uint32_t, uint32_t)>(caller);
    const auto event = reinterpret_cast<void*>(1234);
    invoke(&owner, event, 0, 1);
    if (owner.event != event || owner.mouse || owner.keyboard != 1 || owner.target != 1) {
        throw std::runtime_error("Zero-subscriber forwarding changed virtual arguments");
    }
    auto* mouse = h5::hooks::AddScriptObserver(cameraFixtureStorage->observers, MouseCaptureFixture);
    auto* keyboard = h5::hooks::AddScriptObserver(cameraFixtureStorage->observers, KeyboardCaptureFixture);
    const auto control = _mm_getcsr();
    invoke(&owner, event, 0, 0);
    if (!mouse || !keyboard || owner.mouse != 1 || owner.keyboard != 1 || _mm_getcsr() != control) {
        throw std::runtime_error("Camera capture votes or floating-point preservation failed");
    }
    table[36] = reinterpret_cast<void*>(SecondCameraTarget);
    if (!h5::hooks::RemoveScriptObserver(mouse, MouseCaptureFixture)) { throw std::runtime_error("Camera mouse removal failed"); }
    invoke(&owner, event, 0, 0);
    if (owner.target != 2 || owner.mouse || owner.keyboard != 1) {
        throw std::runtime_error("Camera dispatcher cached a virtual target or removed another subscriber");
    }
    if (!h5::hooks::RemoveScriptObserver(keyboard, KeyboardCaptureFixture)) { throw std::runtime_error("Camera keyboard removal failed"); }
    invoke(&owner, event, 0, 0);
    if (owner.mouse || owner.keyboard) { throw std::runtime_error("Camera capture votes survived subscriber removal"); }
    cameraFixtureStorage->observers.version++;
    if (h5::hooks::FindCameraInputObservers(site)) { throw std::runtime_error("Wrong camera descriptor version accepted"); }
    VirtualFree(caller, 0, MEM_RELEASE);
    VirtualFree(reinterpret_cast<void*>(code), 0, MEM_RELEASE);
    cameraFixtureStorage = nullptr;
}
}

int main() {
    try {
        VerifyGraphicsTransaction();
        VerifyCameraInputDispatcher();
        static_assert(h5::hooks::BankLayout.Resume() == 0x5f8806);
        static_assert(h5::hooks::PlacementRenderer.Resume() == 0x577016);
        static_assert(h5::hooks::AdventureAttack.Resume() == 0x433445);
        static_assert(h5::hooks::PlacementHover.Resume() == 0x5767a6);
        int32_t offset = 0;
        std::memcpy(&offset, h5::hooks::ScriptDispatchCall.expected + 1, 4);
        if (h5::hooks::ScriptDispatchCall.Resume() + offset != h5::hooks::ScriptDispatchTarget) { return 1; }
        const auto directory = std::filesystem::temp_directory_path() /
            ("h5-api-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
        if (!std::filesystem::create_directory(directory)) { return 1; }
        const auto sample = directory / "H5_Game.exe";
        { std::ofstream file(sample); file << "abc"; }
        const auto digest = h5::Sha256(sample);
        const auto graphics = directory / "d3d9.dll";
        const auto retainedGraphics = directory / "d3d9.universe.dll";
        { std::ofstream file(graphics); file << "abc"; }
        { std::ofstream file(retainedGraphics); file << "not the original Universe wrapper"; }
        for (const auto& permitted : {std::string(), digest, std::string(64, 'A'), std::string(64, '0')}) {
            bool rejected = false;
            try { h5::VerifyGraphicsFiles(directory, permitted); }
            catch (const std::runtime_error&) { rejected = true; }
            if (!rejected) { throw std::runtime_error("Untrusted facade or retained graphics accepted"); }
        }
        std::filesystem::remove(graphics);
        std::filesystem::remove(retainedGraphics);
        const auto pinnedFile = CreateFileW(sample.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (pinnedFile == INVALID_HANDLE_VALUE) { return 1; }
        LARGE_INTEGER position{};
        position.QuadPart = 2;
        if (!SetFilePointerEx(pinnedFile, position, nullptr, FILE_BEGIN)) { return 1; }
        const auto renamed = directory / "original.bin";
        std::filesystem::rename(sample, renamed);
        { std::ofstream file(sample); file << "replacement"; }
        if (h5::Sha256(pinnedFile) != digest || h5::Sha256(sample) == digest ||
            !SetFilePointerEx(pinnedFile, {}, &position, FILE_CURRENT) || position.QuadPart != 2) {
            throw std::runtime_error("Hash did not preserve pinned file identity and cursor");
        }
        CloseHandle(pinnedFile);
        std::filesystem::remove(sample);
        std::filesystem::rename(renamed, sample);
        const auto empty = directory / "empty.bin";
        { std::ofstream file(empty); }
        if (h5::Sha256(empty) != "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") { return 1; }
        std::filesystem::remove(empty);
        bool buildRejected = false;
        try { h5::VerifyGame(sample); } catch (const std::runtime_error& error) {
            buildRejected = std::string(error.what()).find("Unsupported Universe build") == 0;
        }
        if (digest != "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" || !buildRejected) { return 1; }
        bool identityRejected = false;
        try {
            const auto process = h5::OpenOwnedProcess(GetCurrentProcessId(), 0, L"H5_Game.exe", PROCESS_QUERY_INFORMATION);
            CloseHandle(process);
        } catch (const std::runtime_error&) { identityRejected = true; }
        if (!identityRejected) { return 1; }
        FILETIME created{}, exited{}, kernel{}, user{};
        if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) { return 1; }
        const auto creation = (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
        bool graphicsBoundsRejected = false;
        try { h5::RepairOwnedGraphicsProxyLifetime(GetCurrentProcessId(), creation, sample, nullptr, nullptr); }
        catch (const std::runtime_error& error) { graphicsBoundsRejected = std::string(error.what()) == "graphics_proxy_bounds_invalid"; }
        if (!graphicsBoundsRejected) { return 1; }
        DWORD graphicsHandlesBefore = 0, graphicsHandlesAfter = 0;
        if (!GetProcessHandleCount(GetCurrentProcess(), &graphicsHandlesBefore)) { return 1; }
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            bool graphicsOwnerRejected = false;
            try { h5::RepairOwnedGraphicsProxyLifetime(GetCurrentProcessId(), creation + 1, sample, nullptr, reinterpret_cast<void*>(4096)); }
            catch (const std::runtime_error& error) { graphicsOwnerRejected = std::string(error.what()) == "pid_creation_mismatch"; }
            if (!graphicsOwnerRejected) { return 1; }
        }
        if (!GetProcessHandleCount(GetCurrentProcess(), &graphicsHandlesAfter) || graphicsHandlesBefore != graphicsHandlesAfter) { return 1; }
        const h5::ImagePlacementContract invalidRange{sample, digest, 4096, 4096, 4095, 2};
        bool rangeRejected = false;
        try { h5::VerifyOwnedImagePlacement(GetCurrentProcessId(), creation, sample, nullptr, nullptr, invalidRange); }
        catch (const std::runtime_error& error) { rangeRejected = std::string(error.what()) == "image_contract_bounds_invalid"; }
        if (!rangeRejected) { return 1; }
        const h5::ImagePlacementContract validRange{sample, digest, 4096, 4096, 0, 4};
        bool ownerRejected = false;
        try { h5::VerifyOwnedImagePlacement(GetCurrentProcessId(), creation + 1, sample, nullptr, nullptr, validRange); }
        catch (const std::runtime_error& error) { ownerRejected = std::string(error.what()) == "pid_creation_mismatch"; }
        if (!ownerRejected) { return 1; }
        DWORD handlesBefore = 0, handlesAfter = 0;
        if (!GetProcessHandleCount(GetCurrentProcess(), &handlesBefore)) { return 1; }
        bool pathRejected = false;
        try {
            const auto process = h5::OpenOwnedProcess(GetCurrentProcessId(), creation, sample, PROCESS_QUERY_INFORMATION);
            CloseHandle(process);
        } catch (const std::runtime_error& error) { pathRejected = std::string(error.what()) == "game_path_mismatch"; }
        if (!GetProcessHandleCount(GetCurrentProcess(), &handlesAfter) || handlesBefore != handlesAfter || !pathRejected) { return 1; }
        std::filesystem::remove(sample);
        std::filesystem::remove(directory);
        auto* memory = static_cast<unsigned char*>(VirtualAlloc(nullptr, 16384, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (!memory) { throw std::runtime_error("Observer fixture allocation failed"); }
        const auto code = reinterpret_cast<uintptr_t>(memory);
        const auto site = code + 1024, target = code + 2048;
        const auto prefix = h5::hooks::ScriptObserverPrefix(code);
        std::memcpy(memory, prefix.data(), prefix.size());
        memory[prefix.size()] = 0xe8;
        offset = static_cast<int32_t>(target - code - prefix.size() - 5);
        std::memcpy(memory + prefix.size() + 1, &offset, sizeof(offset));
        memory[prefix.size() + 5] = 0xc3;
        memory[1024] = 0xe8;
        offset = static_cast<int32_t>(code - site - 5);
        std::memcpy(memory + 1025, &offset, sizeof(offset));
        auto* observers = reinterpret_cast<h5::hooks::ScriptObservers*>(memory + h5::hooks::ScriptObserverDescriptorOffset);
        *observers = {{'H', '5', 'O', 'B', 'S', '0', '0', '1'}, 1, sizeof(*observers),
                      static_cast<uint32_t>(site), static_cast<uint32_t>(target),
                      static_cast<uint32_t>(prefix.size() + 6), static_cast<uint32_t>(prefix.size()), {}, {}};
        if (!h5::hooks::ObserverCodeDigest(code, prefix.size() + 6, observers->codeDigest)) { return 1; }
        DWORD protection = 0;
        if (!VirtualProtect(memory, 4096, PAGE_EXECUTE_READ, &protection)) { return 1; }
        if (h5::hooks::FindScriptObservers(site, target) != observers) { throw std::runtime_error("Valid observer owner rejected"); }
        observers->version = 99;
        if (h5::hooks::FindScriptObservers(site, target)) { throw std::runtime_error("Unknown observer ABI accepted"); }
        observers->version = 1;
        observers->codeDigest[0] ^= 1;
        if (h5::hooks::FindScriptObservers(site, target)) { throw std::runtime_error("Changed code digest accepted"); }
        observers->codeDigest[0] ^= 1;
        observers->originalCallOffset = observers->codeBytes;
        if (h5::hooks::FindScriptObservers(site, target)) { throw std::runtime_error("Out-of-bounds original call accepted"); }
        observers->originalCallOffset = static_cast<uint32_t>(prefix.size());
        auto firstCallback = +[]() {};
        auto secondCallback = +[]() { SetLastError(0); };
        auto* firstSlot = h5::hooks::AddScriptObserver(*observers, firstCallback);
        auto* secondSlot = h5::hooks::AddScriptObserver(*observers, secondCallback);
        if (!firstSlot || !secondSlot || firstSlot == secondSlot ||
            h5::hooks::RemoveScriptObserver(secondSlot, firstCallback) ||
            !h5::hooks::RemoveScriptObserver(firstSlot, firstCallback) || !*secondSlot ||
            h5::hooks::RemoveScriptObserver(firstSlot, firstCallback)) {
            throw std::runtime_error("Observer removal changed another subscriber or accepted wrong ownership");
        }
        for (size_t index = 1; index < h5::hooks::ScriptObserverCapacity; ++index) {
            if (!h5::hooks::AddScriptObserver(*observers, firstCallback)) { return 1; }
        }
        if (h5::hooks::AddScriptObserver(*observers, firstCallback)) { throw std::runtime_error("Observer capacity exceeded"); }
        if (!VirtualProtect(memory, 4096, PAGE_READWRITE, &protection)) { return 1; }
        memory[0] ^= 1;
        if (!VirtualProtect(memory, 4096, PAGE_EXECUTE_READ, &protection)) { return 1; }
        if (h5::hooks::FindScriptObservers(site, target)) { throw std::runtime_error("Altered sealed dispatcher accepted"); }
        VirtualFree(memory, 0, MEM_RELEASE);
        std::cout << "PASS: SHA, process guards, hook catalog, observer ABI/digest/capacity/independent removal\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
