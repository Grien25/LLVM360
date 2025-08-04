# LLVM360 Project

LLVM360 is a modular toolkit and emulator framework for Xbox 360 binaries, focused on reverse engineering, static analysis, and dynamic emulation. The project integrates custom tools, recompilers, and open-source components to provide a comprehensive environment for exploring and running Xbox 360 executables.

## Project Structure

- **llvm360/Emulator/**: Core emulator implementation for Xbox 360 binaries, using LLVM for code analysis and dynamic recompilation.
- **llvm360/Naive+/**: Experimental or alternative analysis/recompilation engine for Xbox 360 binaries.
- **external/imgui/**: [Dear ImGui](https://github.com/ocornut/imgui) library for building graphical user interfaces in the emulator and tools.
- **external/recompiler/**: Standalone recompiler and analysis tool with a GUI for project management, memory viewing, and symbol navigation.
- **external/xenia/**: [Xenia](https://github.com/xenia-project/xenia) Xbox 360 emulator, included as a submodule for reference and backend support.
- **CMakeLists.txt / CMakeSettings.json**: CMake-based build system for configuring and building all components.
- **compile.bat**: Windows batch script for building the project.

## Features

- Load, analyze, and emulate Xbox 360 binaries (XEX, ELF, etc.).
- Project-based workflow: create, save, and manage analysis projects.
- Memory viewer, symbol browser, and disassembly tools.
- GUI tools built with Dear ImGui for interactive exploration.
- Integration with Xenia for advanced emulation and debugging.

## Getting Started

1. Clone the repository and its submodules.
2. Install dependencies ([CMake Binary][CMake], [win-llvm][win-llvm], and Visual Studio with the proper tools installed).
3. Move the win-llvm files into a folder, for this project we put them in `C:/LLVM19` but developers may change it.
4. Run `cmake -B out -DLLVM_USE_CRT_RELEASE=MT -DCMAKE_PREFIX_PATH=<LLVM_BINS_>` (LLVM_BINS is the root folder where you stored your LLVM libs mine is "C:/LLVM19" for example)
5. In the "out/" folder open the `LLVM360.sln`. If everything is good and i or you didn't messed up something, it should compile fine
## Contributing

Contributions are welcome! Please see the `TODO.md` for guidelines, and open pull requests for discussion. Join the [Discord][dis] for better communication!

## License

See individual component folders for license details. The main project is licensed under the terms found in each submodule and custom codebase.

[win-llvm]: https://github.com/c3lang/win-llvm
[CMake]: https://cmake.org/download/
[dis]: https://discord.gg/JufwFS9mmf