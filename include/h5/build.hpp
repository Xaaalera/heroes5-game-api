#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace h5 {
// Hash the caller's file object without reopening its path or changing its cursor.
inline std::string Sha256(HANDLE file) {
    LARGE_INTEGER length{};
    if (!GetFileSizeEx(file, &length) || length.QuadPart < 0 ||
        static_cast<uint64_t>(length.QuadPart) > SIZE_MAX) {
        throw std::runtime_error("Cannot read file size.");
    }
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        throw std::runtime_error("Cannot initialize SHA-256.");
    }
    if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        throw std::runtime_error("Cannot create SHA-256 hash.");
    }
    bool valid = true;
    HANDLE mapping = nullptr;
    const unsigned char* contents = nullptr;
    if (length.QuadPart) {
        mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (mapping) {
            contents = static_cast<const unsigned char*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
        }
        valid = contents != nullptr;
        for (size_t offset = 0; valid && offset < static_cast<size_t>(length.QuadPart);) {
            const auto remaining = static_cast<size_t>(length.QuadPart) - offset;
            const auto bytes = static_cast<ULONG>(remaining > 65536 ? 65536 : remaining);
            valid = BCryptHashData(hash, const_cast<PUCHAR>(contents + offset), bytes, 0) == 0;
            offset += bytes;
        }
    }
    std::array<unsigned char, 32> digest{};
    valid = valid && BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) == 0;
    if (contents) { UnmapViewOfFile(contents); }
    if (mapping) { CloseHandle(mapping); }
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!valid) { throw std::runtime_error("Failed to hash file."); }
    std::string result;
    for (const auto value : digest) {
        result += "0123456789abcdef"[value >> 4];
        result += "0123456789abcdef"[value & 15];
    }
    return result;
}

inline std::string Sha256(const std::filesystem::path& path) {
    const auto file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) { throw std::runtime_error("Cannot read file: " + path.filename().string()); }
    try {
        const auto result = Sha256(file);
        CloseHandle(file);
        return result;
    } catch (...) {
        CloseHandle(file);
        throw;
    }
}

inline constexpr const char* UniverseGraphicsSha256 =
    "5eb152357f99d53397b764384d5cf9a0f6aece733ced30a34186ac57fb15be25";

// The facade digest comes from the caller's sealed SDK build, never a game-folder manifest.
inline void VerifyGraphicsFiles(const std::filesystem::path& directory,
    const std::string& facadeSha256 = {}) {
    const auto graphics = Sha256(directory / L"d3d9.dll");
    if (graphics == UniverseGraphicsSha256) { return; }
    if (facadeSha256.size() != 64 || facadeSha256.find_first_not_of("0123456789abcdef") != std::string::npos ||
        graphics != facadeSha256 || Sha256(directory / L"d3d9.universe.dll") != UniverseGraphicsSha256) {
        throw std::runtime_error("Unsupported Universe graphics chain.");
    }
}

inline void VerifyGame(const std::filesystem::path& executable, const std::string& facadeSha256 = {}) {
    if (_wcsicmp(executable.filename().c_str(), L"H5_Game.exe") != 0) {
        throw std::runtime_error("Select bin/H5_Game.exe from the supported Universe installation.");
    }
    const std::pair<const wchar_t*, const char*> binaries[] = {
        {L"H5_Game.exe", "88c9dc6107b9bced0649924a86360f1c56397ee00de0413f6f2b08f865ed5519"},
        {L"uni.dll", "aa5211151d9e9a8c135e180ff8832908d128ccae08a5145162bcdae4946c18ee"},
        {L"um.dll", "1956c00b371d22a3e1a644394ff3e7159b6ec36d660d5ffa36628fcf63fd0fc6"},
    };
    for (const auto& [name, expected] : binaries) {
        if (Sha256(executable.parent_path() / name) != expected) {
            throw std::runtime_error("Unsupported Universe build. No game files were changed.");
        }
    }
    VerifyGraphicsFiles(executable.parent_path(), facadeSha256);
}

}
