#pragma once
#include "process.hpp"

namespace h5 {

struct ImagePlacementContract {
    std::filesystem::path file;
    std::string sha256;
    uintptr_t preferredBase;
    uint32_t imageBytes;
    uint32_t requiredDataOffset;
    uint32_t requiredDataBytes;
};

// Caller must stop its own loader at LOAD_DLL_DEBUG_EVENT, retaining hFile.
// No process discovery, memory writes, event continuation or handle transfer.
inline void VerifyOwnedImagePlacement(DWORD pid, uint64_t creation,
    const std::filesystem::path& executable, HANDLE imageFile, const void* loadedBase,
    const ImagePlacementContract& expected, const std::string& facadeSha256 = {}) {
    if (!expected.preferredBase || !expected.imageBytes || !expected.requiredDataBytes ||
        expected.preferredBase > UINTPTR_MAX - expected.imageBytes ||
        expected.requiredDataOffset >= expected.imageBytes ||
        expected.requiredDataBytes > expected.imageBytes - expected.requiredDataOffset) {
        throw std::runtime_error("image_contract_bounds_invalid");
    }
    const auto process = OpenOwnedProcess(pid, creation, executable,
        PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, facadeSha256);
    try {
        std::array<wchar_t, 32768> filename{};
        const auto length = GetFinalPathNameByHandleW(imageFile, filename.data(), DWORD(filename.size()), FILE_NAME_NORMALIZED);
        if (!length || length >= filename.size()) { throw std::runtime_error("loaded_image_path_unavailable"); }
        std::wstring normalized(filename.data());
        if (normalized.starts_with(L"\\\\?\\")) { normalized.erase(0, 4); }
        const auto actualFile = std::filesystem::canonical(normalized);
        if (actualFile != std::filesystem::canonical(expected.file) || Sha256(imageFile) != expected.sha256) {
            throw std::runtime_error("loaded_image_identity_mismatch");
        }
        MEMORY_BASIC_INFORMATION memory{};
        if (reinterpret_cast<uintptr_t>(loadedBase) != expected.preferredBase ||
            VirtualQueryEx(process, loadedBase, &memory, sizeof(memory)) != sizeof(memory) ||
            memory.AllocationBase != loadedBase || memory.Type != MEM_IMAGE || memory.State != MEM_COMMIT) {
            throw std::runtime_error("loaded_image_placement_mismatch");
        }
        IMAGE_DOS_HEADER dos{};
        SIZE_T read = 0;
        if (!ReadProcessMemory(process, loadedBase, &dos, sizeof(dos), &read) || read != sizeof(dos) ||
            dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 ||
            uint32_t(dos.e_lfanew) > expected.imageBytes ||
            sizeof(IMAGE_NT_HEADERS32) > expected.imageBytes - uint32_t(dos.e_lfanew)) {
            throw std::runtime_error("loaded_image_header_invalid");
        }
        IMAGE_NT_HEADERS32 header{};
        if (!ReadProcessMemory(process, reinterpret_cast<const BYTE*>(loadedBase) + dos.e_lfanew,
            &header, sizeof(header), &read) || read != sizeof(header) || header.Signature != IMAGE_NT_SIGNATURE ||
            header.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
            header.OptionalHeader.ImageBase != expected.preferredBase || header.OptionalHeader.SizeOfImage != expected.imageBytes) {
            throw std::runtime_error("loaded_image_header_mismatch");
        }
        const auto address = reinterpret_cast<uintptr_t>(loadedBase) + expected.requiredDataOffset;
        MEMORY_BASIC_INFORMATION data{};
        if (VirtualQueryEx(process, reinterpret_cast<void*>(address), &data, sizeof(data)) != sizeof(data) ||
            data.AllocationBase != loadedBase || data.Type != MEM_IMAGE || data.State != MEM_COMMIT ||
            (data.Protect & PAGE_GUARD) ||
            !(data.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) ||
            address < reinterpret_cast<uintptr_t>(data.BaseAddress) ||
            address - reinterpret_cast<uintptr_t>(data.BaseAddress) >= data.RegionSize ||
            expected.requiredDataBytes > data.RegionSize - (address - reinterpret_cast<uintptr_t>(data.BaseAddress))) {
            throw std::runtime_error("loaded_image_data_unavailable");
        }
        std::array<BYTE, 4096> contents{};
        for (size_t offset = 0; offset < expected.requiredDataBytes;) {
            const auto remaining = expected.requiredDataBytes - offset;
            const auto bytes = remaining > contents.size() ? contents.size() : remaining;
            if (!ReadProcessMemory(process, reinterpret_cast<const void*>(address + offset),
                contents.data(), bytes, &read) || read != bytes) {
                throw std::runtime_error("loaded_image_data_unreadable");
            }
            offset += bytes;
        }
        CloseHandle(process);
    } catch (...) {
        CloseHandle(process);
        throw;
    }
}

} // namespace h5
