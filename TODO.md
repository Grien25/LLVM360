# LLVM360 To-Do List

This document outlines the major tasks and goals for the future development of the llvm360 project.

### Core Infrastructure & Refactoring
- [ ] Refactor the overall project structure based on the new design principles.
- [ ] Remove the dependency on the thread-local storage (TLS) variable for the CPU context, passing it as a function parameter instead.
- [ ] [cite_start]Improve CMake build scripts to automatically locate LLVM libraries instead of using a hardcoded path[cite: 14].

### Recompiler (`Naive+`) Enhancements
- [ ] **Expand Instruction Coverage:**
    - [ ] [cite_start]Implement instruction emitters for the remaining VMX (AltiVec) operations[cite: 678, 711, 739].
    - [ ] [cite_start]Add support for more floating-point (single and double precision) instructions[cite: 904, 922].
- [ ] **Improve Control Flow Analysis:**
    - [ ] [cite_start]Enhance jump table detection to correctly identify all targets for `bcctr` instructions [cite: 588, 1207-1212].
    - [ ] [cite_start]Refine logic for detecting function boundaries, especially for functions that do not have standard prologues or epilogues[cite: 563, 591].
    - [ ] [cite_start]Improve tail-call detection to correctly identify and optimize them[cite: 541, 604].

### Runtime (`Emulator`) Implementation
- [ ] **Kernel (`XboxKrnl`) Implementation (High Priority):**
    - [ ] Move beyond stubs for critical kernel functions. Start with a focus on:
        - [ ] [cite_start]**Thread Management:** Fully implement `XeThread` and synchronization primitives like `RtlEnterCriticalSection` and `NtWaitForSingleObjectEx`[cite: 4, 262, 267, 279].
        - [ ] [cite_start]**Memory Management:** Implement `NtFreeVirtualMemory` and `NtQueryVirtualMemory`[cite: 273, 277].
        - [ ] [cite_start]**File I/O:** Implement a basic file system to handle operations like `NtCreateFile`, `NtReadFile`, and `NtQueryInformationFile`[cite: 283, 285, 284].
- [ ] **XAM Module Implementation:**
    - [ ] [cite_start]Begin implementing core XAM functions required for user interface and system notifications, beyond the current `XamLoaderTerminateTitle` stub[cite: 263, 264].
- [ ] **Loader & Module Handling:**
    - [ ] [cite_start]Properly handle imported variables (`XexExecutableModuleHandle`, `XboxHardwareInfo`) by allocating and initializing their corresponding structures in the runtime [cite: 234-239].

### Graphics & Debugging
- [ ] **DirectX 12 Backend:**
    - [ ] [cite_start]Flesh out the `DX12Manager` to initialize a device and swap chain capable of rendering a frame[cite: 2, 36].
- [ ] **Debugger UI:**
    - [ ] Fully integrate the `ImGuiDebugger` with the runtime.
    - [ ] [cite_start]Create UI windows to display the live `XenonState` (CPU registers), memory contents, and instruction traces [cite: 3, 20-24].

### Long-Term Goals
- [ ] Achieve the primary goal of recompiling and running `xboxkrnl.exe` itself within the runtime.
- [ ] Begin low-level emulation of the Xenos GPU, translating graphics commands to the DirectX 12 backend.
- [ ] Test the full recompiler-runtime pipeline with a simple homebrew application before moving to commercial games.