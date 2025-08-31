# Xenon Emulator Project Context

## Project Overview

Xenon is an **Xbox 360 emulator** written in **C++**, targeting **Windows** and **Linux** platforms. It is currently in an early experimental stage, primarily capable of running small programs like XeLL (Xenon Linux Loader), Linux (ppc64-xenon), and LK (Little Kernel).

The project utilizes CMake for its build system and manages dependencies through a combination of system packages (if `XENON_USE_SYSTEM_DEPS` is enabled) and bundled subdirectories (if `XENON_ALLOW_BUNDLED_DEPS` is enabled). Key bundled dependencies include `fmt`, `asmjit`, `toml11`, `glad`, `SDL3`, `sirit`, `ImGui`, and `microprofile`. It uses C++23 and C17 standards.

The core components include emulation of the Xenon CPU (three PPU cores with SMT and VMX), Xenos GPU, RAM, NAND, and various PCI devices like SMC, Ethernet, Audio Controller, USB controllers (EHCI/OHCI), SFCX, XMA, ODD, and HDD. Graphics rendering is handled by backend renderers (e.g., OpenGL).

Configuration is managed via a `TOML` file. The main entry point is `Xenon/Main.cpp`, which initializes the emulator core (`XeMain`), sets up signal handlers for clean shutdown, and starts the CPU execution loop.

## Building and Running

### Prerequisites
- CMake 3.22 or higher
- A C++23 compatible compiler (MSVC, GCC, Clang)
- A C17 compatible C compiler
- Dependencies (either system-installed or bundled):
  - fmt
  - asmjit
  - toml11
  - glad (if graphics enabled)
  - SDL3 (if graphics enabled)
  - sirit (if graphics enabled)

### Build Steps

1.  **Configure:** Create a build directory and run CMake.
    ```bash
    mkdir build && cd build
    cmake ..
    ```
    You can customize the build with options like:
    - `-DCMAKE_BUILD_TYPE=Release` (default) or `Debug`.
    - `-DGFX_ENABLED=ON` (default) or `OFF` to enable/disable graphics.
    - `-DXENON_USE_SYSTEM_DEPS=ON` (default) or `OFF` to prefer system packages.
    - `-DXENON_ALLOW_BUNDLED_DEPS=ON` (default) or `OFF` to allow/disallow bundled dependencies.

2.  **Build:** Use your chosen build tool (e.g., `make`, `ninja`, or Visual Studio).
    ```bash
    cmake --build .
    ```

3.  **Run:** Execute the generated `Xenon` binary from the build directory.
    ```bash
    ./Xenon
    ```

### Platform-Specific Instructions
Detailed build guides are available for [Windows](https://github.com/xenon-emu/Xenon/blob/main/Deps/Docs/Building/building-windows.md), [Linux](https://github.com/xenon-emu/Xenon/blob/main/Deps/Docs/Building/building-linux.md), and [macOS](https://github.com/xenon-emu/Xenon/blob/main/Deps/Docs/Building/building-macos.md).

## Development Conventions

- **Language:** C++23 for core logic, C17 for some base components.
- **Build System:** CMake.
- **Dependencies:** Managed via CMake's `find_package` and `add_subdirectory` for bundled deps. The `xenon_dep` macro in `CMakeLists.txt` facilitates this.
- **Coding Style:** Based on the code snippets, it uses `#pragma once` for headers, `inline` for namespace variables, and a namespace structure like `XeMain::`. Error handling involves logging and global state flags.
- **Configuration:** Uses `TOML` for configuration, managed within the `Config` namespace.
- **Graphics:** Conditional compilation using `#ifdef GFX_ENABLED` / `#ifndef NO_GFX`.
- **Logging:** Uses a custom logging backend (`Base::Log`).
- **Documentation:** Inline comments and separate markdown files in `Deps/Docs`.
- **Versioning:** Version information is embedded via CMake (`EMULATOR_VERSION_*`) and Git commit count/branch.