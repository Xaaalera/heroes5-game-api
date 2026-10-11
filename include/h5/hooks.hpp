#pragma once
#ifndef HEROES5_GAME_API_HOOKS_HPP
#define HEROES5_GAME_API_HOOKS_HPP
#include <cstddef>
#include <cstdint>
#include <span>

namespace h5::hooks {
// PUSHFD followed by PUSHAD, captured before the displaced game operation.
// savedEsp is the stack after PUSHFD; the entry stack is savedEsp + 4.
struct X86ObserverFrame {
    uint32_t edi, esi, ebp, savedEsp, ebx, edx, ecx, eax, flags;
};
static_assert(sizeof(X86ObserverFrame) == 36);
// Catalog entries describe bytes of the pinned Universe build, not a promise
// that arbitrary replacement code is safe at these sites.
template <size_t Size> struct Site {
    uintptr_t address;
    unsigned char expected[Size];
    const char* evidence;
    constexpr uintptr_t Resume() const { return address + Size; }
};
inline constexpr Site<6> BankLayout{0x5f8800, {0x8b, 0x2d, 0x68, 0x96, 0xfd, 0x00},
    "bank-reference InstallSelector: native installer test and live reference window"};
inline constexpr Site<6> PlacementRenderer{0x577010, {0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8},
    "deployment-preview InstallRendererHook: shipped legacy renderer hook"};
inline constexpr uintptr_t PlacementRendererCaller = 0x57788f;
inline constexpr Site<5> AdventureAttack{0x433440, {0x83, 0xec, 0x50, 0x53, 0x55},
    "deployment-preview InstallRendererHook: pre-battle entry"};
inline constexpr Site<6> PlacementHover{0x5767a0, {0x81, 0xec, 0xbc, 0, 0, 0},
    "deployment-preview InstallRendererHook: pointer hover"};
inline constexpr Site<6> PlacementConstructor{0x577e70, {0x53, 0x33, 0xdb, 0x56, 0x8b, 0xf1}, "deployment-preview placement lifecycle"};
inline constexpr Site<9> PlacementDestructor{0x579370, {0x56, 0x8b, 0xf1, 0x8b, 0x86, 0x98, 0, 0, 0}, "deployment-preview placement lifecycle"};
inline constexpr Site<5> DeploymentEnd{0x541fa0, {0x51, 0x53, 0x33, 0xdb, 0x56}, "deployment-preview deployment end"};
inline constexpr Site<7> ActorInput{0xbfbb08, {0xd9, 0x44, 0x24, 0x24, 0xd8, 0x43, 0x04}, "deployment-preview opt-in actor capture"};
inline constexpr Site<6> HeroCorrection{0xbfc12f, {0x03, 0xd8, 0x8b, 0x44, 0x24, 0x28}, "deployment-preview opt-in correction capture"};
inline constexpr Site<5> AuxiliaryPower{0xbfa4a5, {0x8b, 0x44, 0x24, 0x0c, 0x5f}, "deployment-preview opt-in power capture"};
inline constexpr Site<7> SourceStrength{0x4e9c4d, {0x8b, 0x7d, 0x08, 0x8b, 0x54, 0x24, 0x0c}, "deployment-preview opt-in source strength"};
inline constexpr Site<6> SourceSplitSeed{0x855451, {0x8b, 0x10, 0x6a, 0x64, 0x6a, 0x01}, "deployment-preview opt-in source split seed"};
inline constexpr Site<7> ScriptedSource{0x5d434c, {0x8d, 0x8c, 0x24, 0x84, 0, 0, 0}, "deployment-preview opt-in scripted source"};
inline constexpr Site<6> PlacementTooltip{0x546f40, {0x81, 0xec, 0x14, 0x07, 0, 0}, "deployment-preview tooltip source"};
inline constexpr Site<6> ProjectionOutline{0x578986, {0x8b, 0x56, 0x40, 0x8b, 0x46, 0x3c}, "deployment-preview projection outline"};
inline constexpr Site<5> CombatPositionResult{0x568234, {0x8d,0x44,0x24,0x20,0x50},
    "existing game_control after-Start position observer; result pair and unit name"};
inline constexpr Site<8> CombatQuantityResult{0x5688e4, {0x89,0x44,0x24,0x14,0x8d,0x44,0x24,0x14},
    "existing game_control after-Start creature-number observer; EAX result and unit name"};
inline constexpr Site<8> CombatRosterCount{0x567ef2, {0x8b,0x44,0x24,0x18,0x2b,0x44,0x24,0x14},
    "pinned GetUnits binding: filtered native unit vector before Lua table construction; verification only"};
inline constexpr Site<6> DeploymentCommitted{0x4d94b7, {0x8b,0x06,0x8b,0xce,0xff,0x10},
    "army ready command: both deployment flags cleared and both army initializers returned; before world start callback; ESI army"};
inline constexpr Site<6> ProjectionTooltipHitTest{0xa4eb10, {0x8b, 0x4f, 0x98, 0x8b, 0x04, 0xb1}, "deployment-preview projection hit test"};
inline constexpr Site<5> ProjectionTooltipSkip{0xa4eb2c, {0x83,0xee,0x01,0x79,0xc6},
    "deployment-preview existing hit-test decoration-skip continuation"};
inline constexpr Site<7> ProjectionProviderWindow{0x6b7eb0, {0x56,0x8b,0xf1,0x83,0x7e,0x0c,0x00},
    "legacy provider window callback; pinned native prologue verified"};
inline constexpr Site<7> ProjectionProviderClick{0x6b7790, {0x83,0x41,0x10,0x01,0x8b,0x41,0x08},
    "legacy provider right-click callback; pinned native prologue verified"};
inline constexpr Site<7> ProjectionDetailsCanRotate{0x789612,{0x8b,0x06,0x8b,0xce,0xff,0x50,0x10},
    "native description factory rotator capability call"};
// Native single-specimen rotator, including its preceding RTTI locator.
// All entries remain game-owned; only the no-argument bool capability differs.
inline constexpr uint32_t DetailsRotatorMethods[16]{0xedfde0,0x782cf0,0x782cf0,0x724930,
    0xc98940,0x412b40,0xa332c0,0xa332c0,0x412b40,0xc98940,0xc98940,0xc98940,
    0xaa2e60,0x78af10,0xc98940,0x412b40};
inline constexpr Site<5> DetailsRotationAvailable{0x412bd0,{0xb0,0x01,0xc3,0xcc,0xcc},
    "pinned native no-argument bool true; preserves ECX and caller stack"};
inline constexpr Site<7> ProjectionDetailsCanRotateContinue{0x789619,{0xf6,0xd8,0x1b,0xc0,0x24,0xfd,0x83},
    "native description factory capability result handling"};
inline constexpr Site<7> ProjectionDetailsNext{0x78a702,{0x8b,0x4e,0x2c,0x8b,0x01,0xff,0x10},
    "native description next button calls rotator slot0"};
inline constexpr Site<7> ProjectionDetailsNextContinue{0x78a709,{0x8b,0xcf,0xe8,0xe0,0xfd,0xff,0xff},
    "native description next button refresh"};
inline constexpr Site<8> ProjectionDetailsPrevious{0x78a6ea,{0x8b,0x4e,0x2c,0x8b,0x11,0xff,0x52,0x04},
    "native description previous button calls rotator slot1"};
inline constexpr Site<7> ProjectionDetailsPreviousContinue{0x78a6f2,{0x8b,0xcf,0xe8,0xf7,0xfd,0xff,0xff},
    "native description previous button refresh"};
inline constexpr Site<10> PlacementPolicyInitialize{0x8598e0,
    {0x56, 0x8b, 0xf1, 0x80, 0xbe, 0xd0, 0, 0, 0, 0},
    "accuracy-campaign policy observer: original aggregate verification only"};
inline constexpr Site<5> DefensiveCapabilityFirst{0xa2fe26, {0xe8,0xe5,0xf8,0xff,0xff},
    "original defensive selector first capability call; verification only"};
inline constexpr Site<5> DefensiveCapabilitySecond{0xa2fe83, {0xe8,0x88,0xf8,0xff,0xff},
    "original defensive selector opposing capability call; verification only"};
inline constexpr Site<8> DefensiveCapabilityFirstResult{0xa2fe2b, {0x84,0xc0,0x75,0x22,0xd9,0x44,0x24,0x28},
    "original first capability result before native branch; verification only"};
inline constexpr Site<6> DefensiveCapabilitySecondResult{0xa2fe88, {0x33,0xdb,0x84,0xc0,0x75,0x07},
    "original opposing capability result before native branch; verification only"};
inline constexpr Site<6> DefensiveDecisionStore{0xa2fea0, {0x88,0x85,0x98,0,0,0},
    "original defensive flag store; verification only"};
inline std::span<const unsigned char> EngineSiteSignature(uintptr_t address) {
    struct Entry { uintptr_t address; std::span<const unsigned char> expected; };
    const Entry entries[]{
        {AdventureAttack.address, AdventureAttack.expected},
        {PlacementRenderer.address, PlacementRenderer.expected},
        {PlacementHover.address, PlacementHover.expected},
        {PlacementConstructor.address, PlacementConstructor.expected},
        {PlacementDestructor.address, PlacementDestructor.expected},
        {DeploymentEnd.address, DeploymentEnd.expected},
        {ActorInput.address, ActorInput.expected},
        {HeroCorrection.address, HeroCorrection.expected},
        {AuxiliaryPower.address, AuxiliaryPower.expected},
        {SourceStrength.address, SourceStrength.expected},
        {SourceSplitSeed.address, SourceSplitSeed.expected},
        {ScriptedSource.address, ScriptedSource.expected},
        {PlacementTooltip.address, PlacementTooltip.expected},
        {ProjectionOutline.address, ProjectionOutline.expected},
        {ProjectionTooltipHitTest.address, ProjectionTooltipHitTest.expected},
        {PlacementPolicyInitialize.address, PlacementPolicyInitialize.expected}};
    for (const auto& entry : entries) {
        if (entry.address == address) { return entry.expected; }
    }
    return {};
}
inline constexpr Site<5> ScriptDispatchCall{0xd112db, {0xe8, 0xa0, 0xf6, 0xff, 0xff},
    "devkit game_control and native HMR: live main-thread CALL observer"};
inline constexpr uintptr_t ScriptDispatchTarget = 0xd10980;
inline constexpr Site<6> AdventureCameraInput{0x58c69b, {0xff, 0x90, 0x90, 0, 0, 0},
    "pinned adventure input handler: virtual camera input with mouse/keyboard suppression"};
inline constexpr Site<5> AdventureInputTail{0x5a1a03, {0xe9,0x08,0xac,0xfe,0xff},
    "validated adjustor tail before adventure binding dispatch; owned physical keyboard trace"};
inline constexpr uintptr_t AdventureInputTarget = 0x58c610;
inline constexpr Site<16> ExitRequest{0x838c10,
    {0x56, 0x8b, 0xf1, 0x85, 0xf6, 0x74, 0x04, 0x83, 0x46, 0x04, 0x01, 0x57, 0x8b, 0x3d, 0x98, 0x71},
    "devkit native exit: ECX=null queues game exit; verified owned mailbox exit0"};
}
#endif
