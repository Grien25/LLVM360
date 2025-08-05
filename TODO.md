# LLVM360 TODO List

This list tracks the major tasks, improvements, and roadmap for LLVM360. See the README and comprehensive report for more context. Contributions and suggestions are welcome!

## Core Recompiler & Analysis
- [ ] Complete PowerPC instruction set coverage (VMX, floating-point, special instructions)
- [ ] Expand and test IR generation for all instruction formats
- [ ] Improve support for additional Xbox 360 binary formats (XEX, ELF variants)
- [ ] Integrate more robust error handling and logging

## Runtime & OS Emulation
- [ ] Implement missing Xbox 360 kernel (XboxKrnl) and XAM system calls (threading, file I/O, etc.)
- [ ] Replace thread-local context hack with explicit thread context management
- [ ] Complete multi-threading and synchronization primitives
- [ ] Improve memory manager and virtual address space simulation

## Graphics & Audio
- [ ] Decide on approach for GPU/graphics emulation (API call translation, stubbing, or Xenia integration)
- [ ] Implement basic graphics/audio API handling or pass-through
- [ ] Explore integration with Xenia or other emulators for GPU/audio subsystems

## Optimization & Stability
- [ ] Implement more aggressive LLVM optimization passes tailored for recompiled code
- [ ] Test and debug output executables for stability (target real games as test cases)
- [ ] Add support for game-specific behaviors and undocumented system calls

## Tooling & User Interface
- [ ] Develop ImGui-based GUI for project management, code browsing, and debugging
- [ ] Add user-friendly front-end for selecting binaries and configuring recompilation
- [ ] Enhance memory viewer, symbol browser, and add search/filter features
- [ ] Add scripting or automation support (e.g., Python integration)

## Build System & Dependencies
- [ ] Streamline CMake configuration for easier setup (Windows, Linux)
- [ ] Update and document external dependencies (Dear ImGui, Xenia, LLVM, etc.)
- [ ] Add CI/CD pipelines for automated builds and tests

## Documentation & Community
- [ ] Expand README with usage examples, screenshots, and architecture diagrams
- [ ] Write developer guides for adding new features or backends
- [ ] Document code structure and module responsibilities
- [ ] Organize public issue tracking and encourage community testing

## Stretch Goals
- [ ] Integrate with Xenia for live debugging and state inspection
- [ ] Add support for additional platforms or architectures
- [ ] Explore cloud-based analysis or remote debugging features
