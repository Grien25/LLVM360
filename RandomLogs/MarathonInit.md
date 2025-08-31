# Marathon Recompiled - Qwen Code Context

## Project Overview

This repository contains the source code for "Marathon Recompiled", an unofficial PC port of the Xbox 360 version of Sonic the Hedgehog (2006). It's created through static recompilation, converting the original PowerPC code and Xenos shaders into compatible C++ and HLSL code. The port supports Windows, Linux, and macOS.

Key technologies and components:
- **C++20**: The primary programming language.
- **CMake**: Build system with presets for different platforms and configurations.
- **vcpkg**: Dependency management.
- **SDL2**: Likely used for cross-platform windowing/input.
- **Clang**: The compiler used for building.
- **Ninja**: The build generator.
- **XenonRecomp / XenosRecomp**: External tools (likely submodules) used for the core recompilation process.
- **Third-party libraries**: A substantial number of libraries are included in the `thirdparty` directory as submodules (e.g., ImGui, SDL, FFmpeg-core, MoltenVK, etc.).

Crucially, this project **does not include any game assets**. Users must provide legally acquired game files (specifically `default.xex`, `shader.arc`, `shader_lt.arc`) to build or install the port.

## Building and Running

### Prerequisites

1.  **Clone the Repository**: Use `git clone --recurse-submodules` to get all submodules.
2.  **Game Files**: Place `default.xex`, `shader.arc`, and `shader_lt.arc` from the original Xbox 360 game into `./MarathonRecompLib/private/`.
3.  **Dependencies**:
    *   **All Platforms**: Ensure `vcpkg` is available (path specified in `CMakePresets.json` via `VCPKG_ROOT`).
    *   **Windows**: Visual Studio 2022 with C++ Clang Compiler and CMake tools.
    *   **Linux**: `autoconf`, `automake`, `libtool`, `pkg-config`, `curl`, `cmake`, `ninja-build`, `clang`, `clang-tools`, `libgtk-3-dev` (or equivalents).
    *   **macOS**: Xcode or Xcode Command Line Tools, `cmake`, `ninja`, `pkg-config` (via Homebrew or MacPorts).

### Build Commands (CMake Presets)

The project uses CMake presets defined in `CMakePresets.json`. The main presets for building are:

*   **Windows**:
    *   `x64-Clang-Debug`
    *   `x64-Clang-RelWithDebInfo`
    *   `x64-Clang-Release`
*   **Linux**:
    *   `linux-debug`
    *   `linux-relwithdebinfo`
    *   `linux-release`
*   **macOS**:
    *   `macos-debug`
    *   `macos-relwithdebinfo`
    *   `macos-release`

**Example (Linux Release)**:
1.  Configure: `cmake . --preset linux-release`
2.  Build: `cmake --build ./out/build/linux-release --target MarathonRecomp`

**Example (macOS Release)**:
1.  Configure: `cmake . --preset macos-release`
2.  Build: `cmake --build ./out/build/macos-release --target MarathonRecomp`

**Example (Windows)**:
Building is typically done through Visual Studio after opening the folder and letting CMake generation complete.

The output executable will be in the corresponding `out/build/<preset-name>` directory.

## Development Conventions

*   **Language**: C++20.
*   **Build System**: CMake with presets.
*   **Dependencies**: Managed via `vcpkg` and submodules in the `thirdparty` directory.
*   **Structure**: The codebase is split into `MarathonRecompLib` (likely core emulation/recompilation logic) and `MarathonRecomp` (main application, UI, OS integration). Various subsystems (CPU, GPU, HID, etc.) appear to be modularized within the `MarathonRecomp` directory.
*   **Compiler Flags**: Uses Clang. Targets Sandy Bridge architecture for x86_64. Static linking is implied (`CMAKE_MSVC_RUNTIME_LIBRARY` setting, vcpkg triplet `x64-windows-static`).
*   **Configuration**: Uses `config.toml` for runtime settings (key bindings, etc.).
*   **Save Data**: Stored in platform-specific user config directories, unless `portable.txt` is present.
