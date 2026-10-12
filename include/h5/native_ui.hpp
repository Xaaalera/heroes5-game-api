#pragma once
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace h5::ui {
// Pinned Universe x86. Caller verifies build/ownership and supplies a bounded
// bool reader(uint32_t address, void* destination, size_t bytes). No native
// function is called by the read APIs. Read on the UI thread or from a caller-stabilized view;
// these structural checks do not provide synchronization with game mutation.
struct NativeWindowSnapshot {
    uint32_t interfaceAddress = 0, objectAddress = 0, sharedAddress = 0;
    char name[128]{};
    float left = 0, top = 0, width = 0, height = 0;
};

namespace native_detail {
template<class Reader>
bool Read(Reader& reader, int64_t address, void* destination, size_t bytes) {
    return address >= 0x10000 && bytes && bytes <= UINT32_MAX &&
        static_cast<uint64_t>(address) + bytes <= (uint64_t{1} << 32) &&
        reader(static_cast<uint32_t>(address), destination, bytes);
}
template<class Reader>
bool Word(Reader& reader, int64_t address, uint32_t& value) {
    return Read(reader, address, &value, sizeof(value));
}
// Native IWindow thunks adjust the virtual base, then tail-call the same
// CWindow implementation. Do not confuse IScreen/IWindow with complete objects.
template<class Reader>
bool CommonBase(Reader& reader, uint32_t window, uint32_t slot,
    uint32_t expectedTarget, uint32_t& common) {
    uint32_t table = 0, entry = 0, displacement = 0;
    std::array<unsigned char, 16> code{};
    if (!Word(reader, window, table) || !Word(reader, int64_t{table} + slot, entry) ||
        !Read(reader, entry, code.data(), code.size()) ||
        code[0] != 0x2b || code[1] != 0x49 || code[2] != 0xfc ||
        !Word(reader, int64_t{window} - 4, displacement) || displacement > 4096) { return false; }
    uint32_t adjustment = 0, jump = 0;
    if (code[3] == 0x83 && code[4] == 0xe9 && code[5] < 0x80) {
        adjustment = code[5]; jump = 6;
    } else if (code[3] == 0x81 && code[4] == 0xe9) {
        std::memcpy(&adjustment, code.data() + 5, 4); jump = 9;
    } else { return false; }
    int32_t relative = 0;
    if (adjustment > 4096 || code[jump] != 0xe9) { return false; }
    std::memcpy(&relative, code.data() + jump + 1, 4);
    const int64_t target = int64_t{entry} + jump + 5 + relative;
    const int64_t adjusted = int64_t{window} - displacement - adjustment;
    if (target != expectedTarget || adjusted < 0x10000 || adjusted > UINT32_MAX) { return false; }
    common = static_cast<uint32_t>(adjusted); return true;
}
}

template<class Reader>
bool ReadNativeWindow(Reader& reader, uint32_t window, NativeWindowSnapshot& output) {
    output = {};
    constexpr std::array<unsigned char, 19> nameImplementation{
        0x8b,0x81,0xf4,0xfe,0xff,0xff,0x81,0xc1,0xf4,0xfe,0xff,0xff,
        0xff,0x50,0x08,0x83,0xc0,0x58,0xc3};
    std::array<unsigned char, 19> actual{};
    uint32_t common = 0, table = 0, getter = 0, shared = 0;
    if (!native_detail::Read(reader, 0xa4d8e0, actual.data(), actual.size()) || actual != nameImplementation ||
        !native_detail::CommonBase(reader, window, 0xe8, 0xa4d8e0, common) || common < 0x1010c ||
        !native_detail::Word(reader, int64_t{common} - 0x10c, table) ||
        !native_detail::Word(reader, int64_t{table} + 8, getter)) { return false; }
    std::array<unsigned char, 7> sharedGetter{};
    uint32_t sharedOffset = 0;
    if (!native_detail::Read(reader, getter, sharedGetter.data(), sharedGetter.size()) ||
        sharedGetter[0] != 0x8b || sharedGetter[1] != 0x81 || sharedGetter[6] != 0xc3) { return false; }
    std::memcpy(&sharedOffset, sharedGetter.data() + 2, 4);
    if (sharedOffset > 4096 || !native_detail::Word(reader,
        int64_t{common} - 0x10c + sharedOffset, shared)) { return false; }
    uint32_t name[3]{};
    NativeWindowSnapshot candidate{};
    float position[2]{}, size[2]{};
    if (!native_detail::Read(reader, int64_t{shared} + 0x58, name, sizeof(name)) ||
        name[1] < name[0] || name[2] < name[1] || name[1] - name[0] >= sizeof(candidate.name) ||
        (name[1] != name[0] && !native_detail::Read(reader, name[0], candidate.name, name[1] - name[0])) ||
        !native_detail::Read(reader, int64_t{common} - 0xa8, position, sizeof(position)) ||
        !native_detail::Read(reader, int64_t{shared} + 0xa4, size, sizeof(size))) { return false; }
    if (!std::isfinite(position[0]) || !std::isfinite(position[1]) ||
        !std::isfinite(size[0]) || !std::isfinite(size[1]) || size[0] < 0 || size[1] < 0 ||
        !std::isfinite(position[0] + size[0]) || !std::isfinite(position[1] + size[1])) { return false; }
    for (uint32_t index = 0; index < name[1] - name[0]; ++index) {
        if (static_cast<unsigned char>(candidate.name[index]) < 0x20 ||
            static_cast<unsigned char>(candidate.name[index]) > 0x7e) { return false; }
    }
    candidate.interfaceAddress = window; candidate.objectAddress = common - 0x10c;
    candidate.sharedAddress = shared; candidate.left = position[0]; candidate.top = position[1];
    candidate.width = size[0]; candidate.height = size[1]; output = candidate; return true;
}

template<class Reader>
bool ReadNativeWindowChildren(Reader& reader, uint32_t window,
    std::array<uint32_t, 128>& output, uint32_t& count) {
    output = {}; count = 0;
    constexpr std::array<unsigned char, 8> lookupHead{0x83,0xec,0x0c,0x53,0x8b,0x5c,0x24,0x14};
    std::array<unsigned char, 8> actual{};
    uint32_t common = 0, begin = 0, end = 0;
    if (!native_detail::Read(reader, 0xa4ecd0, actual.data(), actual.size()) || actual != lookupHead ||
        !native_detail::CommonBase(reader, window, 0x94, 0xa4ecd0, common) ||
        !native_detail::Word(reader, int64_t{common} - 0x68, begin) ||
        !native_detail::Word(reader, int64_t{common} - 0x64, end) ||
        end < begin || (end - begin) % 4 || (end - begin) / 4 > output.size()) { return false; }
    std::array<uint32_t, 128> candidate{};
    const uint32_t length = (end - begin) / 4;
    for (uint32_t index = 0; index < length; ++index) {
        uint32_t child = 0, virtualTable = 0, displacement = 0;
        if (!native_detail::Word(reader, int64_t{begin} + index * 4, child) ||
            !native_detail::Word(reader, int64_t{child} + 16, virtualTable) ||
            !native_detail::Word(reader, int64_t{virtualTable} + 8, displacement) || displacement > 4096 ||
            uint64_t{child} + 16 + displacement > UINT32_MAX) { return false; }
        candidate[index] = child + 16 + displacement;
        NativeWindowSnapshot snapshot{};
        if (candidate[index] == window ||
            std::find(candidate.begin(), candidate.begin() + index, candidate[index]) != candidate.begin() + index ||
            !ReadNativeWindow(reader, candidate[index], snapshot)) { return false; }
    }
    uint32_t currentBegin = 0, currentEnd = 0;
    if (!native_detail::Word(reader, int64_t{common} - 0x68, currentBegin) ||
        !native_detail::Word(reader, int64_t{common} - 0x64, currentEnd) ||
        currentBegin != begin || currentEnd != end) { return false; }
    output = candidate; count = length; return true;
}

template<class Reader>
bool ReadNativeCombatRoot(Reader& reader,uint32_t& output,uint32_t* screenObject=nullptr) {
    output=0;
    if (screenObject) { *screenObject=0; }
    uint32_t head=0,node=0,screen=0,table=0,locator=0,descriptor=0,displacement=0,screenDisplacement=0,root=0,virtualTable=0;
    std::array<uint32_t,16> visited{};uint32_t count=0;
    if (!native_detail::Word(reader,0xfe719c,head) || !native_detail::Word(reader,head,node)) { return false; }
    while (node!=head) {
        if (count==visited.size() || std::find(visited.begin(),visited.begin()+count,node)!=visited.begin()+count ||
            !native_detail::Word(reader,int64_t{node}+8,screen)) { return false; }
        visited[count++]=node;
        if (!native_detail::Word(reader,node,node)) { return false; }
    }
    constexpr char expected[]=".?AVCCombatArenaScreen@NUI@@";
    char actual[sizeof(expected)]{};
    if (!screen || !native_detail::Word(reader,screen,table) ||
        !native_detail::Word(reader,int64_t{table}-4,locator) ||
        !native_detail::Word(reader,int64_t{locator}+12,descriptor) ||
        !native_detail::Read(reader,int64_t{descriptor}+8,actual,sizeof(actual)) || std::memcmp(actual,expected,sizeof(actual)) ||
        !native_detail::Word(reader,int64_t{locator}+4,screenDisplacement) || screenDisplacement>8192 || screen<screenDisplacement ||
        !native_detail::Word(reader,int64_t{screen}-screenDisplacement+0xa0,root) ||
        !native_detail::Word(reader,int64_t{root}+4,virtualTable) ||
        !native_detail::Word(reader,int64_t{virtualTable}+8,displacement) || displacement>4096 ||
        uint64_t{root}+4+displacement>UINT32_MAX) { return false; }
    const uint32_t candidate=root+4+displacement;
    uint32_t common=0;
    if (!native_detail::CommonBase(reader,candidate,0x94,0xa4ecd0,common)) { return false; }
    output=candidate;
    // The stack stores an IScreen subobject; the screen operation takes the complete object.
    if (screenObject) { *screenObject=screen-screenDisplacement; }
    return true;
}

template<class Reader>
bool ReadNativeWindowVisible(Reader& reader,uint32_t window,bool& visible) {
    visible=false;NativeWindowSnapshot snapshot{};unsigned char value=0;
    if (!ReadNativeWindow(reader,window,snapshot) ||
        !native_detail::Read(reader,int64_t{snapshot.sharedAddress}+0x70,&value,1) || value>1) { return false; }
    visible=value!=0;return true;
}

// Distinguish a retired combat root from an unreadable screen stack. Cleanup
// must not interpret the absence of an active combat screen as a read failure.
template<class Reader>
bool ReadNativeCombatRootPresence(Reader& reader,uint32_t expectedRoot,bool& present) {
    present=false;uint32_t head=0,node=0;
    if (!expectedRoot || !native_detail::Word(reader,0xfe719c,head) || !native_detail::Word(reader,head,node)) { return false; }
    const uint32_t first=node;
    std::array<uint32_t,16> visited{};uint32_t count=0;bool found=false;
    while (node!=head) {
        uint32_t screen=0,table=0,locator=0,descriptor=0,displacement=0,rootObject=0,virtualTable=0,rootDisplacement=0;
        if (count==visited.size() || std::find(visited.begin(),visited.begin()+count,node)!=visited.begin()+count ||
            !native_detail::Word(reader,int64_t{node}+8,screen) || !native_detail::Word(reader,screen,table) ||
            !native_detail::Word(reader,int64_t{table}-4,locator) || !native_detail::Word(reader,int64_t{locator}+12,descriptor)) { return false; }
        visited[count++]=node;
        constexpr char combat[]=".?AVCCombatArenaScreen@NUI@@";char name[sizeof(combat)]{};
        if (!native_detail::Read(reader,int64_t{descriptor}+8,name,sizeof(name)) || std::memcmp(name,".?AV",4)) { return false; }
        if (!std::memcmp(name,combat,sizeof(combat))) {
            if (!native_detail::Word(reader,int64_t{locator}+4,displacement) || displacement>8192 || screen<displacement ||
                !native_detail::Word(reader,int64_t{screen}-displacement+0xa0,rootObject) ||
                !native_detail::Word(reader,int64_t{rootObject}+4,virtualTable) ||
                !native_detail::Word(reader,int64_t{virtualTable}+8,rootDisplacement) || rootDisplacement>4096 ||
                uint64_t{rootObject}+4+rootDisplacement>UINT32_MAX) { return false; }
            const uint32_t root=rootObject+4+rootDisplacement;NativeWindowSnapshot snapshot{};
            if (!ReadNativeWindow(reader,root,snapshot)) { return false; }
            found=found || root==expectedRoot;
        }
        if (!native_detail::Word(reader,node,node)) { return false; }
    }
    uint32_t currentHead=0,currentFirst=0;
    if (!native_detail::Word(reader,0xfe719c,currentHead) || currentHead!=head ||
        !native_detail::Word(reader,head,currentFirst) || currentFirst!=first) { return false; }
    present=found;return true;
}

// IModalWindow interfaces owned by the active combat screen, in list order.
// Keep this separate from ordinary root children: a visible child is not a modal.
template<class Reader>
bool ReadNativeCombatModals(Reader& reader,std::array<uint32_t,128>& output,uint32_t& count) {
    output={};count=0;
    uint32_t root=0,screen=0,head=0,node=0;
    if (!ReadNativeCombatRoot(reader,root,&screen) ||
        !native_detail::Word(reader,int64_t{screen}+0x14c,head) ||
        !native_detail::Word(reader,head,node)) { return false; }
    std::array<uint32_t,128> visited{},candidate{};uint32_t length=0;
    while (node!=head) {
        uint32_t modal=0;
        if (length==visited.size() || std::find(visited.begin(),visited.begin()+length,node)!=visited.begin()+length ||
            !native_detail::Word(reader,int64_t{node}+8,modal) || modal<0x10000 ||
            std::find(candidate.begin(),candidate.begin()+length,modal)!=candidate.begin()+length) { return false; }
        visited[length]=node;candidate[length++]=modal;
        if (!native_detail::Word(reader,node,node)) { return false; }
    }
    output=candidate;count=length;return true;
}

template<class Reader>
bool ReadNativeWindowClass(Reader& reader,uint32_t window,uint32_t& type) {
    type=0;NativeWindowSnapshot snapshot{};uint32_t candidate=0;
    if (!ReadNativeWindow(reader,window,snapshot) || !native_detail::Word(reader,int64_t{snapshot.sharedAddress}+0x44,candidate)) { return false; }
    type=candidate;return true;
}

// Read the text owned by a stock WindowTextView. Copies UTF-16 into caller
// storage; failure empties the visible string and resets length. No native call.
// Resolve the sole text view in a bounded stock scroll-container subtree.
// Names alone are insufficient: registered text may be renamed by the engine.
template<class Reader>
bool ReadNativeScrollTextWindow(Reader& reader,uint32_t container,uint32_t& output) {
    output=0;uint32_t type=0;
    if (!ReadNativeWindowClass(reader,container,type) || type!=386593537) { return false; }
    std::array<uint32_t,256> pending{};std::array<unsigned,256> depths{};
    uint32_t queued=1,found=0;pending[0]=container;
    for (uint32_t index=0;index<queued;++index) {
        if (!ReadNativeWindowClass(reader,pending[index],type)) { return false; }
        if (type==285694860) {
            if (found) { return false; }
            found=pending[index];
        }
        std::array<uint32_t,128> children{};uint32_t count=0;
        if (!ReadNativeWindowChildren(reader,pending[index],children,count) || count>pending.size()-queued ||
            (depths[index]>=3 && count)) { return false; }
        for (uint32_t child=0;child<count;++child) {
            if (std::find(pending.begin(),pending.begin()+queued,children[child])!=pending.begin()+queued) { return false; }
            pending[queued]=children[child];depths[queued]=depths[index]+1;++queued;
        }
    }
    output=found;return found!=0;
}

template<class Reader>
bool ReadNativeWindowText(Reader& reader,uint32_t window,wchar_t* output,uint32_t capacity,uint32_t& length) {
    length=0;
    if (!output || !capacity) { return false; }
    output[0]=0;
    if (capacity>16385) { return false; }
    NativeWindowSnapshot snapshot{};uint32_t type=0,table=0,getter=0;
    unsigned char code[4]{};
    if (!ReadNativeWindow(reader,window,snapshot) ||
        !native_detail::Word(reader,int64_t{snapshot.sharedAddress}+0x44,type) || type!=285694860 ||
        !native_detail::Word(reader,int64_t{snapshot.objectAddress}+0xf8,table) ||
        !native_detail::Word(reader,table,getter) || getter!=0xb95690 ||
        !native_detail::Read(reader,getter,code,sizeof(code)) ||
        code[0]!=0x8d || code[1]!=0x41 || code[2]!=0x14 || code[3]!=0xc3) { return false; }
    uint32_t begin=0,end=0,room=0;
    if (!native_detail::Word(reader,int64_t{snapshot.objectAddress}+0x10c,begin) ||
        !native_detail::Word(reader,int64_t{snapshot.objectAddress}+0x110,end) ||
        !native_detail::Word(reader,int64_t{snapshot.objectAddress}+0x114,room) ||
        end<begin || room<end || ((begin|end|room)&1) || (begin && begin<0x10000) ||
        (!begin && (end || room)) || end-begin>32768 || (end-begin)/2>=capacity) { return false; }
    const uint32_t count=(end-begin)/2;
    if (count && !native_detail::Read(reader,begin,output,count*sizeof(wchar_t))) { output[0]=0;return false; }
    output[count]=0;length=count;return true;
}

namespace native_detail {
// WindowTextView overrides placement to reflow its text. Its IWindow thunk
// adjusts only the virtual displacement; ordinary widgets also subtract a
// fixed base offset. Both implementations preserve the five-value ABI.
template<class Reader>
bool PlacementMethod(Reader& reader,uint32_t window,NativeWindowSnapshot& snapshot,uint32_t& method) {
    snapshot={};method=0;NativeWindowSnapshot candidate{};uint32_t table=0,entry=0,common=0;
    constexpr unsigned char head[]{0x83,0xec,0x10,0x53,0x8a,0x5c,0x24,0x28,0xf6,0xc3,0x01,0x56,0x57,0x8b,0xf1};
    constexpr unsigned char tail[]{0x5f,0x5e,0x5b,0x83,0xc4,0x10,0xc2,0x14,0x00};
    unsigned char actualHead[sizeof(head)]{},actualTail[sizeof(tail)]{};
    if (!ReadNativeWindow(reader,window,candidate) || !Word(reader,window,table) || !Word(reader,int64_t{table}+0x58,entry) ||
        !Read(reader,0xa4f810,actualHead,sizeof(actualHead)) || std::memcmp(actualHead,head,sizeof(head)) ||
        !Read(reader,0xa4f971,actualTail,sizeof(actualTail)) || std::memcmp(actualTail,tail,sizeof(tail))) { return false; }
    if (!CommonBase(reader,window,0x58,0xa4f810,common)) {
        uint32_t type=0,displacement=0;
        unsigned char thunk[8]{},overrideHead[6]{},overrideTail[7]{};
        constexpr unsigned char expectedHead[]{0x83,0xec,0x18,0x56,0x8b,0xf1};
        constexpr unsigned char expectedTail[]{0x5e,0x83,0xc4,0x18,0xc2,0x14,0x00};
        int32_t jump=0;
        if (!Word(reader,int64_t{candidate.sharedAddress}+0x44,type) || type!=285694860 ||
            !Word(reader,int64_t{window}-4,displacement) || displacement>4096 ||
            int64_t{window}-displacement!=int64_t{candidate.objectAddress}+0x13c ||
            !Read(reader,entry,thunk,sizeof(thunk)) || thunk[0]!=0x2b || thunk[1]!=0x49 || thunk[2]!=0xfc || thunk[3]!=0xe9) { return false; }
        std::memcpy(&jump,thunk+4,4);
        if (int64_t{entry}+8+jump!=0xb95830 ||
            !Read(reader,0xb95830,overrideHead,sizeof(overrideHead)) || std::memcmp(overrideHead,expectedHead,sizeof(expectedHead)) ||
            !Read(reader,0xb9592f,overrideTail,sizeof(overrideTail)) || std::memcmp(overrideTail,expectedTail,sizeof(expectedTail))) { return false; }
    }
    snapshot=candidate;method=entry;return true;
}
}

enum class NativeWindowMutationState { Refused, Completed, Unconfirmed };
struct NativeWindowMutation {
    NativeWindowMutationState state = NativeWindowMutationState::Refused;
    uint32_t child = 0; // Borrowed game object, not a callback or owned allocation.
};

#if defined(_MSC_VER) && defined(_M_IX86)
namespace native_detail {
inline bool OwnedUiThread(HWND game) {
    DWORD process = 0;
    wchar_t name[16]{};
    return game && GetWindowThreadProcessId(game, &process) == GetCurrentThreadId() &&
        process == GetCurrentProcessId() && GetClassNameW(game, name, 16) &&
        (wcscmp(name, L"H5") == 0 || wcscmp(name, L"H5UNI") == 0);
}
inline bool ReadLocal(uint32_t address, void* output, size_t bytes) {
    __try { std::memcpy(output, reinterpret_cast<const void*>(address), bytes); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}

// The stock ITextView setter copies the caller's UTF-16 string and invalidates
// its layout. No pointer into the mod is retained. Only WindowTextView admitted.
inline NativeWindowMutation SetNativeWindowText(HWND game,uint32_t window,const wchar_t* text,uint32_t length) {
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax,floatingState }
    __asm { fxsave [eax] }
    __try {
        NativeWindowMutation result{};
        if (!native_detail::OwnedUiThread(game) || !text || length>16384 ||
            uint64_t{reinterpret_cast<uintptr_t>(text)}+(uint64_t{length}+1)*2>(uint64_t{1}<<32)) { return result; }
        auto reader=native_detail::ReadLocal;
        NativeWindowSnapshot snapshot{};uint32_t type=0,table=0,getter=0,method=0;
        wchar_t terminator=1;
        constexpr unsigned char expected[]{
            0x53,0x55,0x56,0x8b,0xd9,0x8b,0x53,0x14,0x8b,0x43,0x18,0x57,0x8b,0x7c,0x24,0x14,
            0x8b,0x0f,0x8b,0x6f,0x04,0x8d,0x73,0x14,0x2b,0xc2,0x2b,0xe9,0xd1,0xf8,0xd1,0xfd,
            0x3b,0xc5,0x75,0x10,0x50,0x51,0x52,0xff,0x15,0xe0,0xa2,0xe0,0x00,0x83,0xc4,0x0c,
            0x85,0xc0,0x74,0x2b,0x3b,0xfe,0x74,0x0e,0x8b,0x47,0x04,0x8b,0x0f,0x50,0x51,0x8b,
            0xce,0xe8,0x1a,0x8b,0x87,0xff,0x8b,0x93,0x18,0xff,0xff,0xff,0x8b,0x42,0x08,0x8b,
            0x94,0x18,0x18,0xff,0xff,0xff,0x8d,0x8c,0x18,0x18,0xff,0xff,0xff,0xff,0x12,0x5f,
            0x5e,0x5d,0xb0,0x01,0x5b,0xc2,0x04,0x00};
        unsigned char actual[sizeof(expected)]{};
        if (!ReadNativeWindow(reader,window,snapshot) ||
            !native_detail::Word(reader,int64_t{snapshot.sharedAddress}+0x44,type) || type!=285694860 ||
            !native_detail::Word(reader,int64_t{snapshot.objectAddress}+0xf8,table) ||
            !native_detail::Word(reader,table,getter) || getter!=0xb95690 ||
            !native_detail::Word(reader,int64_t{table}+4,method) || method!=0xb95e00 ||
            !native_detail::Read(reader,method,actual,sizeof(actual)) || std::memcmp(actual,expected,sizeof(actual)) ||
            !native_detail::Read(reader,reinterpret_cast<uintptr_t>(text)+uint64_t{length}*2,&terminator,2) || terminator) { return result; }
        const wchar_t* borrowed[]{text,text+length,text+length+1};
        result.child=window;result.state=NativeWindowMutationState::Unconfirmed;
        __try {
            if (!reinterpret_cast<bool(__thiscall*)(uint32_t,const wchar_t* const*)>(method)(snapshot.objectAddress+0xf8,borrowed)) { return result; }
            uint32_t begin=0,end=0;
            if (native_detail::Word(reader,int64_t{snapshot.objectAddress}+0x10c,begin) &&
                native_detail::Word(reader,int64_t{snapshot.objectAddress}+0x110,end) &&
                end>=begin && end-begin==length*2 && (!length ||
                std::memcmp(reinterpret_cast<const void*>(begin),text,length*2)==0)) {
                result.state=NativeWindowMutationState::Completed;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { return result; }
        return result;
    } __finally {
        __asm { lea eax,floatingState }
        __asm { fxrstor [eax] }
    }
}

// Update native placement values in the widget's existing alignment system.
// Mask bits: x=1, y=2, width=4, height=8. Position values are alignment-relative,
// not resolved screen coordinates. The stock method also invalidates layout.
inline NativeWindowMutation SetNativeWindowPlacement(HWND game,uint32_t window,
    float x,float y,float width,float height,uint32_t mask) {
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax,floatingState }
    __asm { fxsave [eax] }
    __try {
        NativeWindowMutation result{};
        if (!native_detail::OwnedUiThread(game) || !mask || mask>15 ||
            !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height) ||
            std::fabs(x)>65536 || std::fabs(y)>65536 || width<0 || height<0 || width>32768 || height>32768) { return result; }
        auto reader=native_detail::ReadLocal;
        NativeWindowSnapshot snapshot{};uint32_t method=0;
        if (!native_detail::PlacementMethod(reader,window,snapshot,method)) { return result; }
        result.child=window;result.state=NativeWindowMutationState::Unconfirmed;
        __try { reinterpret_cast<void(__thiscall*)(uint32_t,float,float,float,float,uint32_t)>(method)(window,x,y,width,height,mask); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return result; }
        float actual[4]{};const float expected[]{x,y,width,height};
        if (!native_detail::Read(reader,int64_t{snapshot.sharedAddress}+0x88,actual,8) ||
            !native_detail::Read(reader,int64_t{snapshot.sharedAddress}+0xa4,actual+2,8)) { return result; }
        for (uint32_t index=0;index<4;++index) {
            if ((mask&(1u<<index)) && actual[index]!=expected[index]) { return result; }
        }
        result.state=NativeWindowMutationState::Completed;return result;
    } __finally {
        __asm { lea eax,floatingState }
        __asm { fxrstor [eax] }
    }
}

// A scrollable container's inherited children are not its scroll items. Register
// one existing text view through IScrollableContainer, then recompute its range.
// Registration renames/reparents the text using stock engine ownership rules.
// A replaced explanation starts at the top, including the content translation.
inline NativeWindowMutation UpdateNativeScrollText(HWND game,uint32_t container,uint32_t text) {
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax,floatingState }
    __asm { fxsave [eax] }
    __try {
        NativeWindowMutation result{};
        if (!native_detail::OwnedUiThread(game)) { return result; }
        auto reader=native_detail::ReadLocal;
        NativeWindowSnapshot parent{},content{};
        uint32_t type=0,contentType=0,table=0,add=0,refresh=0,reset=0,begin=0,end=0,room=0;
        if (!ReadNativeWindow(reader,container,parent) || !ReadNativeWindow(reader,text,content) ||
            !native_detail::Word(reader,int64_t{parent.sharedAddress}+0x44,type) || type!=386593537 ||
            !native_detail::Word(reader,int64_t{content.sharedAddress}+0x44,contentType) || contentType!=285694860 ||
            !native_detail::Word(reader,int64_t{parent.objectAddress}+0xf8,table) ||
            !native_detail::Word(reader,int64_t{table}+8,add) || add!=0x8895c0 ||
            !native_detail::Word(reader,int64_t{table}+12,refresh) || refresh!=0x889590 ||
            !native_detail::Word(reader,int64_t{table}+0x30,reset) || reset!=0x887df0 ||
            !native_detail::Word(reader,int64_t{parent.objectAddress}+0x160,begin) ||
            !native_detail::Word(reader,int64_t{parent.objectAddress}+0x164,end) ||
            !native_detail::Word(reader,int64_t{parent.objectAddress}+0x168,room) ||
            end<begin || room<end || (begin && begin<0x10000) || ((begin|end|room)&3) ||
            (!begin && (end || room)) || room-begin>4096 || (end-begin!=0 && end-begin!=32)) { return result; }
        constexpr unsigned char addHead[]{0x8b,0x44,0x24,0x04,0x83,0xec,0x28,0x85,0xc0};
        constexpr unsigned char addTail[]{0xc2,0x08,0x00};
        constexpr unsigned char refreshHead[]{0x8b,0x41,0x68,0x3b,0x41,0x6c,0x74,0x09};
        constexpr unsigned char resetHead[]{0x56,0x8b,0xf1,0x8b,0x46,0x5c,0x85,0xc0};
        unsigned char actualReset[sizeof(resetHead)]{};
        unsigned char actualAdd[sizeof(addHead)]{},actualTail[sizeof(addTail)]{},actualRefresh[sizeof(refreshHead)]{};
        if (!native_detail::Read(reader,add,actualAdd,sizeof(actualAdd)) || std::memcmp(actualAdd,addHead,sizeof(addHead)) ||
            !native_detail::Read(reader,0x8897e7,actualTail,sizeof(actualTail)) || std::memcmp(actualTail,addTail,sizeof(addTail)) ||
            !native_detail::Read(reader,refresh,actualRefresh,sizeof(actualRefresh)) || std::memcmp(actualRefresh,refreshHead,sizeof(refreshHead)) ||
            !native_detail::Read(reader,reset,actualReset,sizeof(actualReset)) || std::memcmp(actualReset,resetHead,sizeof(resetHead))) { return result; }
        if (end!=begin) {
            uint32_t name=0,nameEnd=0,registeredText=0;
            if (!ReadNativeScrollTextWindow(reader,container,registeredText) || registeredText!=text) { return result; }
            if (!native_detail::Word(reader,begin,name) || !native_detail::Word(reader,int64_t{begin}+4,nameEnd) ||
                nameEnd<name || nameEnd-name!=std::strlen(content.name)) { return result; }
            char existing[128]{};
            if (nameEnd-name>=sizeof(existing) || (nameEnd!=name && !native_detail::Read(reader,name,existing,nameEnd-name)) ||
                std::strcmp(existing,content.name)) { return result; }
        } else {
            std::array<uint32_t,128> children{};uint32_t count=0;
            if (!ReadNativeWindowChildren(reader,container,children,count) ||
                std::find(children.begin(),children.begin()+count,text)==children.begin()+count) { return result; }
        }
        result.child=text;result.state=NativeWindowMutationState::Unconfirmed;
        __try {
            const uint32_t scrollInterface=parent.objectAddress+0xf8;
            if (end==begin) { reinterpret_cast<void(__thiscall*)(uint32_t,uint32_t,uint32_t)>(add)(scrollInterface,text,0); }
            reinterpret_cast<void(__thiscall*)(uint32_t)>(refresh)(scrollInterface);
            reinterpret_cast<void(__thiscall*)(uint32_t)>(reset)(scrollInterface);
        } __except (EXCEPTION_EXECUTE_HANDLER) { return result; }
        uint32_t currentBegin=0,currentEnd=0;
        if (native_detail::Word(reader,int64_t{parent.objectAddress}+0x160,currentBegin) &&
            native_detail::Word(reader,int64_t{parent.objectAddress}+0x164,currentEnd) &&
            currentEnd>=currentBegin && currentEnd-currentBegin==32 && ReadNativeWindow(reader,text,content)) {
            result.state=NativeWindowMutationState::Completed;
        }
        return result;
    } __finally {
        __asm { lea eax,floatingState }
        __asm { fxrstor [eax] }
    }
}

// Stock LoadLayout enables this flag before screen admission. CWindow uses it
// to dispatch default UI bindings and consume otherwise unhandled input.
// It does not register a screen modal or select that screen's UI keybinding group.
inline NativeWindowMutation SetNativeWindowModalInput(HWND game,uint32_t window,bool enabled) {
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax,floatingState }
    __asm { fxsave [eax] }
    __try {
        NativeWindowMutation result{};
        if (!native_detail::OwnedUiThread(game)) { return result; }
        auto reader=native_detail::ReadLocal;
        NativeWindowSnapshot snapshot{};uint32_t common=0,table=0,method=0;unsigned char value=0;
        constexpr unsigned char expected[]{0x8a,0x44,0x24,0x04,0x88,0x81,0x08,0xff,0xff,0xff,0xc2,0x04,0x00};
        unsigned char actual[sizeof(expected)]{};
        if (!ReadNativeWindow(reader,window,snapshot) ||
            !native_detail::CommonBase(reader,window,0x10,0x48e7a0,common) ||
            !native_detail::Word(reader,window,table) || !native_detail::Word(reader,int64_t{table}+0x10,method) ||
            !native_detail::Read(reader,0x48e7a0,actual,sizeof(actual)) || std::memcmp(actual,expected,sizeof(actual)) ||
            !native_detail::Read(reader,int64_t{common}-0xf8,&value,1) || value>1) { return result; }
        result.child=window;result.state=NativeWindowMutationState::Unconfirmed;
        __try { reinterpret_cast<void(__thiscall*)(uint32_t,bool)>(method)(window,enabled); }
        __except(EXCEPTION_EXECUTE_HANDLER) { return result; }
        if (native_detail::Read(reader,int64_t{common}-0xf8,&value,1) && value==static_cast<unsigned char>(enabled)) {
            result.state=NativeWindowMutationState::Completed;
        }
        return result;
    } __finally {
        __asm { lea eax,floatingState }
        __asm { fxrstor [eax] }
    }
}

inline NativeWindowMutation SetNativeWindowVisible(HWND game,uint32_t window,bool visible) {
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax,floatingState }
    __asm { fxsave [eax] }
    __try {
        NativeWindowMutation result{};
        if (!native_detail::OwnedUiThread(game)) { return result; }
        auto reader=native_detail::ReadLocal;
        NativeWindowSnapshot snapshot{};uint32_t common=0,table=0,method=0;
        if (!ReadNativeWindow(reader,window,snapshot) ||
            !native_detail::CommonBase(reader,window,0x44,0xa50040,common) ||
            !native_detail::Word(reader,window,table) || !native_detail::Word(reader,int64_t{table}+0x44,method)) { return result; }
        result.child=window;result.state=NativeWindowMutationState::Unconfirmed;
        __try { reinterpret_cast<void(__thiscall*)(uint32_t,bool)>(method)(window,visible); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return result; }
        unsigned char actual=0;
        if (native_detail::Read(reader,int64_t{snapshot.sharedAddress}+0x70,&actual,1) && actual==static_cast<unsigned char>(visible)) {
            result.state=NativeWindowMutationState::Completed;
        }
        return result;
    } __finally {
        __asm { lea eax,floatingState }
        __asm { fxrstor [eax] }
    }
}

// Consume an engine-owned button's state transition. Resource commands and
// business meaning belong to the caller; no callback into a mod is registered.
inline NativeWindowMutation ConsumeNativeButtonState(HWND game,uint32_t window,uint32_t expected,uint32_t reset) {
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax,floatingState }
    __asm { fxsave [eax] }
    __try {
        NativeWindowMutation result{};
        if (!native_detail::OwnedUiThread(game)) { return result; }
        auto reader=native_detail::ReadLocal;
        NativeWindowSnapshot snapshot{};uint32_t type=0,state=0,table=0,method=0;
        if (!ReadNativeWindow(reader,window,snapshot) ||
            !native_detail::Word(reader,int64_t{snapshot.sharedAddress}+0x44,type) || type!=285694855 ||
            !native_detail::Word(reader,int64_t{snapshot.sharedAddress}+0x114,state) || state!=expected ||
            !native_detail::Word(reader,int64_t{snapshot.objectAddress}+0xf8,table) ||
            !native_detail::Word(reader,int64_t{table}+12,method)) { return result; }
        constexpr unsigned char head[]{0x83,0xec,0x10,0x53,0x55,0x56,0x8b,0xf1};
        unsigned char actual[sizeof(head)]{};
        if (!native_detail::Read(reader,method,actual,sizeof(actual)) || std::memcmp(actual,head,sizeof(actual))) { return result; }
        result.child=window;result.state=NativeWindowMutationState::Unconfirmed;
        __try { reinterpret_cast<void(__thiscall*)(uint32_t,uint32_t)>(method)(snapshot.objectAddress+0xf8,reset); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return result; }
        if (native_detail::Word(reader,int64_t{snapshot.sharedAddress}+0x114,state) && state==reset) {
            result.state=NativeWindowMutationState::Completed;
        }
        return result;
    } __finally {
        __asm { lea eax,floatingState }
        __asm { fxrstor [eax] }
    }
}

// Clone an existing native widget definition through the stock factory, attach
// to parent and run stock initialization/layout. Caller owns authorization and
// the parent's lifetime. The parent owns the child. No mod callback is retained.
// Unconfirmed means effects may already exist: inspect the tree, never replay.
inline NativeWindowMutation CreateNativeWindowChild(HWND game, uint32_t parent, uint32_t model) {
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax, floatingState }
    __asm { fxsave [eax] }
    __try {
        NativeWindowMutation result{};
        if (!native_detail::OwnedUiThread(game)) { return result; }
        auto reader = native_detail::ReadLocal;
        NativeWindowSnapshot source{};
        std::array<uint32_t, 128> before{}, after{};
        uint32_t beforeCount = 0, afterCount = 0, common = 0, type = 0;
        constexpr unsigned char expected[]{
            0x56,0x8b,0xf2,0x85,0xf6,0x57,0x8b,0xf9,0x74,0x18,0xb9,0x00,0xbc,0x09,0x11,
            0xe8,0x2c,0xd5,0xff,0xff,0x8b,0x10,0x56,0x8b,0xc8,0xff,0x52,0x28,0x8b,0xf0,
            0x85,0xf6,0x75,0x05,0x5f,0x33,0xc0,0x5e,0xc3,0x85,0xff,0x74,0x0d,0x8b,0x07,
            0x6a,0x01,0x56,0x8b,0xcf,0xff,0x90,0x8c,0x00,0x00,0x00,0x8b,0x16,0x8b,0xce,
            0xff,0x92,0xc8,0x00,0x00,0x00,0x8b,0x06,0x8b,0xce,0xff,0x90,0xb0,0x00,0x00,
            0x00,0x5f,0x8b,0xc6,0x5e,0xc3};
        unsigned char actual[sizeof(expected)]{};
        if (!ReadNativeWindow(reader, model, source) ||
            !native_detail::Word(reader, int64_t{source.sharedAddress} + 0x44, type) ||
            (type != 285694855 && type != 285700801 && type != 285694860) ||
            !ReadNativeWindowChildren(reader, parent, before, beforeCount) || beforeCount >= before.size() ||
            !native_detail::CommonBase(reader, parent, 0x8c, 0xa515b0, common) ||
            !native_detail::Read(reader, 0xa4d510, actual, sizeof(actual)) ||
            std::memcmp(actual, expected, sizeof(actual))) { return result; }
        result.state = NativeWindowMutationState::Unconfirmed;
        bool returned = false;
        __try {
            result.child = reinterpret_cast<uint32_t(__fastcall*)(uint32_t,uint32_t)>(0xa4d510)(parent, source.sharedAddress);
            returned = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { /* Unknown effect; do not retry. */ }
        if (!returned) { return result; }
        NativeWindowSnapshot created{};
        if (result.child && ReadNativeWindow(reader, result.child, created) &&
            created.sharedAddress != source.sharedAddress && std::strcmp(created.name,source.name) == 0 &&
            ReadNativeWindowChildren(reader, parent, after, afterCount) && afterCount == beforeCount + 1 &&
            std::count(after.begin(),after.begin()+afterCount,result.child) == 1 &&
            std::all_of(before.begin(),before.begin()+beforeCount,[&](uint32_t child) {
                return std::find(after.begin(),after.begin()+afterCount,child) != after.begin()+afterCount;
            })) { result.state = NativeWindowMutationState::Completed; }
        return result;
    } __finally {
        __asm { lea eax, floatingState }
        __asm { fxrstor [eax] }
    }
}

// Stock removal may be queued by the game. Completion is returned only when
// the child is already absent and every other direct child remains present.
inline NativeWindowMutation RemoveNativeWindowChild(HWND game, uint32_t parent, uint32_t child) {
    alignas(16) unsigned char floatingState[512];
    __asm { lea eax, floatingState }
    __asm { fxsave [eax] }
    __try {
        NativeWindowMutation result{};
        if (!native_detail::OwnedUiThread(game)) { return result; }
        auto reader = native_detail::ReadLocal;
        std::array<uint32_t,128> before{}, after{};
        uint32_t beforeCount = 0, afterCount = 0, common = 0, table = 0, remove = 0;
        constexpr unsigned char expected[]{0x8b,0x44,0x24,0x04,0x56,0x57,0x6a,0x00,
            0x68,0x88,0xda,0xf2,0x00,0x68,0xf4,0xda};
        unsigned char actual[sizeof(expected)]{};
        if (!ReadNativeWindowChildren(reader,parent,before,beforeCount) ||
            std::find(before.begin(),before.begin()+beforeCount,child) == before.begin()+beforeCount ||
            !native_detail::CommonBase(reader,parent,0xe4,0xa51740,common) ||
            !native_detail::Read(reader,0xa51740,actual,sizeof(actual)) || std::memcmp(actual,expected,sizeof(actual)) ||
            !native_detail::Word(reader,parent,table) || !native_detail::Word(reader,int64_t{table}+0xe4,remove)) { return result; }
        result.state = NativeWindowMutationState::Unconfirmed; result.child = child;
        bool returned = false;
        __try { reinterpret_cast<void(__thiscall*)(uint32_t,uint32_t)>(remove)(parent,child); returned = true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { /* Retain the handle for observation. */ }
        if (!returned) { return result; }
        if (ReadNativeWindowChildren(reader,parent,after,afterCount) && afterCount + 1 == beforeCount &&
            std::find(after.begin(),after.begin()+afterCount,child) == after.begin()+afterCount &&
            std::all_of(after.begin(),after.begin()+afterCount,[&](uint32_t other) {
                return std::find(before.begin(),before.begin()+beforeCount,other) != before.begin()+beforeCount;
            })) { result.state = NativeWindowMutationState::Completed; }
        return result;
    } __finally {
        __asm { lea eax, floatingState }
        __asm { fxrstor [eax] }
    }
}

#endif
}
