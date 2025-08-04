# TODO List for LLVM360

This list tracks major tasks, improvements, and ideas for the LLVM360 project. Contributions and suggestions are welcome!

## Core Emulator & Analysis
- [ ] Refactor thread context handling: remove reliance on TLS variables, pass context as explicit parameters.
- [ ] Improve support for additional Xbox 360 binary formats (e.g., XEX, ELF variants).
- [ ] Expand instruction set coverage and accuracy in the recompiler.
- [ ] Integrate more robust error handling and logging.

## GUI & Tooling
- [ ] Enhance memory viewer and symbol browser features.
- [ ] Add search/filter capabilities to project and memory views.
- [ ] Improve project management UI (recent files, favorites, etc.).
- [ ] Add support for scripting or automation (e.g., Python integration).

## Build System & Dependencies
- [ ] Streamline CMake configuration for easier setup on Windows and Linux.
- [ ] Update and document external dependencies (Dear ImGui, Xenia, etc.).
- [ ] Add CI/CD pipelines for automated builds and tests.

## Documentation
- [ ] Expand README with usage examples and screenshots.
- [ ] Write developer guides for adding new features or backends.
- [ ] Document code structure and module responsibilities.

## Stretch Goals
- [ ] Integrate with Xenia for live debugging and state inspection.
- [ ] Add support for additional platforms or architectures.
- [ ] Explore cloud-based analysis or remote debugging features.
