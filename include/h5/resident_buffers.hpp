#pragma once
#include <windows.h>
#include <array>
#include <cstdint>

namespace h5::resident {
// The registry retains no destructor or callback into a replaceable DLL. Host
// data must follow the same rule. Allocations live until process exit: limits
// apply per HWND registry, so the SDK must bind one stable game window.
inline constexpr wchar_t BufferProperty[] = L"Heroes5SDK.ResidentBuffers.v1";
inline constexpr uint32_t BufferCapacity = 16;
inline constexpr uint32_t MaximumBufferBytes = 1024 * 1024;
namespace detail {
struct BufferSlot {
    uint32_t key = 0;
    uint32_t schema = 0;
    uint32_t bytes = 0;
    unsigned char* data = nullptr;
    SRWLOCK lock = SRWLOCK_INIT;
    volatile LONG writerThread = 0;
    volatile LONG ownerReaders = 0;
};
struct BufferRegistry {
    uint32_t magic = 0x42523548;
    uint32_t version = 2;
    uint32_t size = sizeof(BufferRegistry);
    DWORD process = GetCurrentProcessId();
    HWND window = nullptr;
    SRWLOCK lock = SRWLOCK_INIT;
    std::array<BufferSlot, BufferCapacity> slots{};
};
inline bool PrivateWritable(const void* address, size_t bytes) {
    MEMORY_BASIC_INFORMATION region{};
    const auto begin = reinterpret_cast<uintptr_t>(address);
    return address && bytes && begin + bytes >= begin &&
        VirtualQuery(address, &region, sizeof(region)) &&
        region.State == MEM_COMMIT && region.Type == MEM_PRIVATE &&
        region.Protect == PAGE_READWRITE && region.AllocationBase == address &&
        bytes <= region.RegionSize;
}
inline bool OwnedWindow(HWND window, bool requireThread) {
    DWORD process = 0;
    const auto thread = GetWindowThreadProcessId(window, &process);
    return thread && process == GetCurrentProcessId() &&
        (!requireThread || thread == GetCurrentThreadId());
}
inline BufferRegistry* FindBuffers(HWND window) {
    if (!OwnedWindow(window, false)) { return nullptr; }
    auto* registry = reinterpret_cast<BufferRegistry*>(GetPropW(window, BufferProperty));
    if (!PrivateWritable(registry, sizeof(BufferRegistry)) ||
        registry->magic != 0x42523548 || registry->version != 2 ||
        registry->size != sizeof(BufferRegistry) || registry->window != window ||
        registry->process != GetCurrentProcessId()) { return nullptr; }
    return registry;
}
}
inline bool EnsureBuffer(HWND window, uint32_t key, uint32_t schema, uint32_t bytes) {
    using namespace detail;
    if (!key || !schema || !bytes || bytes > MaximumBufferBytes || !OwnedWindow(window, true)) {
        return false;
    }
    auto* registry = FindBuffers(window);
    if (!registry) {
        if (GetPropW(window, BufferProperty)) { return false; }
        registry = static_cast<BufferRegistry*>(VirtualAlloc(nullptr, sizeof(BufferRegistry),
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (!registry) { return false; }
        *registry = BufferRegistry{};
        registry->window = window;
        if (!SetPropW(window, BufferProperty, registry)) {
            VirtualFree(registry, 0, MEM_RELEASE);
            return false;
        }
    }
    AcquireSRWLockExclusive(&registry->lock);
    BufferSlot* available = nullptr;
    BufferSlot* result = nullptr;
    for (auto& slot : registry->slots) {
        if (slot.key == key) {
            if (slot.schema == schema && slot.bytes == bytes && PrivateWritable(slot.data, bytes)) {
                result = &slot;
            }
            ReleaseSRWLockExclusive(&registry->lock);
            return result != nullptr;
        }
        if (!slot.key && !available) { available = &slot; }
    }
    if (available) {
        auto* data = static_cast<unsigned char*>(VirtualAlloc(nullptr, bytes,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (data) { *available = {key, schema, bytes, data}; result = available; }
    }
    ReleaseSRWLockExclusive(&registry->lock);
    return result != nullptr;
}

// A lease borrows bytes only for one synchronous host operation. Do not store
// its data pointer in PluginSnapshot or retain it in a swappable payload.
// A host may explicitly reserve immutable native-only data for game objects:
// the reservation must have compile-time non-overlap/bounds, never be reset,
// migrated or reused, and contain no addresses into replaceable modules. The
// native consumer may retain that permanent address; payload snapshots may not.
// Consumers restore native object bindings while retaining object ownership.
class BufferLease {
    detail::BufferRegistry* registry_ = nullptr;
    detail::BufferSlot* slot_ = nullptr;
    bool writable_ = false;
    bool ownerRead_ = false;
public:
    BufferLease(HWND window, uint32_t key, uint32_t schema, bool writable) : writable_(writable) {
        registry_ = detail::FindBuffers(window);
        if (!registry_ || (writable && !detail::OwnedWindow(window, true))) { registry_ = nullptr; return; }
        AcquireSRWLockShared(&registry_->lock);
        if (GetPropW(window, BufferProperty) == registry_) {
            for (auto& slot : registry_->slots) {
                if (slot.key == key && slot.schema == schema && detail::PrivateWritable(slot.data, slot.bytes)) {
                    slot_ = &slot;
                    break;
                }
            }
        }
        ReleaseSRWLockShared(&registry_->lock);
        if (!slot_) { registry_ = nullptr; return; }
        // Slot metadata and allocations never move or retire. Do not hold the
        // registry lock through payload code: another core may use another slot.
        if (InterlockedCompareExchange(&slot_->writerThread, 0, 0) == static_cast<LONG>(GetCurrentThreadId()) ||
            (writable && InterlockedCompareExchange(&slot_->ownerReaders, 0, 0) != 0)) {
            slot_ = nullptr; registry_ = nullptr; return;
        }
        if (writable) {
            AcquireSRWLockExclusive(&slot_->lock);
            InterlockedExchange(&slot_->writerThread, static_cast<LONG>(GetCurrentThreadId()));
        } else if (detail::OwnedWindow(window, true)) {
            ownerRead_ = true;
            if (InterlockedIncrement(&slot_->ownerReaders) == 1) { AcquireSRWLockShared(&slot_->lock); }
        } else { AcquireSRWLockShared(&slot_->lock); }
    }
    BufferLease(const BufferLease&) = delete;
    BufferLease& operator=(const BufferLease&) = delete;
    ~BufferLease() {
        if (!registry_) { return; }
        if (writable_) {
            InterlockedExchange(&slot_->writerThread, 0);
            ReleaseSRWLockExclusive(&slot_->lock);
        } else if (ownerRead_) {
            if (InterlockedDecrement(&slot_->ownerReaders) == 0) { ReleaseSRWLockShared(&slot_->lock); }
        } else { ReleaseSRWLockShared(&slot_->lock); }
    }
    const unsigned char* Data() const { return slot_ ? slot_->data : nullptr; }
    unsigned char* WritableData() { return writable_ && slot_ ? slot_->data : nullptr; }
    uint32_t Bytes() const { return slot_ ? slot_->bytes : 0; }
};
}
