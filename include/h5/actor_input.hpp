#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include "hooks.hpp"
namespace h5::army {
// Pinned Universe x86, game/window thread only. Original CCreature::GetName:
// definition -> SCreatureVisual -> localized text resource. The returned game
// string is borrowed only during this call; consumers retain a bounded copy.
inline bool ReadCreatureDisplayName(uint32_t type, wchar_t* output, uint32_t capacity) {
    if (!output || capacity < 2 || capacity > 1024) { return false; }
    output[0] = 0;
    if (!type || type >= 512) { return false; }
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax, floatingState }
    __asm { fxsave [eax] }
    bool valid = false;
    __try {
        const auto definition = reinterpret_cast<unsigned char*(__fastcall*)(uint32_t)>(0x89fbf0)(type);
        const auto visual = definition ? reinterpret_cast<unsigned char*(__fastcall*)(void*)>(0x4bdad0)(definition + 0xf4) : nullptr;
        const auto text = visual ? reinterpret_cast<const uintptr_t*(__thiscall*)(void*)>(0x4c7430)(visual + 0x44) : nullptr;
        if (text && text[0] && text[1] > text[0] && (text[1] - text[0]) % sizeof(wchar_t) == 0) {
            const uintptr_t length = (text[1] - text[0]) / sizeof(wchar_t);
            if (length < capacity) {
                memcpy(output, reinterpret_cast<const void*>(text[0]), length * sizeof(wchar_t));
                output[length] = 0; valid = true;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { output[0] = 0; }
    __asm { lea eax, floatingState }
    __asm { fxrstor [eax] }
    return valid;
}

struct ActorInput {
    uint32_t world, army, source, type, quantity, damage;
    int32_t attack, defence, health;
    float initiative, atb;
    int32_t morale, luck, speed;
    uint32_t category, eligible, skipSpeed, selected, retaliationBonus, controlWord;
    uint32_t aggregateIdentity, unsupported, specialCategory, attackGeneration;
    // rangedCapability: bit0 can attack at range, bit1 excluded from the native capability list.
    uint32_t rangedCapability, ignoreWhenNoRanged, armySize;
    int32_t counterMaximum[3], counterCurrent[3];
    uint32_t healthTransformFlags, healthScaleBits[2];
};
static_assert(sizeof(ActorInput) == 144, "Primitive input trace layout");
using ActorCategorySelector = unsigned (*)(bool, bool, bool, bool, bool);
// Primitive getters for the pinned build; caller authorizes hidden inputs.
inline void ReadActorInputValues(const uint32_t* actor, uint32_t aggregateIdentity,
    uint32_t damage, uint32_t selected, uint32_t specialCategory, uint32_t generation,
    ActorInput& input, ActorCategorySelector categorySelector) {
    input.damage = damage;
    input.aggregateIdentity = aggregateIdentity;
    input.specialCategory = specialCategory & 255;
    input.attackGeneration = generation;
    uint16_t controlWord;
    __asm fnstcw controlWord
    input.controlWord = controlWord;
    if (actor == nullptr || actor[0] != 0xe4fba4) {
        input.unsupported = 1;
    } else {
        const auto* methods = reinterpret_cast<const uint32_t*>(actor[0]);
        const auto integer = [&](uint32_t offset) {
            return reinterpret_cast<int32_t (__thiscall*)(const void*)>(methods[offset / 4])(actor);
        };
        const auto floating = [&](uint32_t offset) {
            return reinterpret_cast<float (__thiscall*)(const void*)>(methods[offset / 4])(actor);
        };
        const auto boolean = [&](uint32_t offset) {
            return reinterpret_cast<bool (__thiscall*)(const void*)>(methods[offset / 4])(actor);
        };
        const auto flag = [&](uint32_t value) {
            return reinterpret_cast<bool (__thiscall*)(const void*, uint32_t)>(methods[0x28c / 4])(actor, value);
        };
        const auto status = [&](uint32_t value) {
            return reinterpret_cast<uint32_t (__thiscall*)(const void*, uint32_t)>(methods[0x28 / 4])(actor, value);
        };
        input.world = integer(8);
        if (input.world == 0 || *reinterpret_cast<const uint32_t*>(input.world) != 0xe79d3c) input.unsupported |= 128;
        input.army = integer(0xc);
        input.attack = integer(0x198);
        input.defence = integer(0x19c);
        input.health = reinterpret_cast<int32_t (__thiscall*)(const void*, uint32_t)>(methods[0x1dc / 4])(
            actor, integer(0x20));
        input.initiative = floating(0x1bc);
        const auto* timer = reinterpret_cast<const float*>(integer(0x184));
        if (timer != nullptr) input.atb = timer[0x1c / 4];
        else input.unsupported |= 2;
        input.morale = integer(0x1ac);
        input.luck = integer(0x1b0);
        input.speed = integer(0x48);
        const bool shooter = boolean(0x1e0);
        input.rangedCapability = shooter || reinterpret_cast<int (__fastcall*)(const void*)>(0xbfa4f0)(actor) > 0;
        if (boolean(0x5c) || integer(0x4c) == 1) { input.rangedCapability |= 2u; }
        input.ignoreWhenNoRanged = flag(0x37) && flag(5);
        const auto* army = reinterpret_cast<const uint32_t*>(input.army);
        if (army != nullptr && army[0] == 0xe17bcc && army[0x2c / 4] >= army[0x28 / 4]
            && (army[0x2c / 4] - army[0x28 / 4]) % 4 == 0) {
            input.armySize = (army[0x2c / 4] - army[0x28 / 4]) / 4;
        } else input.unsupported |= 256;
        bool excludedCategory = false;
        const auto* role = reinterpret_cast<const uint32_t*>(integer(0x7c));
        if (role != nullptr) {
            const auto* roleMethods = reinterpret_cast<const uint32_t*>(role[0]);
            const auto* definition = reinterpret_cast<const uint32_t* (__thiscall*)(const void*)>(roleMethods[0])(role);
            excludedCategory = definition != nullptr && definition[0x1c / 4] == 2;
        }
        input.category = categorySelector(excludedCategory, shooter, !shooter && boolean(0x190),
                                                        input.specialCategory != 0, input.ignoreWhenNoRanged != 0);
        input.skipSpeed = (flag(1) ? 1u : 0u) | ((flag(4) && shooter) ? 2u : 0u) | (shooter ? 4u : 0u);
        input.selected = reinterpret_cast<uint32_t>(actor) == selected;
        input.retaliationBonus = boolean(0x1e4) && !flag(0x22);
        const auto* source = reinterpret_cast<const uint32_t*>(integer(0x6c));
        if (source != nullptr) {
            const auto* sourceMethods = reinterpret_cast<const uint32_t*>(source[0]);
            const auto* descriptor = reinterpret_cast<const uint32_t* (__thiscall*)(const void*)>(sourceMethods[0])(source);
            if (descriptor != nullptr) {
                input.source = reinterpret_cast<uint32_t>(descriptor);
                input.type = descriptor[0x1c / 4];
                input.quantity = descriptor[0x20 / 4];
                input.counterMaximum[1] = reinterpret_cast<int (__thiscall*)(const void*)>(0x4bab70)(descriptor);
                input.counterMaximum[2] = reinterpret_cast<int (__thiscall*)(const void*)>(0x4bab80)(descriptor);
                input.counterCurrent[1] = reinterpret_cast<int (__thiscall*)(const void*)>(sourceMethods[0x2c / 4])(source);
                input.counterCurrent[2] = reinterpret_cast<int (__thiscall*)(const void*)>(sourceMethods[0x34 / 4])(source);
            } else input.unsupported |= 4;
            if (integer(0) == 1) {
                const auto* secondary = reinterpret_cast<const int32_t*>(source[1]);
                const void* eligible = reinterpret_cast<const unsigned char*>(source) + 4 + secondary[2];
                const auto* eligibleMethods = *reinterpret_cast<const uint32_t* const*>(eligible);
                input.eligible = !reinterpret_cast<bool (__thiscall*)(const void*)>(eligibleMethods[0x58 / 4])(eligible);
            }
        } else input.unsupported |= 4;
        if ((input.skipSpeed & 1u) && !shooter) input.unsupported |= 8;
        input.counterMaximum[0] = integer(0x238);
        input.counterCurrent[0] = integer(0x234);
        if (reinterpret_cast<float (__fastcall*)(const void*)>(0xbfb3d0)(actor) != 0) input.unsupported |= 32;
        const uint32_t effects[] = {0x28u, 0x22u, 0x1du, 0x13du, 0x13u, 0xc9u, 0x14au, 0x150u, 0x9fu, 0x11u};
        for (const uint32_t effect : effects) {
            if (status(effect)) input.unsupported |= 64;
        }
        if (flag(0x28)) {
            const bool alternative = status(0x100) != 0;
            input.healthTransformFlags = alternative ? 3 : 1;
            const double scale = *reinterpret_cast<const double*>(alternative ? 0xe72bf8 : 0xe87cf8);
            memcpy(input.healthScaleBits, &scale, sizeof(scale));
        }
    }
}
inline void ReadActorInputFrame(const h5::hooks::X86ObserverFrame& frame,
    uint32_t generation, ActorInput& input, ActorCategorySelector categorySelector) {
    input = {};
    __try {
        const auto* stack = reinterpret_cast<const uint32_t*>(frame.savedEsp + 4);
        ReadActorInputValues(reinterpret_cast<const uint32_t*>(frame.esi), frame.edi,
            frame.ebp, stack[10], stack[12], generation, input, categorySelector);
    } __except (EXCEPTION_EXECUTE_HANDLER) { input.unsupported |= 0x80000000u; }
}
struct HeroCorrectionInput {
    uint32_t aggregateIdentity, inputCount;
    int32_t firstScalar, secondScalar;
    uint32_t controlWord, attackGeneration;
};
struct AuxiliaryPowerInput {
    uint32_t aggregateIdentity, inputCount;
    int32_t value;
    uint32_t caller, controlWord, attackGeneration;
};
static_assert(sizeof(HeroCorrectionInput) == 24);
static_assert(sizeof(AuxiliaryPowerInput) == 24);
// Raw combat protection state, not a completed placement aggregate. Tags name
// the getter and its fractional output store in the pinned executable.
inline constexpr uint32_t ActorProtectionCapacity = 0xa5b780;
inline constexpr uint32_t ActorProtectionFraction = 0xa5b89f;
inline constexpr uint32_t ActorRunicDescriptorCount = 0xbfb3db;
inline constexpr uint32_t ActorRunicAvailability = 0xbfb430;
inline constexpr uint32_t ActorRunicInputCapacity = 257;
// Read per-descriptor properties; placement scoring belongs to the caller.
inline bool ReadActorRunicInputs(const uint32_t* actor, const ActorInput& input,
    uint32_t inputCount, AuxiliaryPowerInput (&outputs)[ActorRunicInputCapacity], uint32_t& outputCount) {
    outputCount = 0;
    __try {
        if (input.unsupported & ~32u) { return true; }
        if (!actor || actor[0] != 0xe4fba4) { return false; }
        const auto* methods = reinterpret_cast<const uint32_t*>(actor[0]);
        const auto* descriptors = reinterpret_cast<const uint32_t* (__thiscall*)(const void*)>(methods[0x248 / 4])(actor);
        if (!descriptors) { return false; }
        const uint32_t begin = descriptors[0], end = descriptors[1];
        if ((!begin && end) || ((begin | end) & 3u) || end < begin || (end - begin) % 20 ||
            (end - begin) / 20 > ActorRunicInputCapacity - 1) { return false; }
        outputs[0] = {input.aggregateIdentity, inputCount, 0, ActorRunicDescriptorCount,
            input.controlWord, input.attackGeneration};
        outputCount = 1;
        for (uint32_t address = begin; address < end; address += 20) {
            const auto* descriptor = reinterpret_cast<const uint32_t*>(address);
            if (!reinterpret_cast<bool (__fastcall*)(uint32_t)>(0xa345b0)(descriptor[1])) { continue; }
            const int32_t state = reinterpret_cast<int32_t (__fastcall*)(const void*, const void*)>(0x41a040)(descriptor, actor);
            outputs[outputCount++] = {input.aggregateIdentity, inputCount, state, ActorRunicAvailability,
                input.controlWord, input.attackGeneration};
            ++outputs[0].value;
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { outputCount = 0; return false; }
}
inline bool ReadActorProtectionInputs(const uint32_t* actor, const ActorInput& input,
    uint32_t inputCount, AuxiliaryPowerInput (&outputs)[2], uint32_t& outputCount) {
    outputCount = 0;
    __try {
        if (!actor || actor[0] != 0xe4fba4 || (input.unsupported & ~32u)) { return true; }
        int32_t capacity = 0;
        float fraction = 0;
        reinterpret_cast<void (__fastcall*)(const void*, float*, int32_t*)>(0xa5b780)(
            actor, &fraction, &capacity);
        if (capacity <= 0) { return true; }
        if (!(fraction >= 0 && fraction <= 1)) { return false; }
        int32_t fractionBits;
        memcpy(&fractionBits, &fraction, sizeof(fraction));
        outputs[0] = {input.aggregateIdentity, inputCount, capacity,
            ActorProtectionCapacity, input.controlWord, input.attackGeneration};
        outputs[1] = {input.aggregateIdentity, inputCount, fractionBits,
            ActorProtectionFraction, input.controlWord, input.attackGeneration};
        outputCount = 2;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
enum class AuxiliaryReadResult { Captured, UnrelatedCaller, InvalidFrame };
struct NativePolicyAggregates {
    uint32_t side, defensive;
    uint32_t aggregates[2][18];
};
// Verification only: never use the engine's completed aggregates as inputs.
inline bool ReadNativePolicyAggregates(const h5::hooks::X86ObserverFrame& frame,
    NativePolicyAggregates& result) {
    result = {};
    __try {
        const auto* policy = reinterpret_cast<const unsigned char*>(frame.ecx);
        if (!policy) { return false; }
        const uint32_t side = *reinterpret_cast<const uint32_t*>(policy) & 255u;
        const uint32_t defensive = policy[0xd0];
        if (side > 1 || defensive > 1) { return false; }
        memcpy(result.aggregates, policy + 0x38, sizeof(result.aggregates));
        result.side = side; result.defensive = defensive;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { result = {}; return false; }
}

// Verification-only context: engine capabilities never become forecast inputs.
inline bool ReadNativePolicyCapabilities(const h5::hooks::X86ObserverFrame& frame, uint32_t* words,
    uint32_t* armyCounts = nullptr) {
    __try {
        const auto* policy = reinterpret_cast<const uint32_t*>(frame.ecx);
        if (!policy || !policy[0x30 / 4]) { return false; }
        auto* army = reinterpret_cast<void*>(policy[0x30 / 4]);
        const auto* methods = *reinterpret_cast<const uint32_t* const*>(army);
        auto* world = reinterpret_cast<void* (__thiscall*)(void*)>(methods[0])(army);
        if (!world) { return false; }
        const auto* worldMethods = *reinterpret_cast<const uint32_t* const*>(world);
        auto* other = reinterpret_cast<void* (__thiscall*)(void*, void*)>(worldMethods[0xe0 / 4])(world, army);
        if (!other) { return false; }
        using Power = int (__thiscall*)(const void*, const void*);
        const auto power = reinterpret_cast<Power>(0xbf9d30);
        const void* first = reinterpret_cast<const unsigned char*>(policy) + 0x38;
        const void* second = reinterpret_cast<const unsigned char*>(policy) + 0x80;
        const int firstRanged = power(first, second), secondRanged = power(second, first);
        using Capability = bool (__thiscall*)(void*, bool);
        const auto capability = reinterpret_cast<Capability>(0xa2f710);
        words[0] = reinterpret_cast<uint32_t>(army);
        words[1] = reinterpret_cast<uint32_t>(other);
        words[2] = capability(army, firstRanged == 0);
        words[3] = capability(other, secondRanged == 0);
        words[4] = static_cast<uint32_t>(firstRanged);
        words[5] = static_cast<uint32_t>(secondRanged);
        if (armyCounts) {
            void* armies[2]{army, other};
            for (uint32_t side = 0; side < 2; ++side) {
                const auto* object = reinterpret_cast<const uint32_t*>(armies[side]);
                if (object[0] != 0xe17bcc) { return false; }
                const uint32_t begin = object[0x28 / 4], end = object[0x2c / 4];
                if ((!begin && end) || ((begin | end) & 3u) || end < begin ||
                    (end - begin) % 4 || end - begin > 4096 * 4) {
                    return false;
                }
                armyCounts[side] = (end - begin) / 4;
            }
            armyCounts[2] = 1; // Counts belong to this synchronous verification call.
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

struct NativeCapabilityInput {
    uint32_t army, rawUnitCount, noRanged, classRejected;
};
// Observe the input before the original capability CALL, not a later policy snapshot.
inline bool ReadNativeCapabilityCall(const h5::hooks::X86ObserverFrame& frame, NativeCapabilityInput& result) {
    result = {};
    __try {
        const auto* army = reinterpret_cast<const uint32_t*>(frame.ecx);
        if (!army || army[0] != 0xe17bcc) { return false; }
        const uint32_t begin = army[0x28 / 4], end = army[0x2c / 4];
        if ((!begin && end) || ((begin | end) & 3u) || end < begin ||
            (end - begin) % 4 || end - begin > 4096 * 4) { return false; }
        const auto* metadata = reinterpret_cast<const int32_t*>(army[1]);
        if (!metadata || metadata[1] < -4096 || metadata[1] > 4096) { return false; }
        const auto rejected = *reinterpret_cast<const unsigned char*>(frame.ecx + metadata[1] + 0xb) & 0x80;
        const auto noRanged = *reinterpret_cast<const unsigned char*>(frame.savedEsp + 4);
        if (noRanged > 1) { return false; }
        result = {frame.ecx, (end - begin) / 4, noRanged, rejected ? 1u : 0u};
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { result = {}; return false; }
}

// Call synchronously during an owned engine callback; retain identities only.
inline bool ReadActorArmySides(uint32_t worldIdentity, uint32_t (&armies)[2]) {
    armies[0] = armies[1] = 0;
    __try {
        const auto* world = reinterpret_cast<const uint32_t*>(worldIdentity);
        if (!world || world[0] != 0xe79d3c) { return false; }
        const uint32_t first = world[0x8c / 4], second = world[0x90 / 4];
        if (!first || !second || first == second ||
            *reinterpret_cast<const uint32_t*>(first) != 0xe17bcc ||
            *reinterpret_cast<const uint32_t*>(second) != 0xe17bcc) { return false; }
        armies[0] = first; armies[1] = second;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

inline void ReadHeroCorrectionFrame(const h5::hooks::X86ObserverFrame& frame,
    uint32_t inputCount, uint32_t generation, HeroCorrectionInput& input) {
    unsigned short control = 0;
    __asm { fnstcw control }
    input = {frame.edi, inputCount, static_cast<int32_t>(frame.ebx),
        static_cast<int32_t>(frame.eax), control, generation};
}
inline AuxiliaryReadResult ReadAuxiliaryPowerFrame(const h5::hooks::X86ObserverFrame& frame,
    uint32_t inputCount, uint32_t generation, AuxiliaryPowerInput& input) {
    input = {};
    __try {
        const auto* stack = reinterpret_cast<const uint32_t*>(frame.savedEsp + 4);
        const auto caller = stack[7];
        if (caller != 0xbfbb90 && caller != 0xbfbbc3 && caller != 0xbfc194) {
            return AuxiliaryReadResult::UnrelatedCaller;
        }
        unsigned short control = 0;
        __asm { fnstcw control }
        input = {stack[0], inputCount, static_cast<int32_t>(stack[3]), caller, control, generation};
        return AuxiliaryReadResult::Captured;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return AuxiliaryReadResult::InvalidFrame; }
}
// Verification only. Copied Lua binding results, never prediction inputs.
struct CombatRosterSample {
    uint32_t side, kind, count;
};
inline bool ReadCombatRosterCount(const h5::hooks::X86ObserverFrame& frame, CombatRosterSample& result) {
    result = {};
    __try {
        const auto* stack = reinterpret_cast<const uint32_t*>(frame.savedEsp + 4);
        const uint32_t begin = stack[0x14 / 4], end = stack[0x18 / 4];
        const uint32_t argumentsBegin = stack[0x20 / 4], argumentsEnd = stack[0x24 / 4];
        if (!argumentsBegin || argumentsEnd < argumentsBegin || argumentsEnd - argumentsBegin != 56
            || end < begin || (end - begin) % 4 || (end - begin) / 4 > 7) { return false; }
        const auto* arguments = reinterpret_cast<const uint32_t*>(argumentsBegin);
        result = {arguments[0], arguments[7], (end - begin) / 4};
        return result.side <= 1 && result.kind >= 1 && result.kind <= 4 && result.kind == frame.ebp;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
struct CombatUnitSample {
    uint32_t nameLength;
    char name[96];
    int32_t x, y;
    uint32_t quantity;
};
// Initial committed facts only. Calling this from an ordinary placement render
// would expose hidden inputs; the admitted DeploymentCommitted site is required.
struct DeploymentStackSample {
    uint32_t actor, type, quantity, footprint;
    int32_t x, y;
};
struct CommittedDeploymentSample {
    uint32_t world, armies[2], counts[2], failures, overflow;
    DeploymentStackSample stacks[2][7];
};
inline bool ReadCommittedDeployment(const h5::hooks::X86ObserverFrame& frame,
    CommittedDeploymentSample& result) {
    result = {};
    uint32_t stage = 1;
    __try {
        const auto* army = reinterpret_cast<const uint32_t*>(frame.esi);
        if (!army || army[0] != 0xe17bcc) { result.failures = 1; return false; }
        stage = 2;
        const auto* armyMethods = reinterpret_cast<const uint32_t*>(army[0]);
        const auto* world = reinterpret_cast<const uint32_t* (__thiscall*)(const void*)>(armyMethods[0])(army);
        if (!world || world[0] != 0xe79d3c) { result.failures = 2; return false; }
        const auto* worldMethods = reinterpret_cast<const uint32_t*>(world[0]);
        result.world = reinterpret_cast<uint32_t>(world);
        for (uint32_t side = 0; side < 2; ++side) {
            stage = 3 + side;
            const auto* sideArmy = reinterpret_cast<const uint32_t* (__thiscall*)(const void*, bool)>(
                worldMethods[0x84 / 4])(world, side == 0);
            if (!sideArmy || sideArmy[0] != 0xe17bcc ||
                *reinterpret_cast<const unsigned char*>(reinterpret_cast<uintptr_t>(sideArmy) + 0x64)) { result.failures = stage; return false; }
            result.armies[side] = reinterpret_cast<uint32_t>(sideArmy);
        }
        if (result.armies[0] == result.armies[1] ||
            (result.armies[0] != frame.esi && result.armies[1] != frame.esi)) { result.failures = 5; return false; }
        stage = 6;
        // Same adjusted world interface and argument as GetCreatureNumber.
        const auto worldOffset = reinterpret_cast<const int32_t*>(world[1])[2];
        const void* worldInterface = reinterpret_cast<const unsigned char*>(world) + 4 + worldOffset;
        const auto* healthMethods = *reinterpret_cast<const uint32_t* const*>(worldInterface);
        const uint32_t healthContext = reinterpret_cast<uint32_t (__thiscall*)(const void*)>(
            healthMethods[0x40 / 4])(worldInterface);
        for (uint32_t side = 0; side < 2; ++side) {
            stage = 7 + side;
            const auto* sideArmy = reinterpret_cast<const uint32_t*>(result.armies[side]);
            const uint32_t begin = sideArmy[0x28 / 4], end = sideArmy[0x2c / 4];
            if (end < begin || (end - begin) % 4 || (end - begin) / 4 > 64 || (end != begin && !begin)) { result.failures = stage; return false; }
            for (uint32_t index = 0; index < (end - begin) / 4; ++index) {
                const auto* actor = reinterpret_cast<const uint32_t* const*>(begin)[index];
                // The exact active-reference filter used by GetUnits enumeration.
                if (!reinterpret_cast<bool (__thiscall*)(const void*)>(0xa6d490)(actor)) { continue; }
                const auto* methods = reinterpret_cast<const uint32_t*>(actor[0]);
                const auto* source = reinterpret_cast<const uint32_t* (__thiscall*)(const void*)>(methods[0x6c / 4])(actor);
                if (!source) { continue; } // Heroes, machines and other non-creature units.
                const auto* sourceMethods = reinterpret_cast<const uint32_t*>(source[0]);
                const auto* descriptor = reinterpret_cast<const uint32_t* (__thiscall*)(const void*)>(sourceMethods[0])(source);
                if (!descriptor) { result.failures = 9; continue; }
                if (result.counts[side] == 7) { result.overflow = 1; continue; }
                DeploymentStackSample sample{};
                sample.actor = reinterpret_cast<uint32_t>(actor); sample.type = descriptor[0x1c / 4];
                if (!sample.type || sample.type > 1024) { result.failures = 10; continue; }
                const auto* definition = reinterpret_cast<const uint32_t* (__fastcall*)(uint32_t)>(0x89fbf0)(sample.type);
                if (!definition || definition[0xdc / 4] < 1 || definition[0xdc / 4] > 2) { result.failures = 11; continue; }
                sample.footprint = definition[0xdc / 4];
                const auto sourceOffset = reinterpret_cast<const int32_t*>(source[1])[2];
                const void* creatureInterface = reinterpret_cast<const unsigned char*>(source) + 4 + sourceOffset;
                const auto* creatureMethods = *reinterpret_cast<const uint32_t* const*>(creatureInterface);
                sample.quantity = reinterpret_cast<uint32_t (__thiscall*)(const void*, uint32_t)>(
                    creatureMethods[0x1c / 4])(creatureInterface, healthContext);
                int32_t position[2]{};
                reinterpret_cast<void (__thiscall*)(const void*, int32_t*)>(methods[0x40 / 4])(actor, position);
                sample.x = position[0]; sample.y = position[1];
                if (!sample.quantity || sample.quantity > INT32_MAX || sample.x < 0 || sample.x >= 32 ||
                    sample.y < 0 || sample.y >= 32) { result.failures = 12; continue; }
                result.stacks[side][result.counts[side]++] = sample;
            }
        }
        return !result.failures && !result.overflow && result.counts[0] && result.counts[1];
    } __except (EXCEPTION_EXECUTE_HANDLER) { result.failures = 100 + stage; return false; }
}
inline bool ReadCombatUnitName(uint32_t vectorIdentity, CombatUnitSample& result) {
    __try {
        const auto* vector = reinterpret_cast<const uint32_t*>(vectorIdentity);
        if (!vector || !vector[0] || vector[1] <= vector[0] ||
            vector[1] - vector[0] >= sizeof(result.name)) { return false; }
        result.nameLength = vector[1] - vector[0];
        const auto* source = reinterpret_cast<const unsigned char*>(vector[0]);
        for (uint32_t index = 0; index < result.nameLength; ++index) {
            if (source[index] < 32 || source[index] > 126) { return false; }
            result.name[index] = static_cast<char>(source[index]);
        }
        result.name[result.nameLength] = 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool ReadCombatPositionResult(const h5::hooks::X86ObserverFrame& frame, CombatUnitSample& result) {
    result = {};
    __try {
        const auto* stack = reinterpret_cast<const int32_t*>(frame.savedEsp + 4);
        result.x = stack[7];result.y = stack[8];
        return result.x >= 0 && result.x < 32 && result.y >= 0 && result.y < 32 &&
            ReadCombatUnitName(frame.esi, result);
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool ReadCombatQuantityResult(const h5::hooks::X86ObserverFrame& frame, CombatUnitSample& result) {
    result = {};
    result.quantity = frame.eax;
    return static_cast<int32_t>(result.quantity) > 0 && ReadCombatUnitName(frame.ebp, result);
}

// Loaded rule coefficients only; no engine policy/output is read.
inline bool ReadPlacementStrategySettings(float* settings) {
    __try {
        const auto* source = reinterpret_cast<const unsigned char* (__cdecl*)()>(0xbcb3f0)();
        settings[0] = *reinterpret_cast<const float*>(source + 0x1d0);
        settings[1] = *reinterpret_cast<const float*>(source + 0x1d8);
        settings[2] = *reinterpret_cast<const float*>(source + 0xbfc);
        settings[3] = *reinterpret_cast<const float*>(source + 0xc00);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
