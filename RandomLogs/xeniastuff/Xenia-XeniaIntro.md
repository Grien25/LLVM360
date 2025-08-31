# Xenia Canary - Xbox 360 Emulator

## Project Overview

Xenia Canary is an experimental fork of the Xenia emulator, designed to emulate Xbox 360 games. The project is written primarily in C++ and utilizes tools like Premake and CMake for its build system. It supports Windows and Linux platforms, with experimental Android support.

Key technologies and components include:
- **C++20**: The primary language for the emulator core.
- **Premake/CMake**: Build system generators.
- **Visual Studio / Clang / GCC**: Supported compilers.
- **Vulkan**: Primary graphics API for rendering.
- **ImGui**: Used for the debugging UI.

## Building and Running

### Prerequisites

- **Windows**:
  - Windows 10 or later
  - Visual Studio 2022
  - CMake 3.10+
  - Windows 11 SDK (version 10.0.22000.0 or newer)
  - Python 3.9+ 64-bit
- **Linux**:
  - Clang 19+ (preferred) or GCC
  - Required development libraries (e.g., `build-essential`, `mesa-vulkan-drivers`, `libc++-dev`, `libgtk-3-dev`, `libsdl2-dev`, `libvulkan-dev`, `ninja-build`, etc.)
  - Vulkan drivers

### Setup and Build Commands

1.  **Clone and Setup**:
    ```bash
    git clone https://github.com/xenia-canary/xenia-canary.git
    cd xenia-canary
    ./xb setup
    ```
2.  **Build**:
    ```bash
    # Command line build (default config is debug)
    ./xb build [--config=release|debug|checked]

    # Open in IDE (e.g., Visual Studio, Xcode, CLion)
    ./xb devenv
    ```
3.  **Run**:
    - After building, the executable (`xenia-app`) will be located in `build/bin/[Platform]/[Config]/`.
    - You can run it from the command line: `./build/bin/[Platform]/[Config]/xenia-app --log_file=stdout /path/to/game.iso` or `.xex`.

## Development Conventions

- **Language**: C++20 is used. Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) as closely as possible.
- **Formatting**: Code is formatted using `clang-format` with the Google style. Run `./xb format` before committing.
- **Linting**: The project uses `cpplint` for style checks. Run `./xb lint` to check for issues.
- **Git History**: Maintain a clean git history. Individual commits should be functional and compile successfully. Use `git rebase` to clean up history before submitting pull requests.
- **Code Structure**: The codebase is organized under the `src/` directory. Key areas include CPU emulation, GPU emulation, kernel, HID, and the UI.
- **Contributing**: Follow the guidelines in `.github/CONTRIBUTING.md`. Ensure all information is derived from reverse engineering legally obtained materials. No code from official Xbox Development Kits (XDKs) is allowed.

## Detailed Technical Analysis

For a comprehensive technical analysis of how Xenia Canary emulates specific Xbox 360 components, please refer to the following detailed documents:

1. **[Xenon CPU Emulation Analysis](XENON_CPU_ANALYSIS.md)** - Detailed examination of how Xenia emulates the custom 3-core PowerPC processor, including dynamic recompilation, memory management, and threading systems.

2. **[Xenos GPU Emulation Analysis](XENOS_GPU_ANALYSIS.md)** - In-depth look at the emulation of the custom ATI graphics processor, covering command processing, shader translation, render state management, and EDRAM emulation.

3. **[Xbox 360 Kernel Emulation Analysis](KERNEL_ANALYSIS.md)** - Comprehensive analysis of the reimplementation of the Xbox 360 kernel, including object management, threading, memory management, and I/O subsystems.

These documents provide code examples and technical details about the implementation of each component based on the actual Xenia Canary source code.