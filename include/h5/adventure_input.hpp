#pragma once
#include "script_observers.hpp"
#include <vector>

namespace h5::hooks {
// Resident code retains system API targets and POD only. Its caller owns
// publication and invocation on the verified, nonreentrant game thread.
struct AdventureInputGate {
    std::array<unsigned char,8> magic;
    uint32_t version, bytes, target, codeBytes;
    uint32_t gameWindow, propertyName, propertyApi, visibleApi, threadApi, focusApi;
    std::array<unsigned char,32> digest;
    uint32_t panelOwner;
    GUITHREADINFO focus;
    wchar_t property[28];
};
static_assert(offsetof(AdventureInputGate,panelOwner)==80);
static_assert(offsetof(AdventureInputGate,focus)==84);
static_assert(offsetof(AdventureInputGate,focus)+offsetof(GUITHREADINFO,hwndFocus)<=127);
static_assert(sizeof(GUITHREADINFO)==48);

inline std::vector<unsigned char> AdventureInputCode(uintptr_t code, uintptr_t data,
                                                     uintptr_t target) {
    static_assert(sizeof(void*)==4);
    // Event kind0/id<=255/pressed1 is verified in owned key traces.
    // Resolve the current panel from the game property every invocation.
    std::vector<unsigned char> bytes{
        0x9c,0x60,0x8b,0x44,0x24,0x28,0x83,0x78,0x08,0,0x75,0x6b,0x81,0x38,0xff,0,0,0,0x77,0x63,
        0x83,0x78,0x10,1,0x75,0x5d,0xbb,0,0,0,0,0xff,0x73,0,0xff,0x73,0,0xff,0x53,0,
        0x85,0xc0,0x74,0x4b,0x89,0xc6,0x56,0xff,0x53,0,0x85,0xc0,0x74,0x41,0x8d,0x43,0,0x50,0x56,
        0xff,0x53,0,0x85,0xc0,0x74,0x35,0x89,0xc7,0x64,0xa1,0x20,0,0,0,0x3b,0x43,0,0x75,0x28,
        0x8d,0x43,0,0x50,0x57,0xff,0x53,0,0x85,0xc0,0x74,0x1c,0x3b,0x73,0,0x75,0x17,
        0xff,0x73,0,0xff,0x73,0,0xff,0x53,0,0x39,0xf0,0x75,0x0a,0x61,0x9d,0xb8,1,0,0,0,0xc2,4,0,
        0x61,0x9d,0xe9,0,0,0,0};
    const uint32_t base=static_cast<uint32_t>(data);
    std::memcpy(bytes.data()+27,&base,4);
    for (const auto position : {33u,98u}) { bytes[position]=offsetof(AdventureInputGate,propertyName); }
    for (const auto position : {36u,101u}) { bytes[position]=offsetof(AdventureInputGate,gameWindow); }
    for (const auto position : {39u,104u}) { bytes[position]=offsetof(AdventureInputGate,propertyApi); }
    bytes[49]=offsetof(AdventureInputGate,visibleApi);
    bytes[56]=offsetof(AdventureInputGate,panelOwner);
    bytes[61]=offsetof(AdventureInputGate,threadApi);
    bytes[76]=offsetof(AdventureInputGate,panelOwner);
    bytes[81]=offsetof(AdventureInputGate,focus);
    bytes[86]=offsetof(AdventureInputGate,focusApi);
    bytes[93]=offsetof(AdventureInputGate,focus)+offsetof(GUITHREADINFO,hwndFocus);
    const auto displacement=static_cast<uint32_t>(target-code-bytes.size());
    std::memcpy(bytes.data()+122,&displacement,4);
    return bytes;
}

inline AdventureInputGate* CreateAdventureInputGate(uintptr_t target,HWND gameWindow) {
    const auto system=GetModuleHandleW(L"user32.dll");
    DWORD owner=0;
    if (!gameWindow || !GetWindowThreadProcessId(gameWindow,&owner) || owner!=GetCurrentProcessId()) { return nullptr; }
    const auto property=GetProcAddress(system,"GetPropW");
    const auto thread=GetProcAddress(system,"GetWindowThreadProcessId");
    const auto visible=GetProcAddress(system,"IsWindowVisible");
    const auto focus=GetProcAddress(system,"GetGUIThreadInfo");
    if (!property || !thread || !visible || !focus) { return nullptr; }
    auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,16384,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if (!memory) { return nullptr; }
    auto* gate=reinterpret_cast<AdventureInputGate*>(memory+ScriptObserverDescriptorOffset);
    *gate={};
    gate->magic={'H','5','I','N','P','0','0','1'};
    gate->version=1; gate->bytes=sizeof(*gate); gate->target=static_cast<uint32_t>(target);
    gate->gameWindow=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(gameWindow));
    wcscpy_s(gate->property,L"XalKit.Console.Frame.v1");
    gate->propertyName=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(gate->property));
    gate->propertyApi=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(property));
    gate->threadApi=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(thread));
    gate->visibleApi=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(visible));
    gate->focusApi=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(focus));
    gate->focus.cbSize=sizeof(GUITHREADINFO);
    const auto code=AdventureInputCode(reinterpret_cast<uintptr_t>(memory),reinterpret_cast<uintptr_t>(gate),target);
    gate->codeBytes=static_cast<uint32_t>(code.size());
    std::memcpy(memory,code.data(),code.size());
    DWORD previous=0;
    if (!ObserverCodeDigest(reinterpret_cast<uintptr_t>(memory),code.size(),gate->digest) ||
        !VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&previous) ||
        !FlushInstructionCache(GetCurrentProcess(),memory,code.size())) {
        VirtualFree(memory,0,MEM_RELEASE); return nullptr;
    }
    return gate;
}

inline AdventureInputGate* FindAdventureInputGate(uintptr_t site,uintptr_t target) {
    if (!ObserverMemory(site,5,PAGE_EXECUTE_READ)) { return nullptr; }
    const auto* jump=reinterpret_cast<const unsigned char*>(site);
    if (jump[0]!=0xe9) { return nullptr; }
    int32_t displacement=0; std::memcpy(&displacement,jump+1,4);
    const auto code=site+5+displacement;
    if (!ObserverMemory(code,126,PAGE_EXECUTE_READ,reinterpret_cast<void*>(code)) ||
        !ObserverMemory(code+ScriptObserverDescriptorOffset,sizeof(AdventureInputGate),PAGE_READWRITE,
                        reinterpret_cast<void*>(code))) { return nullptr; }
    auto* gate=reinterpret_cast<AdventureInputGate*>(code+ScriptObserverDescriptorOffset);
    const std::array<unsigned char,8> magic{'H','5','I','N','P','0','0','1'};
    constexpr wchar_t property[] = L"XalKit.Console.Frame.v1";
    const auto system=GetModuleHandleW(L"user32.dll");
    if (gate->magic!=magic || gate->version!=1 || gate->bytes!=sizeof(*gate) || gate->target!=target ||
        gate->propertyName!=reinterpret_cast<uintptr_t>(gate->property) ||
        std::memcmp(gate->property,property,sizeof(property)) || gate->focus.cbSize!=sizeof(GUITHREADINFO) ||
        !gate->propertyApi || !gate->threadApi || !gate->visibleApi || !gate->focusApi ||
        gate->propertyApi!=reinterpret_cast<uintptr_t>(GetProcAddress(system,"GetPropW")) ||
        gate->threadApi!=reinterpret_cast<uintptr_t>(GetProcAddress(system,"GetWindowThreadProcessId")) ||
        gate->visibleApi!=reinterpret_cast<uintptr_t>(GetProcAddress(system,"IsWindowVisible")) ||
        gate->focusApi!=reinterpret_cast<uintptr_t>(GetProcAddress(system,"GetGUIThreadInfo"))) { return nullptr; }
    const auto expected=AdventureInputCode(code,reinterpret_cast<uintptr_t>(gate),target);
    std::array<unsigned char,32> digest{};
    if (gate->codeBytes!=expected.size() || std::memcmp(reinterpret_cast<void*>(code),expected.data(),expected.size()) ||
        !ObserverCodeDigest(code,expected.size(),digest) || digest!=gate->digest) { return nullptr; }
    return gate;
}
}
