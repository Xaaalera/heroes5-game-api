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
inline std::string Sha256(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { throw std::runtime_error("Cannot read file: " + path.filename().string()); }
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        throw std::runtime_error("Cannot initialize SHA-256.");
    }
    if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        throw std::runtime_error("Cannot create SHA-256 hash.");
    }
    std::array<unsigned char, 65536> buffer{};
    bool valid = true;
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        if (file.gcount() && BCryptHashData(hash, buffer.data(), static_cast<ULONG>(file.gcount()), 0) != 0) {
            valid = false;
            break;
        }
    }
    std::array<unsigned char, 32> digest{};
    valid = valid && file.eof() && BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) == 0;
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

inline void VerifyGame(const std::filesystem::path& executable) {
    if (_wcsicmp(executable.filename().c_str(), L"H5_Game.exe") != 0) {
        throw std::runtime_error("Select bin/H5_Game.exe from the supported Universe installation.");
    }
    const std::pair<const wchar_t*, const char*> binaries[] = {
        {L"H5_Game.exe", "88c9dc6107b9bced0649924a86360f1c56397ee00de0413f6f2b08f865ed5519"},
        {L"uni.dll", "aa5211151d9e9a8c135e180ff8832908d128ccae08a5145162bcdae4946c18ee"},
        {L"um.dll", "1956c00b371d22a3e1a644394ff3e7159b6ec36d660d5ffa36628fcf63fd0fc6"},
        {L"d3d9.dll", "5eb152357f99d53397b764384d5cf9a0f6aece733ced30a34186ac57fb15be25"},
    };
    for (const auto& [name, expected] : binaries) {
        if (Sha256(executable.parent_path() / name) != expected) {
            throw std::runtime_error("Unsupported Universe build. No game files were changed.");
        }
    }
}

}
