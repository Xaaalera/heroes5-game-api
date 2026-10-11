#ifndef H5_SOURCE_STRENGTH_HPP
#define H5_SOURCE_STRENGTH_HPP
#include <windows.h>
#include <cstdint>
#include <cstring>
#include "hooks.hpp"
namespace h5::army {
struct SourceStrengthInput {
    int baseValue, quantity, deltas[6], modifierA, modifierB;
    uint32_t initiativeBits[3]; // Low 10 bytes are the getter's x87 value.
    int baseInitiative;
    bool shooterBonus, modifierPenalty;
    float coefficients[11];
    double baseScale, one;
};
static_assert(sizeof(SourceStrengthInput) == 120, "Source strength primitive layout");

struct SourceStrengthSample {
    uint32_t type = 0, phase = 0, controlWord = 0, unsupported = 0;
    SourceStrengthInput input{};
    int64_t calculated = 0;
};
// Pinned Universe primitive getters, not final battle placement. Caller must
// authorize hidden input access and invoke on the game thread.
inline void ReadSourceStrength(const uint32_t* source, const uint32_t* context, uint32_t phase, SourceStrengthSample& sample) {
    sample = {}; sample.phase = phase;
    auto& input = sample.input;
    uint32_t controlWord = 0;
    auto& unsupported = sample.unsupported;
    auto& type = sample.type;
    __try {
        __asm { fnstcw word ptr controlWord }
        type = source[0x1c / 4];
        const auto* definition = reinterpret_cast<const int* (__fastcall*)(uint32_t)>(0x89fbf0)(type);
        if (definition == nullptr || static_cast<int>(source[0x20 / 4]) <= 0
            || *reinterpret_cast<const unsigned char*>(0x10c5178) == 0) {
            unsupported = 1;
        } else {
            input.baseValue = definition[0x84 / 4];
            input.quantity = static_cast<int>(source[0x20 / 4]);
            using Stat = int (__thiscall*)(const void*, const void*, uint32_t);
            const auto health = reinterpret_cast<Stat>(0x4bc470);
            const auto maximum = reinterpret_cast<Stat>(0x4bbfb0);
            const auto minimum = reinterpret_cast<Stat>(0x4bc020);
            const auto attack = reinterpret_cast<Stat>(0x4bb170);
            const auto defence = reinterpret_cast<Stat>(0x4bb2a0);
            input.deltas[0] = static_cast<int>(static_cast<uint32_t>(health(source, context, phase))
                - static_cast<uint32_t>(definition[0x64 / 4]));
            input.deltas[1] = static_cast<int>(static_cast<uint32_t>(maximum(source, context, phase))
                + static_cast<uint32_t>(minimum(source, context, phase))
                - static_cast<uint32_t>(definition[0x54 / 4]) - static_cast<uint32_t>(definition[0x50 / 4]));
            input.deltas[2] = static_cast<int>(static_cast<uint32_t>(attack(source, nullptr, phase))
                + static_cast<uint32_t>(defence(source, nullptr, phase))
                - static_cast<uint32_t>(definition[0x44 / 4]) - static_cast<uint32_t>(definition[0x48 / 4]));
            const auto* extras = reinterpret_cast<const signed char*>(source) + 0x38;
            input.deltas[3] = extras[8]; input.deltas[4] = extras[9]; input.deltas[5] = extras[12];
            input.modifierA = reinterpret_cast<int (__thiscall*)(const void*, uint32_t)>(0x4bb3c0)(source, phase);
            input.modifierB = reinterpret_cast<Stat>(0x4bb3f0)(source, context, phase);
            if (context != nullptr) {
                const auto* methods = reinterpret_cast<const uint32_t*>(context[0]);
                if (definition[0x4c / 4] > 0) {
                    input.shooterBonus = reinterpret_cast<int (__thiscall*)(const void*)>(methods[0x324 / 4])(context) > 0;
                }
                const auto owner = reinterpret_cast<const uint32_t* (__thiscall*)(const void*)>(methods[0x54 / 4])(context);
                if (owner != nullptr) {
                    const auto* ownerMethods = reinterpret_cast<const uint32_t*>(owner[0]);
                    input.modifierPenalty = reinterpret_cast<int (__thiscall*)(const void*, int)>(ownerMethods[0x34 / 4])(owner, 4) > 0;
                }
            }
            input.baseInitiative = definition[0x5c / 4];
            void* initiative = input.initiativeBits;
            const uintptr_t getter = 0x4bc190;
            __asm {
                push phase
                push context
                mov ecx, source
                call getter
                mov eax, initiative
                fstp tbyte ptr [eax]
            }
            memcpy(input.coefficients, reinterpret_cast<const void*>(0x10c5480 + 0x250), sizeof(input.coefficients));
            memcpy(&input.baseScale, reinterpret_cast<const void*>(0xe0d608), sizeof(double));
            memcpy(&input.one, reinterpret_cast<const void*>(0xe12e00), sizeof(double));
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { unsupported = 2; }
    sample.controlWord = controlWord;
}
inline void ReadSourceStrengthFrame(const h5::hooks::X86ObserverFrame& frame, SourceStrengthSample& sample) {
    sample = {};
    __try {
        const auto* source = reinterpret_cast<const uint32_t*>(frame.esi);
        const auto* context = *reinterpret_cast<const uint32_t* const*>(frame.savedEsp + 16);
        const auto phase = *reinterpret_cast<const uint32_t*>(frame.ebp + 8);
        ReadSourceStrength(source, context, phase, sample);
    } __except (EXCEPTION_EXECUTE_HANDLER) { sample.unsupported = 2; }
}
}
#endif
