#pragma once
#include "process.hpp"
#include <algorithm>

namespace h5 {

namespace detail {
// Internal transaction only; the caller establishes identity and quiescence.
template<class Write, class Protect, class Flush, class Verify>
inline void CommitGraphicsRepair(const std::array<unsigned char, 42>& original,
    const std::array<unsigned char, 42>& repaired, Write write, Protect protect,
    Flush flush, Verify verify) {
    if (write(repaired) && protect(false) && flush() && verify(repaired)) { return; }
    const bool writable = protect(true);
    const bool restored = writable && write(original);
    const bool protectedAgain = protect(false);
    const bool flushed = flush();
    const bool confirmed = verify(original);
    throw std::runtime_error(restored && protectedAgain && flushed && confirmed ?
        "graphics_proxy_repair_rejected" : "graphics_proxy_rollback_unconfirmed");
}
}

namespace detail {
// Borrowed handle: never closes it. Only guarded startup wrappers may call this.
inline bool RepairGraphicsProxyImage(HANDLE process, const std::filesystem::path& expectedImage,
    HANDLE imageFile, const void* loadedBase) {
    constexpr uintptr_t imageBytes = 0x15000;
    const auto base = reinterpret_cast<uintptr_t>(loadedBase);
    if (!base || base > UINTPTR_MAX - imageBytes) {
        throw std::runtime_error("graphics_proxy_bounds_invalid");
    }
    std::array<wchar_t, 32768> filename{};
    const auto length = GetFinalPathNameByHandleW(imageFile, filename.data(), DWORD(filename.size()), FILE_NAME_NORMALIZED);
    if (!length || length >= filename.size()) { throw std::runtime_error("graphics_proxy_path_unavailable"); }
    std::wstring normalized(filename.data());
    if (normalized.starts_with(L"\\\\?\\")) { normalized.erase(0, 4); }
    if (std::filesystem::canonical(normalized) != std::filesystem::canonical(expectedImage) ||
        Sha256(imageFile) != "5eb152357f99d53397b764384d5cf9a0f6aece733ced30a34186ac57fb15be25") {
        throw std::runtime_error("graphics_proxy_identity_mismatch");
    }
    auto read = [&](uintptr_t address, void* target, size_t size) {
        SIZE_T bytes = 0;
        if (!ReadProcessMemory(process, reinterpret_cast<const void*>(address), target, size, &bytes) || bytes != size) {
            throw std::runtime_error("graphics_proxy_read_failed");
        }
    };
    MEMORY_BASIC_INFORMATION memory{};
    const auto address = base + 0x1383;
    if (VirtualQueryEx(process, reinterpret_cast<void*>(address), &memory, sizeof(memory)) != sizeof(memory) ||
        memory.AllocationBase != loadedBase || memory.Type != MEM_IMAGE || memory.State != MEM_COMMIT ||
        (memory.Protect & PAGE_GUARD) ||
        !(memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) ||
        address < reinterpret_cast<uintptr_t>(memory.BaseAddress) ||
        address - reinterpret_cast<uintptr_t>(memory.BaseAddress) >= memory.RegionSize ||
        42 > memory.RegionSize - (address - reinterpret_cast<uintptr_t>(memory.BaseAddress))) {
        throw std::runtime_error("graphics_proxy_mapping_mismatch");
    }
    IMAGE_DOS_HEADER dos{}; read(base, &dos, sizeof(dos));
    if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 ||
        uintptr_t(dos.e_lfanew) > imageBytes - sizeof(IMAGE_NT_HEADERS32)) {
        throw std::runtime_error("graphics_proxy_header_invalid");
    }
    IMAGE_NT_HEADERS32 header{}; read(base + dos.e_lfanew, &header, sizeof(header));
    if (header.Signature != IMAGE_NT_SIGNATURE || header.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        header.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC || header.OptionalHeader.SizeOfImage != imageBytes) {
        throw std::runtime_error("graphics_proxy_header_mismatch");
    }
    const std::array<unsigned char, 19> entry{0x55,0x8b,0xec,0x56,0x8b,0x75,0x08,0x8b,0x46,0x04,
        0x8b,0x08,0x8b,0x51,0x08,0x57,0x50,0xff,0xd2};
    std::array<unsigned char, 19> actualEntry{}; read(base + 0x1370, actualEntry.data(), actualEntry.size());
    if (actualEntry != entry) { throw std::runtime_error("graphics_proxy_release_abi_mismatch"); }
    std::array<unsigned char, 42> original{0x8b,0xf8,0xc7,0x05,0,0,0,0,0,0,0,0,
        0x8b,0x06,0x8b,0x90,0xdc,0x01,0,0,0x6a,0x01,0x8b,0xce,0xff,0xd2,
        0x8b,0xc7,0x5f,0x5e,0x5d,0xc2,0x04,0x00,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc};
    std::array<unsigned char, 42> actual{}; read(address, actual.data(), actual.size());
    DWORD global = 0; memcpy(&global, actual.data() + 4, sizeof(global));
    if (global != DWORD(base + 0x10f6c) && global != 0x10010f6c) {
        throw std::runtime_error("graphics_proxy_relocation_mismatch");
    }
    memcpy(original.data() + 4, &global, sizeof(global));
    auto repaired = original;
    repaired[0] = 0xeb; repaired[1] = 0x20;
    const std::array<unsigned char, 8> branch{0x85,0xc0,0x75,0xf6,0x8b,0xf8,0xeb,0xd8};
    std::copy(branch.begin(), branch.end(), repaired.begin() + 34);
    if (actual == repaired) { return false; }
    if (actual != original) { throw std::runtime_error("graphics_proxy_signature_mismatch"); }
    DWORD previous = 0;
    if (!VirtualProtectEx(process, reinterpret_cast<void*>(address), original.size(), PAGE_EXECUTE_READWRITE, &previous)) {
        throw std::runtime_error("graphics_proxy_protection_failed");
    }
    auto write = [&](const auto& bytes) {
        SIZE_T written = 0;
        return WriteProcessMemory(process, reinterpret_cast<void*>(address), bytes.data(), bytes.size(), &written) &&
            written == bytes.size();
    };
    auto verify = [&](const auto& expected) {
        std::array<unsigned char, 42> observed{};
        SIZE_T received = 0;
        MEMORY_BASIC_INFORMATION current{};
        return ReadProcessMemory(process, reinterpret_cast<void*>(address), observed.data(), observed.size(), &received) &&
            received == observed.size() && observed == expected &&
            VirtualQueryEx(process, reinterpret_cast<void*>(address), &current, sizeof(current)) == sizeof(current) &&
            current.AllocationBase == loadedBase && current.Type == MEM_IMAGE && current.State == MEM_COMMIT &&
            current.Protect == previous;
    };
    auto protect = [&](bool writable) {
        DWORD ignored = 0;
        return VirtualProtectEx(process, reinterpret_cast<void*>(address), original.size(),
            writable ? PAGE_EXECUTE_READWRITE : previous, &ignored) != FALSE;
    };
    auto flush = [&]() { return FlushInstructionCache(process, reinterpret_cast<void*>(address), original.size()) != FALSE; };
    detail::CommitGraphicsRepair(original, repaired, write, protect, flush, verify);
    return true;
}
} // namespace detail

// Startup-only: all child threads are stopped at the caller's debug event.
// Files and executable allocations remain unchanged. Never resume on failure.
inline bool RepairOwnedGraphicsProxyLifetime(DWORD pid, uint64_t creation,
    const std::filesystem::path& executable, HANDLE imageFile, const void* loadedBase) {
    const auto base = reinterpret_cast<uintptr_t>(loadedBase);
    if (!base || base > UINTPTR_MAX - 0x15000) {
        throw std::runtime_error("graphics_proxy_bounds_invalid");
    }
    const auto process = OpenOwnedProcess(pid, creation, executable,
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION);
    try {
        const bool changed = detail::RepairGraphicsProxyImage(process, executable.parent_path() / L"d3d9.dll", imageFile, loadedBase);
        CloseHandle(process);
        return changed;
    } catch (...) { CloseHandle(process); throw; }
}

} // namespace h5
