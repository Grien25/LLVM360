# LLVM360 To-Do List

This document outlines the major tasks and goals for the future development of the llvm360 project.

### Core Infrastructure & Refactoring
- [ ] Refactor the overall project structure based on the new design principles.
- [ ] Remove the dependency on the thread-local storage (TLS) variable for the CPU context, passing it as a function parameter instead.
- [ ] Improve CMake build scripts to automatically locate LLVM libraries instead of using a hardcoded path.

### Recompiler (`Naive+`) Enhancements
- [ ] **Expand Instruction Coverage:**
    - [ ] Implement instruction emitters for the remaining VMX (AltiVec) operations.
    - [ ] Add support for more floating-point (single and double precision) instructions.
- [ ] **Improve Control Flow Analysis:**
    - [ ] Enhance jump table detection to correctly identify all targets for `bcctr` instructions.
    - [ ] Refine logic for detecting function boundaries, especially for functions that do not have standard prologues or epilogues.
    - [ ] Improve tail-call detection to correctly identify and optimize them.

### Runtime (`Emulator`) Implementation
- [ ] **Kernel (`XboxKrnl`) Implementation (High Priority):**
    - [ ] Move beyond stubs for critical kernel functions. Start with a focus on (sub-priorities indicated):
        - [ ] [P0] **Thread Management:** Fully implement `XeThread` and synchronization primitives like `RtlEnterCriticalSection` and `NtWaitForSingleObjectEx`.
        - [ ] [P0] **Memory Management:** Implement `NtFreeVirtualMemory` and `NtQueryVirtualMemory`.
        - [ ] [P1] **File I/O:** Implement a basic file system to handle operations like `NtCreateFile`, `NtReadFile`, and `NtQueryInformationFile`.
            - [ ] Implement `NtCreateFile` to handle basic file opening and creation (read-only, create-if-not-exists).
            - [ ] Implement `NtReadFile` for sequential reads from an open handle.
            - [ ] [good-first-issue] Implement `NtClose` to release file handles and return `STATUS_SUCCESS` for valid handles.
- [ ] **XAM Module Implementation:**
    - [ ] Begin implementing core XAM functions required for user interface and system notifications, beyond the current `XamLoaderTerminateTitle` stub.
        - [ ] [good-first-issue] Ensure `XamLoaderTerminateTitle` returns a sensible success status and performs minimal cleanup.
- [ ] **Loader & Module Handling:**
    - [ ] Properly handle imported variables (`XexExecutableModuleHandle`, `XboxHardwareInfo`) by allocating and initializing their corresponding structures in the runtime.
        - [ ] [good-first-issue] Initialize `XboxHardwareInfo` with static, sane defaults until a full implementation exists.

### Graphics & Debugging
- [ ] **DirectX 12 Backend:**
    - [ ] Flesh out the `DX12Manager` to initialize a device and swap chain capable of rendering a frame.
- [ ] **Debugger UI:**
    - [ ] Fully integrate the `ImGuiDebugger` with the runtime.
    - [ ] Create UI windows to display the live `XenonState` (CPU registers), memory contents, and instruction traces.

### Long-Term Goals
- [ ] Achieve the primary goal of recompiling and running `xboxkrnl.exe` itself within the runtime.
- [ ] Begin low-level emulation of the Xenos GPU, translating graphics commands to the DirectX 12 backend.
- [ ] Test the full recompiler-runtime pipeline with a simple homebrew application before moving to commercial games.

### Good First Issues (Summary)
- [ ] `NtClose`: Minimal handle validation and `STATUS_SUCCESS` path.
- [ ] `XamLoaderTerminateTitle`: Return success and no-op cleanup.
- [ ] `XboxHardwareInfo` initialization: Fill with static defaults behind a feature flag.
