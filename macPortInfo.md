# macOS Porting Notes for LLVM360

This document summarizes the Windows-specific code in the repo that will block macOS builds, along with recommended fixes and current progress.

## Problem Areas (by category)

- Headers & Win32 APIs:
  - `windows.h` and Win32 usage should be guarded or replaced.
    - Files: `llvm360/Emulator/src/Runtime/Runtime.h`, `llvm360/Emulator/src/Runtime/MemoryManager.h`, `llvm360/Emulator/src/pch.h`, `llvm360/Emulator/src/Graphics/DX12Manager.h`, `llvm360/Naive+/src/IR/IRGenerator.h`, `llvm360/Naive+/src/Xex/XexLoader.cpp`.
  - Dynamic loading (`GetModuleHandleW`/`GetProcAddress`) needs a `dlopen(nullptr)/dlsym` path on macOS.
    - File: `llvm360/Emulator/src/Runtime/Runtime.cpp`.
  - Win32 input/windowing calls appear in the debug UI (WndProc, window creation, message pump); remove or guard for macOS.
    - Files: `llvm360/Emulator/src/Graphics/ImGuiDebugger.h/.cpp`, `llvm360/Emulator/src/Graphics/ImGuiTest.h`.

- Calling conventions & visibility:
  - `__cdecl` and `__declspec(dllexport)` are MSVC/Windows specific.
    - Files: `llvm360/Emulator/src/Runtime/Runtime.h`, `llvm360/Emulator/src/Runtime/Xen/XenUtils.h`.
  - Define portability macros to make them no-ops or visibility attributes on macOS.

- Graphics (DX12/COM):
  - DirectX 12 (`d3d12.h`, `dxgi*`, `wrl.h`) is Windows-only.
    - Files: `llvm360/Emulator/src/Graphics/DX12Manager.*`.
  - Solution: keep DX12 under `#if _WIN32`; add a Metal backend via Plume on macOS (adapter scaffold added).

- Memory management:
  - `VirtualAlloc`/`VirtualFree` must be replaced with `mmap`/`munmap`/`mprotect` on macOS.
    - File: `llvm360/Emulator/src/Runtime/MemoryManager.h`.

- DLL entry point:
  - `DllMain` has no macOS equivalent; initialization should be called from the host process explicitly.
    - File: `llvm360/Emulator/src/dllmain.cpp`.

- File I/O & Unicode:
  - `_wfopen_s` used in the XEX loader is Windows-specific.
    - Fixed: cross‑platform UTF‑8 conversion helper added.
    - File: `llvm360/Naive+/src/Xex/XexLoader.cpp`.

- Debugging intrinsics:
  - `DebugBreak()` is Windows-specific.
    - Fixed: non‑Windows fallback uses `__builtin_trap()` in `IRGenerator.h`; other call sites should include that header or a common platform header.

- C++ library compatibility:
  - `std::wstring_convert` (<codecvt>) is deprecated but available on Apple libc++; keep an eye on toolchain.
  - Avoid `std::format` on older Apple toolchains (not currently used).

- External folders (do not build on macOS):
  - ImGui examples/backends and MarathonRecomp have Win32 code; ensure CMake excludes them on macOS.
  - Plume D3D12 sources are Windows-only; Metal sources are macOS-friendly.

## Current Cross‑Platform Work Done

- Root CMake discovers LLVM via Homebrew on macOS (`llvm-config` or common brew paths). LLD is optional.
- Emulator only builds on Windows; Naive+ builds on macOS.
- Introduced `IGraphicsBackend` + factory with platform selection:
  - Windows: DX12 backend adapter wraps `DirectX12Manager`.
  - macOS: Plume (Metal) adapter scaffold added; to be wired when Plume is vendored.
- `XexLoader` now uses a cross‑platform `_wfopen` replacement (UTF‑8 fopen on macOS).
- Non‑Windows `DebugBreak` fallback via `__builtin_trap()`.

## Recommended Fix Plan

1) Platform header
   - Add `platform.h` defining:
     - `XPL_EXPORT` → `__declspec(dllexport)` (Windows) / `__attribute__((visibility("default")))` (macOS)
     - `XPL_CDECL` → `__cdecl` (Windows) / empty (macOS)
     - `XPL_DEBUGBREAK()` → `DebugBreak()` (Windows) / `__builtin_trap()` (macOS)
   - Replace direct `__cdecl`/`__declspec`/`DebugBreak` uses with these macros.

2) Memory manager (macOS)
   - Implement `XAlloc` POSIX path using `mmap/munmap/mprotect`.
   - Map Xbox protection flags to POSIX (`PROT_READ/WRITE/EXEC`).
   - Keep Windows path with `VirtualAlloc` under `#if _WIN32`.

3) Runtime symbol lookup (macOS)
   - Replace `GetModuleHandleW`/`GetProcAddress` with `dlopen(nullptr, RTLD_LAZY)` and `dlsym` for exported glue symbols (`getXCtxAddress`, `X_FunctionArray`, etc.).

4) Debugger input/windowing
   - Remove `GetAsyncKeyState` use; query ImGui IO for key state or add a small abstraction.
   - Gate Win32 WND/MSG pump behind `#if _WIN32`; provide a no-op or GLFW/SDL path on macOS if needed.

5) Graphics backends
   - Keep DX12 sources under Windows only.
   - Vendor Plume and wire `PlumeBackendAdapter` to real Metal init/swapchain; add CMake option to select backends (defaults: Windows=DX12, macOS=Metal).

6) Build hygiene
   - Ensure external Win32-heavy examples aren’t built on macOS.
   - Keep `Emulator` excluded on macOS until memory manager + runtime loader are ready.

## Quick File Reference

- Win32 headers to guard:
  - `llvm360/Emulator/src/Runtime/Runtime.h`
  - `llvm360/Emulator/src/Runtime/MemoryManager.h`
  - `llvm360/Emulator/src/pch.h`
  - `llvm360/Emulator/src/Graphics/DX12Manager.h`
  - `llvm360/Naive+/src/IR/IRGenerator.h`
  - `llvm360/Naive+/src/Xex/XexLoader.cpp`

- Dynamic loading to adapt:
  - `llvm360/Emulator/src/Runtime/Runtime.cpp`

- DX12/Win32 UI (Windows only):
  - `llvm360/Emulator/src/Graphics/DX12Manager.*`
  - `llvm360/Emulator/src/Graphics/ImGuiDebugger.*`
  - `llvm360/Emulator/src/Graphics/ImGuiTest.h`

- Calling convention/export macros:
  - `llvm360/Emulator/src/Runtime/Runtime.h`
  - `llvm360/Emulator/src/Runtime/Xen/XenUtils.h`

## Notes on Plume (Metal)

- The `PlumeBackendAdapter` is in place and returns from `CreateGraphicsBackend()` on macOS.
- Once Plume is integrated (submodule or `find_package`), implement init, frame render, and cleanup in:
  - `llvm360/Emulator/src/Graphics/PlumeBackendAdapter.cpp`

