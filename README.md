# LLVM360

LLVM360 is an experimental static recompiler and runtime environment aiming to run Xbox 360 executables natively on PC. It leverages the LLVM compiler toolchain to translate PowerPC assembly into native x86-64 code.

The project's core philosophy is to achieve high compatibility and accuracy through a **Low-Level Emulation (LLE)** approach for the console's kernel and operating system. Instead of reimplementing kernel functions from scratch (HLE), the goal is to recompile the original `xboxkrnl.exe` and other system modules to run them within a thin hardware abstraction layer.

This project is in its early stages and is a personal experiment inspired by projects like `xeo3` and "[rexdex's recompiler](https://github.com/rexdex/recompiler/tree/master)".

## Core Components

The project is architecturally divided into two main parts:

1.  **`Naive+` (The Recompiler)**: An offline command-line tool that analyzes an Xbox 360 `.xex` file and translates its PowerPC code into LLVM Intermediate Representation (IR).
2.  **`Emulator` (The Runtime)**: A Windows DLL that provides the necessary environment for the recompiled code to execute. It manages the virtual memory space, CPU state, and provides implementations for kernel and system library calls.

## Project Workflow

- **Run `Naive+` on a `.xex`:** Use the command-line recompiler on a game's or homebrew's Xbox 360 executable.
- **Generate outputs (`.ll` + `.tss`):** The recompiler emits an LLVM IR file (`.ll`) with translated code and a runtime metadata file (`.tss`) describing module layout, imports/exports, and other runtime needs.
- **Compile and link:** The `.ll` file is compiled to a native object file and linked together with the runtime (`Emulator.dll`) and any required support libraries to produce a final, runnable Windows executable.
- **Run with the runtime:** The produced binary loads `Emulator.dll`, which provides the kernel and environment services the recompiled code expects.

## Why LLVM?

LLVM provides a mature, platform-independent IR and industry-grade optimization passes with high-quality backends (e.g., x86-64), enabling aggressive, portable optimization of recompiled code while leveraging a rich ecosystem of analysis and tooling.

## Current Status

The project is currently in a foundational, experimental phase.

* **Recompiler (`Naive+`):**
    * Successfully loads and parses `.xex` executable files, reusing a loader from a previous project.
    * Implements a multi-pass pipeline: Decode -> Control Flow Analysis -> IR Emission.
    * Decodes a wide range of PowerPC instructions into a structured format.
    * The control flow pass can identify function boundaries by analyzing prologues (`mfspr`), epilogues (`bclr`), and branch instructions (`bl`).
    * It can emit LLVM IR (`.ll` files) for decoded functions and export a custom metadata file (`MD.tss`) for the runtime.

* **Runtime (`Emulator`):**
    * Provides a foundational memory manager that reserves a 32-bit address space for the game.
    * Defines the `XenonState` structure to hold the state of all CPU registers.
    * Contains entry points and stubs for many `XboxKrnl` and `Xam` API calls, though most are not yet implemented.
    * Graphics: Auto-selects backend per platform — DirectX 12 on Windows; Plume (Metal) planned on macOS. Current macOS backend is scaffolded for future Plume integration.

## Building

-   Windows: Install the LLVM 19 libraries.
-   macOS: `brew install llvm` (CMake will auto-detect via `llvm-config`).
-   Set the `LLVM_DIR_INST` variable in the root `CMakeLists.txt` to your LLVM installation path.
-   Use CMake to generate a Visual Studio solution in the `/out` directory.
-   Build the `LLVM360.sln` solution.

## Contributing

Contributions are welcome! Please feel free to fork the repository and submit a Pull Request. You can also join the project's [Discord server](https://discord.gg/JufwFS9mmf) to discuss development.
