#include <h5/build.hpp>
#include <h5/hooks.hpp>
#include <h5/process.hpp>
#include <h5/image_placement.hpp>
#include <h5/graphics_lifetime.hpp>
#include <h5/script_observers.hpp>
#include <h5/camera_input.hpp>
#include <h5/adventure_input.hpp>
#include <h5/console.hpp>
#include <h5/resident_buffers.hpp>
namespace {
extern uint32_t committedArmyMethods[1],committedWorldMethods[34];
const uint32_t* __fastcall CommittedDefinitionGetter(uint32_t);
bool __fastcall CommittedActiveActor(const void*,void*);
}
#include "actor_input_fixture.hpp"
#include <xmmintrin.h>
#include <float.h>
#include <iostream>
#include <future>
#include <thread>

namespace {
void VerifyResidentBuffers(const char* first, const char* second) {
    using namespace h5::resident;
    const auto window = CreateWindowExW(0, L"STATIC", L"resident buffer fixture", WS_POPUP,
        0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) { throw std::runtime_error("Resident owner window unavailable"); }
    for (const auto* path : {first, second}) {
        const auto module = LoadLibraryA(path);
        const auto write = module ? reinterpret_cast<uint32_t (__stdcall*)(HWND)>(
            GetProcAddress(module, "WriteResidentBuffer")) : nullptr;
        if (!write || write(window) != 1) { throw std::runtime_error("Resident writer failed"); }
        if (!FreeLibrary(module)) { throw std::runtime_error("Resident writer did not unload"); }
        if (GetModuleHandleA(path)) { throw std::runtime_error("Resident writer still loaded"); }
        BufferLease lease(window, 1, 1, false);
        const auto expected = path == first ? 101u : 202u;
        if (lease.Bytes() != 65536 || !lease.Data() ||
            *reinterpret_cast<const uint32_t*>(lease.Data()) != expected) {
            throw std::runtime_error("Data did not survive writer DLL unload");
        }
        if (lease.WritableData()) { throw std::runtime_error("Read lease exposed writable data"); }
    }
    if (EnsureBuffer(window, 1, 2, 65536) || EnsureBuffer(window, 1, 1, 32) ||
        EnsureBuffer(window, 2, 1, MaximumBufferBytes + 1)) {
        throw std::runtime_error("Resident schema/size boundary accepted");
    }
    auto* registry = detail::FindBuffers(window);
    if (!registry) { throw std::runtime_error("Resident registry missing"); }
    registry->version = 99;
    const bool unknownVersionAllocated = EnsureBuffer(window, 2, 1, 32);
    bool unknownVersionBorrowed = false;
    { BufferLease unknown(window, 1, 1, false); unknownVersionBorrowed = unknown.Data() != nullptr; }
    registry->version = 2;
    if (unknownVersionAllocated || unknownVersionBorrowed) {
        throw std::runtime_error("Unknown resident registry version accepted");
    }
    const auto outerModule = LoadLibraryA(first), innerModule = LoadLibraryA(second);
    using Writer = uint32_t (__stdcall*)(HWND);
    const auto innerWriter = innerModule ? reinterpret_cast<Writer>(GetProcAddress(innerModule, "WriteSecondResidentBuffer")) : nullptr;
    const auto nestedWriter = outerModule ? reinterpret_cast<uint32_t (__stdcall*)(HWND, Writer)>(
        GetProcAddress(outerModule, "InvokeNestedResidentWriter")) : nullptr;
    if (!innerWriter || !nestedWriter || nestedWriter(window, innerWriter) != 1) {
        throw std::runtime_error("Two DLL nested distinct-slot leases failed");
    }
    const auto sameWriter = reinterpret_cast<Writer>(GetProcAddress(innerModule, "WriteResidentBuffer"));
    const auto nestedUpgrade = reinterpret_cast<uint32_t (__stdcall*)(HWND, Writer)>(
        GetProcAddress(outerModule, "InvokeReadHeldUpgrade"));
    if (!sameWriter || !nestedUpgrade || nestedUpgrade(window, sameWriter) != 1) {
        throw std::runtime_error("Two DLL same-slot read-held upgrade was not refused");
    }
    {
        BufferLease outer(window, 1, 1, true);
        BufferLease sameWrite(window, 1, 1, true), sameRead(window, 1, 1, false);
        if (!outer.Data() || sameWrite.Data() || sameRead.Data()) {
            throw std::runtime_error("Same-thread same-slot recursion was not refused");
        }
    }
    {
        BufferLease outer(window, 1, 1, false);
        BufferLease nestedRead(window, 1, 1, false), upgrade(window, 1, 1, true);
        if (!outer.Data() || nestedRead.Data() != outer.Data() || upgrade.Data()) {
            throw std::runtime_error("Read-held write upgrade was not refused or nested read lost ownership");
        }
    }
    if (!FreeLibrary(outerModule) || !FreeLibrary(innerModule)) {
        throw std::runtime_error("Nested resident fixture unload failed");
    }
    auto foreignThread = std::async(std::launch::async, [window] {
        return EnsureBuffer(window, 2, 1, 32);
    });
    if (foreignThread.get()) { throw std::runtime_error("Foreign-thread allocation accepted"); }
    for (uint32_t key = 2; key <= BufferCapacity; ++key) {
        if (!EnsureBuffer(window, key, 1, 32)) { throw std::runtime_error("Resident capacity too small"); }
    }
    if (EnsureBuffer(window, BufferCapacity + 1, 1, 32)) {
        throw std::runtime_error("Resident capacity exceeded");
    }
    DestroyWindow(window);
    BufferLease retired(window, 1, 1, false);
    if (retired.Data()) { throw std::runtime_error("Destroyed owner accepted"); }
}

uint32_t committedWorld[40]{}, committedArmies[2][28]{}, committedActors[2][32]{};
uint32_t committedArmyMethods[1]{},committedWorldMethods[34]{};
uint32_t committedSources[2][4]{}, committedDescriptor[8]{}, committedDefinition[64]{};
uint32_t committedActorMethods[32]{}, committedSourceMethods[1]{};
uint32_t committedWorldInterface[20]{}, committedCreatureInterface[8]{}, committedRosters[2][8]{};
uint32_t committedQuantities[2]{20,10};
bool committedDuplicateSides = false;
int32_t committedFieldWidth = 16, committedFieldHeight = 12;
void __fastcall CommittedFieldDimensions(const void*, void*, int32_t* width, int32_t* height) {
    *width = committedFieldWidth; *height = committedFieldHeight;
}
const uint32_t* __fastcall CommittedWorldGetter(const void*,void*) { return committedWorld; }
const uint32_t* __fastcall CommittedArmyGetter(const void*,void*,bool first) {
    return committedArmies[first || committedDuplicateSides ? 0 : 1];
}
uint32_t __fastcall CommittedHealthContext(const void*,void*) { return 42; }
bool __fastcall CommittedActiveActor(const void* actor,void*) { return actor != nullptr; }
const uint32_t* __fastcall CommittedSourceGetter(const void* actor,void*) {
    return committedSources[actor == committedActors[0] ? 0 : 1];
}
const uint32_t* __fastcall CommittedDescriptorGetter(const void*,void*) { return committedDescriptor; }
const uint32_t* __fastcall CommittedDefinitionGetter(uint32_t) { return committedDefinition; }
uint32_t __fastcall CommittedQuantityGetter(const void* creature,void*,uint32_t context) {
    return context == 42 ? committedQuantities[creature == committedSources[0]+1 ? 0 : 1] : 0;
}
void __fastcall CommittedPositionGetter(const void* actor,void*,int32_t* position) {
    position[0] = actor == committedActors[0] ? 2 : 13; position[1] = 5;
}
void VerifyCommittedDeploymentBoundaries() {
    // Production admission/enumeration, with native getters/tables at the
    // existing fixture boundary. No fixed-address mappings or game are needed.
    committedArmyMethods[0] = reinterpret_cast<uint32_t>(CommittedWorldGetter);
    committedWorldMethods[0x84/4] = reinterpret_cast<uint32_t>(CommittedArmyGetter);
    committedWorld[0]=reinterpret_cast<uint32_t>(committedWorldMethods);
    committedWorldMethods[0x44/4] = reinterpret_cast<uint32_t>(CommittedFieldDimensions);
    uint32_t fieldWidth = 99, fieldHeight = 99;
    if (!h5::army::ReadCombatFieldDimensions(reinterpret_cast<uint32_t>(committedWorld), fieldWidth, fieldHeight) ||
        fieldWidth != 16 || fieldHeight != 12) { throw std::runtime_error("Native field dimensions lost borders"); }
    const int32_t invalidDimensions[][2] = {{7,12}, {33,12}, {16,3}, {16,33}, {-1,12}};
    for (const auto& dimensions : invalidDimensions) {
        committedFieldWidth = dimensions[0]; committedFieldHeight = dimensions[1];
        fieldWidth = fieldHeight = 99;
        if (h5::army::ReadCombatFieldDimensions(reinterpret_cast<uint32_t>(committedWorld), fieldWidth, fieldHeight) ||
            fieldWidth || fieldHeight) { throw std::runtime_error("Invalid field geometry admitted or retained stale outputs"); }
    }
    committedFieldWidth = 16; committedFieldHeight = 12;
    fieldWidth = fieldHeight = 99;
    if (h5::army::ReadCombatFieldDimensions(0, fieldWidth, fieldHeight) || fieldWidth || fieldHeight) {
        throw std::runtime_error("Missing world retained field geometry");
    }
    committedWorldMethods[0x44/4] = 0;
    if (h5::army::ReadCombatFieldDimensions(reinterpret_cast<uint32_t>(committedWorld), fieldWidth, fieldHeight) ||
        fieldWidth || fieldHeight) { throw std::runtime_error("Missing field getter accepted"); }
    committedWorldMethods[0x44/4] = reinterpret_cast<uint32_t>(CommittedFieldDimensions);
    committedWorld[1]=reinterpret_cast<uint32_t>(committedWorldInterface);
    committedWorldInterface[0x40/4]=reinterpret_cast<uint32_t>(CommittedHealthContext);
    committedActorMethods[0x6c/4]=reinterpret_cast<uint32_t>(CommittedSourceGetter);
    committedActorMethods[0x40/4]=reinterpret_cast<uint32_t>(CommittedPositionGetter);
    committedSourceMethods[0]=reinterpret_cast<uint32_t>(CommittedDescriptorGetter);
    committedCreatureInterface[0x1c/4]=reinterpret_cast<uint32_t>(CommittedQuantityGetter);
    committedDescriptor[0x1c/4]=3;committedDefinition[0xdc/4]=1;
    for (unsigned side=0; side<2; ++side) {
        committedActors[side][0]=reinterpret_cast<uint32_t>(committedActorMethods);
        committedSources[side][0]=reinterpret_cast<uint32_t>(committedSourceMethods);
        committedSources[side][1]=reinterpret_cast<uint32_t>(committedCreatureInterface);
        for (auto& actor : committedRosters[side]) { actor=reinterpret_cast<uint32_t>(committedActors[side]); }
        committedArmies[side][0]=reinterpret_cast<uint32_t>(committedArmyMethods);
        committedArmies[side][0x28/4]=reinterpret_cast<uint32_t>(committedRosters[side]);
        committedArmies[side][0x2c/4]=reinterpret_cast<uint32_t>(committedRosters[side]+1);
    }
    h5::hooks::X86ObserverFrame frame{};frame.esi=reinterpret_cast<uint32_t>(committedArmies[0]);
    h5::army::CommittedDeploymentSample sample{};
    if (!h5::army::ReadCommittedDeployment(frame,sample) || sample.counts[0]!=1 || sample.counts[1]!=1 ||
        sample.stacks[0][0].quantity!=20 || sample.stacks[1][0].quantity!=10) {
        throw std::runtime_error("Distinct committed armies rejected");
    }
    committedDuplicateSides=true;
    if (h5::army::ReadCommittedDeployment(frame,sample)) {
        throw std::runtime_error("Same army accepted as both committed sides");
    }
    committedDuplicateSides=false;
    *reinterpret_cast<unsigned char*>(committedArmies[1]+0x64/4)=1;
    if (h5::army::ReadCommittedDeployment(frame,sample) || sample.failures!=4) {
        throw std::runtime_error("Pending deployment accepted");
    }
    *reinterpret_cast<unsigned char*>(committedArmies[1]+0x64/4)=0;
    const uint32_t originalEnd=committedArmies[0][0x2c/4];
    for (const uint32_t end : {originalEnd-8,originalEnd+1,originalEnd+64*4}) {
        committedArmies[0][0x2c/4]=end;
        if (h5::army::ReadCommittedDeployment(frame,sample) || sample.failures!=7) {
            throw std::runtime_error("Malformed committed roster accepted");
        }
    }
    committedArmies[0][0x2c/4]=reinterpret_cast<uint32_t>(committedRosters[0]+8);
    if (h5::army::ReadCommittedDeployment(frame,sample) || !sample.overflow) {
        throw std::runtime_error("Eight creature stacks accepted");
    }
    committedArmies[0][0x2c/4]=originalEnd;
    committedQuantities[0]=0;
    if (h5::army::ReadCommittedDeployment(frame,sample) || sample.failures!=12) {
        throw std::runtime_error("Zero committed quantity accepted");
    }
    committedQuantities[0]=20;committedDefinition[0xdc/4]=3;
    if (h5::army::ReadCommittedDeployment(frame,sample) || sample.failures!=11) {
        throw std::runtime_error("Invalid creature footprint accepted");
    }
    committedDefinition[0xdc/4]=1;
    if (!h5::army::ReadCommittedDeployment(frame,sample)) { throw std::runtime_error("Valid committed state did not recover"); }
}

uint32_t __fastcall InputFixtureOriginal(void*,void*,const uint32_t*) { return 77; }
void VerifyAdventureInputGate() {
    const auto target=reinterpret_cast<uintptr_t>(InputFixtureOriginal);
    const auto window=CreateWindowExW(0,L"STATIC",L"input fixture",WS_POPUP,0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    auto* gate=h5::hooks::CreateAdventureInputGate(target,window);
    if (!gate) { throw std::runtime_error("Adventure input gate allocation failed"); }
    const auto code=reinterpret_cast<uintptr_t>(gate)-h5::hooks::ScriptObserverDescriptorOffset;
    auto* site=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if (!site) { throw std::runtime_error("Input seal fixture allocation failed"); }
    site[0]=0xe9;
    const auto relative=static_cast<uint32_t>(code-reinterpret_cast<uintptr_t>(site)-5);
    std::memcpy(site+1,&relative,4);
    DWORD previous=0;
    if (!VirtualProtect(site,4096,PAGE_EXECUTE_READ,&previous) ||
        h5::hooks::FindAdventureInputGate(reinterpret_cast<uintptr_t>(site),target)!=gate) {
        throw std::runtime_error("Sealed input gate not recognized");
    }
    const auto originalCharacter=gate->property[0]; gate->property[0]=L'?';
    if (h5::hooks::FindAdventureInputGate(reinterpret_cast<uintptr_t>(site),target)) {
        throw std::runtime_error("Changed input property accepted");
    }
    gate->property[0]=originalCharacter;
    gate->focus.cbSize=0;
    if (h5::hooks::FindAdventureInputGate(reinterpret_cast<uintptr_t>(site),target)) {
        throw std::runtime_error("Invalid input focus buffer accepted");
    }
    gate->focus.cbSize=sizeof(GUITHREADINFO);
    const auto invoke=reinterpret_cast<uint32_t (__thiscall*)(void*,const uint32_t*)>(code);
    uint32_t event[5]{23,0,0,0,1};
    if (invoke(nullptr,event)!=77) { throw std::runtime_error("Unbound input gate changed original dispatch"); }
    if (!window || !SetPropW(window,L"XalKit.Console.Frame.v1",window)) { throw std::runtime_error("Input fixture panel property failed"); }
    if (invoke(nullptr,event)!=77) { throw std::runtime_error("Hidden panel captured input"); }
    ShowWindow(window,SW_SHOWNOACTIVATE);
    SetFocus(window);
    GUITHREADINFO information{}; information.cbSize=sizeof(information);
    if (!GetGUIThreadInfo(GetCurrentThreadId(),&information) || information.hwndFocus!=window) {
        throw std::runtime_error("Native input fixture focus unavailable");
    }
    if (invoke(nullptr,event)!=1) { throw std::runtime_error("Focused panel failed to capture verified key press"); }
    // Actual owned Tab/Escape events carry flags in the identity and padding
    // after the byte-sized press state. Do not treat either field as a DWORD.
    for (const auto identity : {0x0100000fu,0x01000001u,0x011ffc17u}) {
        event[0]=identity;event[4]=0x3e440001;
        if (invoke(nullptr,event)!=1) { throw std::runtime_error("Packed keyboard press escaped focused panel"); }
        event[4]=0x3e440000;
        if (invoke(nullptr,event)!=77) { throw std::runtime_error("Packed keyboard release was consumed"); }
    }
    for (const auto identity : {0x01000100u,0x010003ffu}) {
        event[0]=identity;event[4]=0x3e440001;
        if (invoke(nullptr,event)!=77) { throw std::runtime_error("Non-keyboard packed identity was consumed"); }
    }
    event[0]=23;event[4]=1;
    const auto outside=CreateWindowExW(0,L"STATIC",L"outside input fixture",WS_POPUP,
        0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if (!outside) { throw std::runtime_error("Outside input fixture window failed"); }
    ShowWindow(outside,SW_SHOWNOACTIVATE);
    SetFocus(outside);
    if (!GetGUIThreadInfo(GetCurrentThreadId(),&information) || information.hwndFocus!=outside ||
        invoke(nullptr,event)!=77) { throw std::runtime_error("Visible unfocused console captured map key"); }
    SetFocus(window);
    if (invoke(nullptr,event)!=1) { throw std::runtime_error("Refocused console failed to recapture key"); }
    DestroyWindow(outside);
    for (const auto kind : {3u,6u,99u}) {
        event[2]=kind;
        if (invoke(nullptr,event)!=77) { throw std::runtime_error("Input gate captured unknown or lifecycle event"); }
    }
    event[2]=0; event[0]=0xffffffff;
    if (invoke(nullptr,event)!=77) { throw std::runtime_error("Input gate captured named command"); }
    event[0]=23; event[2]=0; event[4]=0;
    if (invoke(nullptr,event)!=77) { throw std::runtime_error("Input gate captured non-press event"); }
    event[4]=1;
    const auto replacement=CreateWindowExW(0,L"STATIC",L"replacement input panel",WS_POPUP,
        0,0,32,32,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if (!replacement || !SetPropW(window,L"XalKit.Console.Frame.v1",replacement)) {
        throw std::runtime_error("Input panel replacement failed");
    }
    ShowWindow(replacement,SW_SHOWNOACTIVATE);
    SetFocus(replacement);
    if (invoke(nullptr,event)!=1) { throw std::runtime_error("Resident input gate retained old panel"); }
    DestroyWindow(replacement);
    if (invoke(nullptr,event)!=77) { throw std::runtime_error("Destroyed input panel retained capture"); }
    RemovePropW(window,L"XalKit.Console.Frame.v1");
    event[0]=23;
    if (invoke(nullptr,event)!=77) { throw std::runtime_error("Cleared panel retained key capture"); }
    const auto panel=CreateWindowExW(0,L"STATIC",L"module panel",WS_CHILD|WS_VISIBLE,0,0,24,24,window,nullptr,GetModuleHandleW(nullptr),nullptr);
    const auto edit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_VISIBLE,0,0,16,16,panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    const auto sibling=CreateWindowExW(0,L"STATIC",L"second module",WS_CHILD|WS_VISIBLE,24,0,8,24,window,nullptr,GetModuleHandleW(nullptr),nullptr);
    if (!panel || !edit || !sibling || !h5::ui::RegisterInputPanel(panel) || !h5::ui::RegisterInputPanel(sibling)) {
        throw std::runtime_error("Independent UI panel registration failed");
    }
    wcscpy_s(gate->property,h5::ui::KeyboardFocusProperty);
    if (h5::hooks::FindAdventureInputGate(reinterpret_cast<uintptr_t>(site),target)!=gate) {
        throw std::runtime_error("Generic focus property migration failed");
    }
    SetFocus(edit);
    auto capture=h5::ui::ReadInputPanels(window);
    h5::ui::PublishKeyboardFocus(window,capture);
    if (!capture.present || !capture.keyboard || capture.keyboardFocus!=edit || invoke(nullptr,event)!=1) {
        throw std::runtime_error("Registered panel child did not capture keyboard input");
    }
    SetFocus(sibling);capture=h5::ui::ReadInputPanels(window);h5::ui::PublishKeyboardFocus(window,capture);
    if (!capture.keyboard || capture.keyboardFocus!=sibling || invoke(nullptr,event)!=1) {
        throw std::runtime_error("Second module lost keyboard ownership");
    }
    SetFocus(window);capture=h5::ui::ReadInputPanels(window);h5::ui::PublishKeyboardFocus(window,capture);
    if (capture.keyboard || invoke(nullptr,event)!=77) { throw std::runtime_error("Map focus retained module capture"); }
    DestroyWindow(panel);DestroyWindow(sibling);
    capture=h5::ui::ReadInputPanels(window);h5::ui::PublishKeyboardFocus(window,capture);
    if (capture.present || invoke(nullptr,event)!=77) { throw std::runtime_error("Destroyed registered panels retained capture"); }
    wcscpy_s(gate->property,L"XalKit.Console.Frame.v1");
    if (h5::hooks::FindAdventureInputGate(reinterpret_cast<uintptr_t>(site),target)!=gate) {
        throw std::runtime_error("Legacy focus rollback failed");
    }
    // A rolling core update must recognize the previous sealed gate until the
    // owner thread replaces its jump. Unrecognized bodies remain rejected.
    const auto legacyCode=h5::hooks::AdventureInputCode(code,reinterpret_cast<uintptr_t>(gate),target,true);
    if (!VirtualProtect(reinterpret_cast<void*>(code),4096,PAGE_READWRITE,&previous)) {
        throw std::runtime_error("Legacy input fixture preparation failed");
    }
    std::memcpy(reinterpret_cast<void*>(code),legacyCode.data(),legacyCode.size());
    if (!h5::hooks::ObserverCodeDigest(code,legacyCode.size(),gate->digest) ||
        !VirtualProtect(reinterpret_cast<void*>(code),4096,PAGE_EXECUTE_READ,&previous) ||
        !FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(code),legacyCode.size()) ||
        h5::hooks::FindAdventureInputGate(reinterpret_cast<uintptr_t>(site),target)!=gate) {
        throw std::runtime_error("Previous sealed input gate rejected during core update");
    }
    gate->digest[0]^=1;
    if (h5::hooks::FindAdventureInputGate(reinterpret_cast<uintptr_t>(site),target)) {
        throw std::runtime_error("Changed legacy input digest accepted");
    }
    DestroyWindow(window);
    VirtualFree(site,0,MEM_RELEASE);
    VirtualFree(reinterpret_cast<void*>(code),0,MEM_RELEASE);
}
void VerifyConsoleAdmission() {
    const auto window = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) { throw std::runtime_error("Console admission fixture window failed"); }
    h5::ConsoleCommandRequest request;
    wcscpy_s(request.text, L"help");
    bool valid = h5::detail::AllowsConsoleDispatch(window, request);
    valid = valid && !h5::DispatchConsoleCommand(window, request); // Test host lacks pinned game sites.
    valid = valid && h5::detail::AllowsConsoleDispatch(window, request, 1);
    valid = valid && h5::detail::AllowsConsoleDispatch(window, request, 2);
    valid = valid && !h5::detail::AllowsConsoleDispatch(window, request, 3);
    valid = valid && !h5::DispatchGameText(window, request, 0);
    valid = valid && !h5::DispatchGameText(window, request, 1);
    valid = valid && !h5::DispatchGameText(window, request, 2);
    wcscpy_s(request.text, L"Event_123");
    valid = valid && h5::detail::AllowsConsoleDispatch(window, request, 1);
    for (const auto text : {L"event name", L"event-name", L"print(1)", L"\u0441\u043d\u0438\u043c\u043e\u043a", L"event\n"}) {
        wcscpy_s(request.text, text);
        valid = valid && !h5::detail::AllowsConsoleDispatch(window, request, 1);
    }
    wcscpy_s(request.text, L"print('test')");
    valid = valid && h5::detail::AllowsConsoleDispatch(window, request, 2);
    wcscpy_s(request.text, L" ~");
    valid = valid && h5::detail::AllowsConsoleDispatch(window, request, 2);
    for (const auto text : {L"print(1)\n", L"\u0441\u043a\u0440\u0438\u043f\u0442", L"\x7f"}) {
        wcscpy_s(request.text, text);
        valid = valid && !h5::detail::AllowsConsoleDispatch(window, request, 2);
    }
    std::fill(std::begin(request.text), std::end(request.text), L'x');
    request.text[127] = L'\0';
    valid = valid && h5::detail::AllowsConsoleDispatch(window, request, 1);
    request.text[127] = L'x';
    request.text[128] = L'\0';
    valid = valid && !h5::detail::AllowsConsoleDispatch(window, request, 1);
    std::fill(std::begin(request.text), std::end(request.text), L'x');
    valid = valid && !h5::detail::AllowsConsoleDispatch(window, request, 2);
    request.text[4095] = L'\0';
    valid = valid && h5::detail::AllowsConsoleDispatch(window, request, 2);
    request.size = 0;
    valid = valid && !h5::detail::AllowsConsoleDispatch(window, request);
    request = {};
    wcscpy_s(request.text, L"help");
    request.version = 2;
    valid = valid && !h5::detail::AllowsConsoleDispatch(window, request);
    request = {};
    valid = valid && !h5::detail::AllowsConsoleDispatch(window, request);
    std::fill(std::begin(request.text), std::end(request.text), L'x');
    valid = valid && !h5::detail::AllowsConsoleDispatch(window, request);
    request.text[4095] = L'\0';
    valid = valid && h5::detail::AllowsConsoleDispatch(window, request);
    request = {};
    wcscpy_s(request.text, L"help");
    valid = valid && !h5::detail::AllowsConsoleDispatch(nullptr, request);
    std::promise<HWND> ready;
    std::promise<void> finish;
    auto completion = finish.get_future();
    std::thread worker([&]() {
        const auto otherWindow = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        ready.set_value(otherWindow);
        completion.wait();
        if (otherWindow) { DestroyWindow(otherWindow); }
    });
    const auto otherWindow = ready.get_future().get();
    valid = valid && otherWindow && !h5::detail::AllowsConsoleDispatch(otherWindow, request);
    valid = valid && !h5::detail::AllowsConsoleDispatch(otherWindow, request, 1);
    valid = valid && !h5::detail::AllowsConsoleDispatch(otherWindow, request, 2);
    finish.set_value();
    worker.join();
    DestroyWindow(window);
    if (!valid) { throw std::runtime_error("Console ownership/thread/request admission failed"); }
}

void VerifyGeneratedScriptObserver() {
    static unsigned originalCalls = 0, observerCalls = 0;
    const auto original = +[]() { ++originalCalls; };
    const auto observer = +[]() { ++observerCalls; };
    auto* storage = h5::hooks::CreateScriptObserverStorage(reinterpret_cast<uintptr_t>(original), reinterpret_cast<uintptr_t>(original));
    if (!storage) { throw std::runtime_error("Generated script dispatcher allocation failed"); }
    const auto invoke = reinterpret_cast<void (__cdecl*)()>(reinterpret_cast<uintptr_t>(storage) - h5::hooks::ScriptObserverDescriptorOffset);
    invoke();
    auto* slot = h5::hooks::AddScriptObserver(*storage, observer);
    if (!slot) { throw std::runtime_error("Generated script subscriber registration failed"); }
    invoke();
    if (!h5::hooks::RemoveScriptObserver(slot, observer)) { throw std::runtime_error("Generated script subscriber removal failed"); }
    invoke();
    if (originalCalls != 3 || observerCalls != 1) {
        throw std::runtime_error("Generated tail jump or callback retirement failed");
    }
    static h5::hooks::X86ObserverFrame captured{};
    static uint32_t capturedArgument = 0, originalArgument = 0, capturedCalls = 0;
    const auto contextOriginal = +[](uint32_t argument) { originalArgument = argument; };
    const auto contextObserver = +[](const h5::hooks::X86ObserverFrame* frame) {
        captured = *frame;
        ++capturedCalls;
        capturedArgument = *reinterpret_cast<const uint32_t*>(frame->savedEsp + 8);
        _fpreset();
    };
    auto* contextSite = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!contextSite) { throw std::runtime_error("Context CALL fixture allocation failed"); }
    auto* contextual = h5::hooks::CreateScriptObserverStorage(
        reinterpret_cast<uintptr_t>(contextSite), reinterpret_cast<uintptr_t>(contextOriginal), true);
    if (!contextual || h5::hooks::AddScriptObserver(*contextual, observer) ||
        h5::hooks::AddContextObserver(*storage, contextObserver)) {
        throw std::runtime_error("Observer callback ABI mismatch accepted");
    }
    auto* contextSlot = h5::hooks::AddContextObserver(*contextual, contextObserver);
    if (!contextSlot) { throw std::runtime_error("Context subscriber registration failed"); }
    const auto contextInvoke = reinterpret_cast<uintptr_t>(contextual) - h5::hooks::ScriptObserverDescriptorOffset;
    contextSite[0] = 0xe8;
    const auto contextDisplacement = static_cast<uint32_t>(contextInvoke - reinterpret_cast<uintptr_t>(contextSite) - 5);
    memcpy(contextSite + 1, &contextDisplacement, sizeof(contextDisplacement));
    DWORD siteProtection = 0;
    if (!VirtualProtect(contextSite, 4096, PAGE_EXECUTE_READ, &siteProtection) ||
        !FlushInstructionCache(GetCurrentProcess(), contextSite, 5) ||
        h5::hooks::FindScriptObservers(reinterpret_cast<uintptr_t>(contextSite),
            reinterpret_cast<uintptr_t>(contextOriginal), true) != contextual ||
        h5::hooks::FindScriptObservers(reinterpret_cast<uintptr_t>(contextSite),
            reinterpret_cast<uintptr_t>(contextOriginal))) {
        throw std::runtime_error("Sealed context owner rejected or legacy owner misidentified");
    }
    unsigned short originalControl = 0, observedControl = 0, requestedControl = 0x027f;
    __asm {
        fnstcw originalControl
        fldcw requestedControl
        push 12345
        mov eax, 0x11223344
        mov ecx, 0x55667788
        mov edx, 0x12345678
        call contextInvoke
        add esp, 4
        fnstcw observedControl
        fldcw originalControl
    }
    if (captured.eax != 0x11223344 || captured.ecx != 0x55667788 || captured.edx != 0x12345678 ||
        capturedArgument != 12345 || originalArgument != 12345 || observedControl != requestedControl) {
        throw std::runtime_error("Context registers, stack arguments, FPU or retirement changed");
    }
    auto* secondContextSlot = h5::hooks::AddContextObserver(*contextual, contextObserver);
    const auto contextFunction = reinterpret_cast<void (__cdecl*)(uint32_t)>(contextInvoke);
    if (!secondContextSlot) { throw std::runtime_error("Second context subscriber rejected"); }
    contextFunction(54321);
    if (capturedCalls != 3 || capturedArgument != 54321 || originalArgument != 54321 ||
        !h5::hooks::RemoveContextObserver(contextSlot, contextObserver) ||
        !h5::hooks::RemoveContextObserver(secondContextSlot, contextObserver)) {
        throw std::runtime_error("Context dispatcher skipped subscriber or removal failed");
    }
    contextFunction(67890);
    if (capturedCalls != 3 || originalArgument != 67890) {
        throw std::runtime_error("Retired context callback executed or original tail lost");
    }
    auto* threaded = h5::hooks::CreateScriptObserverStorage(reinterpret_cast<uintptr_t>(contextSite),
        reinterpret_cast<uintptr_t>(contextOriginal), true, GetCurrentThreadId());
    if (!threaded) { throw std::runtime_error("Thread-filtered dispatcher allocation failed"); }
    const auto threadedCode = reinterpret_cast<uintptr_t>(threaded) - h5::hooks::ScriptObserverDescriptorOffset;
    const auto threadedDisplacement = static_cast<uint32_t>(threadedCode - reinterpret_cast<uintptr_t>(contextSite) - 5);
    if (!VirtualProtect(contextSite, 4096, PAGE_READWRITE, &siteProtection)) {
        throw std::runtime_error("Threaded CALL preparation failed");
    }
    memcpy(contextSite + 1, &threadedDisplacement, sizeof(threadedDisplacement));
    if (!VirtualProtect(contextSite, 4096, PAGE_EXECUTE_READ, &siteProtection) ||
        !FlushInstructionCache(GetCurrentProcess(), contextSite, 5) ||
        h5::hooks::FindScriptObservers(reinterpret_cast<uintptr_t>(contextSite),
            reinterpret_cast<uintptr_t>(contextOriginal), true, GetCurrentThreadId()) != threaded ||
        h5::hooks::FindScriptObservers(reinterpret_cast<uintptr_t>(contextSite),
            reinterpret_cast<uintptr_t>(contextOriginal), true, GetCurrentThreadId() + 1)) {
        throw std::runtime_error("Threaded owner identity checks failed");
    }
    auto* threadSlot = h5::hooks::AddContextObserver(*threaded, contextObserver);
    if (!threadSlot) { throw std::runtime_error("Threaded subscriber registration failed"); }
    const auto threadFunction = reinterpret_cast<void (__cdecl*)(uint32_t)>(threadedCode);
    threadFunction(222);
    std::thread foreignCall([&] { threadFunction(333); });
    foreignCall.join();
    if (capturedCalls != 4 || capturedArgument != 222 || originalArgument != 333 ||
        !h5::hooks::RemoveContextObserver(threadSlot, contextObserver)) {
        throw std::runtime_error("Worker entered SDK callback or original forwarding changed");
    }
    if (!VirtualFree(contextSite, 0, MEM_RELEASE)) { throw std::runtime_error("Context CALL fixture cleanup failed"); }
    // Resident storage deliberately survives until this isolated test process exits.
}

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

int main(int count, char** arguments) {
    try {
        if (count != 3) { throw std::runtime_error("Expected two resident writer fixtures"); }
        wchar_t invalidCreatureName[8]{L'x'};
        if (h5::army::ReadCreatureDisplayName(0, invalidCreatureName, 8) || invalidCreatureName[0] ||
            h5::army::ReadCreatureDisplayName(512, invalidCreatureName, 8) ||
            h5::army::ReadCreatureDisplayName(3, nullptr, 8)) {
            throw std::runtime_error("Invalid creature-name request crossed the native boundary");
        }
        VerifyCommittedDeploymentBoundaries();
        VerifyResidentBuffers(arguments[1], arguments[2]);
        VerifyConsoleAdmission();
        VerifyGeneratedScriptObserver();
        VerifyGraphicsTransaction();
        VerifyCameraInputDispatcher();
        VerifyAdventureInputGate();
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
