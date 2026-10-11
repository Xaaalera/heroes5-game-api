#include <h5/resident_buffers.hpp>
#ifndef RESIDENT_VALUE
#define RESIDENT_VALUE 101
#endif
extern "C" uint32_t __stdcall WriteResidentBuffer(HWND window) {
    if (!h5::resident::EnsureBuffer(window, 1, 1, 65536)) { return 0; }
    h5::resident::BufferLease lease(window, 1, 1, true);
    if (!lease.WritableData()) { return 0; }
    *reinterpret_cast<uint32_t*>(lease.WritableData()) = RESIDENT_VALUE;
    return 1;
}
#pragma comment(linker, "/EXPORT:WriteResidentBuffer=_WriteResidentBuffer@4")
extern "C" uint32_t __stdcall WriteSecondResidentBuffer(HWND window) {
    if (!h5::resident::EnsureBuffer(window, 2, 1, 32)) { return 0; }
    h5::resident::BufferLease lease(window, 2, 1, true);
    if (!lease.WritableData()) { return 0; }
    *reinterpret_cast<uint32_t*>(lease.WritableData()) = RESIDENT_VALUE;
    return 1;
}
extern "C" uint32_t __stdcall InvokeNestedResidentWriter(HWND window, uint32_t (__stdcall* nested)(HWND)) {
    h5::resident::BufferLease lease(window, 1, 1, true);
    if (!lease.WritableData() || !nested) { return 0; }
    const auto original = *reinterpret_cast<uint32_t*>(lease.WritableData());
    return nested(window) == 1 && *reinterpret_cast<uint32_t*>(lease.WritableData()) == original ? 1u : 0u;
}
#pragma comment(linker, "/EXPORT:WriteSecondResidentBuffer=_WriteSecondResidentBuffer@4")
#pragma comment(linker, "/EXPORT:InvokeNestedResidentWriter=_InvokeNestedResidentWriter@8")
extern "C" uint32_t __stdcall InvokeReadHeldUpgrade(HWND window, uint32_t (__stdcall* nested)(HWND)) {
    h5::resident::BufferLease lease(window, 1, 1, false);
    if (!lease.Data() || !nested) { return 0; }
    const auto original = *reinterpret_cast<const uint32_t*>(lease.Data());
    return nested(window) == 0 && *reinterpret_cast<const uint32_t*>(lease.Data()) == original ? 1u : 0u;
}
#pragma comment(linker, "/EXPORT:InvokeReadHeldUpgrade=_InvokeReadHeldUpgrade@8")
