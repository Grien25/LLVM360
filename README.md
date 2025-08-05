# LLVM360 Project

LLVM360 is an experimental static recompiler for Xbox 360 software (games and OS), aiming to translate PowerPC machine code from Xbox 360 binaries into native Windows executables. Unlike traditional emulators, LLVM360 performs ahead-of-time (AOT) recompilation, producing optimized x86-64 binaries that run natively on PC. The project leverages the LLVM compiler framework for code translation and optimization, and implements a partial Xbox 360 runtime environment for compatibility.

## Project Overview

- **Goal:** Statically recompile Xbox 360 executables (XEX files) into Windows .exe files, enabling games and system software to run on PC without runtime interpretation.
- **Approach:** Parse and decode PowerPC instructions, translate them to LLVM IR, optimize and compile to x86-64, and link with a custom runtime that emulates Xbox 360 OS calls.
- **Status:** Core pipeline is functional—XEX loading, instruction decoding, IR generation, and native binary output are implemented. Many system calls and advanced CPU features are still in progress.

## Major Components

- **llvm360/Naive+/**: Core recompiler—PowerPC instruction decoder, IR generator, and translation logic.
- **llvm360/Emulator/**: Runtime environment for the recompiled code, including memory management and Xbox 360 OS call emulation (HLE approach).
- **external/imgui/**: [Dear ImGui](https://github.com/ocornut/imgui) for GUI tools and debugging interfaces.
- **external/recompiler/**: Standalone GUI tool for project management, memory viewing, and symbol navigation.
- **external/xenia/**: [Xenia](https://github.com/xenia-project/xenia) Xbox 360 emulator, included for reference and possible backend support.
- **CMakeLists.txt / CMakeSettings.json**: CMake-based build system for all components.
- **compile.bat**: Batch script for building the project on Windows.

## Features

- Load and parse Xbox 360 XEX binaries (with code borrowed from prior recompiler projects).
- Decode and translate PowerPC instructions to LLVM IR (basic integer/control-flow supported; floating-point/VMX in progress).
- Generate, optimize, and link x86-64 executables using LLVM and MSVC toolchain.
- Emulate Xbox 360 OS calls and memory management via a custom runtime (many functions stubbed, some implemented).
- Early GUI tooling for project management and debugging (ImGui-based, prototype stage).

## Current Limitations

- Not all PowerPC instructions are supported (VMX, floating-point, and some special instructions are incomplete).
- Many Xbox 360 kernel and system calls are stubbed or partially implemented.
- Multi-threading and synchronization primitives are recognized but not fully functional.
- No GPU or audio emulation yet—graphics/audio calls are not handled.
- GUI is in early development; not yet user-facing.

## Getting Started

1. Clone the repository and its submodules.
2. Install dependencies ([CMake][CMake], [win-llvm][win-llvm], and Visual Studio with the proper tools installed).
3. Place win-llvm files in a folder (e.g., `C:/LLVM19`).
4. Run `cmake -B out -DLLVM_USE_CRT_RELEASE=MT -DCMAKE_PREFIX_PATH=<LLVM_BINS_>` (replace `<LLVM_BINS_>` with your LLVM libs path).
5. Open `LLVM360.sln` in the `out/` folder and build the solution.

## Contributing

Contributions are welcome! See `TODO.md` for guidelines and join the [Discord][dis] for discussion. Please open pull requests for code contributions.

## License

See individual component folders for license details. The main project is licensed under the terms found in each submodule and custom codebase.

[win-llvm]: https://github.com/c3lang/win-llvm
[CMake]: https://cmake.org/download/
[dis]: https://discord.gg/JufwFS9mmf