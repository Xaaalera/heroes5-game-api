#include <h5/build.hpp>
#include <h5/hooks.hpp>
#include <h5/process.hpp>
#include <iostream>

int main() {
    try {
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
        std::cout << "PASS: SHA, corrupt correctly-named build, process creation/path/handle cleanup, hook catalog\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
