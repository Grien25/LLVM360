# Xenos GPU Implementation Analysis

## Overview

The Xenos GPU implementation within the Xenon emulator is designed to replicate the functionality of the custom ATI graphics chip used in the Xbox 360. This analysis covers the key components, data flow, command processing, and state management involved in emulating the Xenos graphics pipeline.

## Files Analyzed

- `Xenon/Core/XGPU/XGPU.h`
- `Xenon/Core/XGPU/XGPU.cpp`
- `Xenon/Core/XGPU/XenosState.h`
- `Xenon/Core/XGPU/XenosState.cpp`
- `Xenon/Core/XGPU/XenosRegisters.h`
- `Xenon/Core/XGPU/CommandProcessor.h`
- `Xenon/Core/XGPU/CommandProcessor.cpp`
- `Xenon/Core/XGPU/RingBuffer.h`
- `Xenon/Core/XGPU/RingBuffer.cpp`
- `Xenon/Core/XGPU/EDRAM.h`
- `Xenon/Core/XGPU/EDRAM.cpp`
- `Xenon/Core/XGPU/PM4Opcodes.h`
- `Xenon/Core/XGPU/Microcode/*`
- `Xenon/Core/XGPU/ShaderConstants.h`

## Architecture and Components

The Xenos GPU emulation is structured around several key classes, each with distinct responsibilities:

1.  **`Xe::Xenos::XGPU`**: This is the main class representing the Xenos GPU device. It inherits from a PCI device base class, integrating it into the emulator's PCI bus system (`PCIBridge`). Key responsibilities include:
    *   Managing the GPU's PCI configuration space (`xgpuConfigSpace`).
    *   Holding pointers to core components: `XenosState`, `CommandProcessor`, `EDRAM`.
    *   Handling PCI BAR (Base Address Register) reads and writes, translating them to register accesses within the `XenosState`.
    *   Managing GPU clock settings via register writes based on the configured console revision.
    *   Running a separate "VSync Worker Thread" (`xeVSyncWorkerThreadLoop`) to simulate vertical blanking interrupts, which are crucial for timing in graphics applications.
    *   Implementing basic device read/write methods (`Read`, `Write`) that route memory-mapped I/O access to the appropriate GPU registers via `XenosState`.

2.  **`Xe::XGPU::XenosState`**: This class encapsulates the entire internal state of the Xenos GPU. It is the central hub for register management and state tracking:
    *   **Registers:** It maintains a large array (`Regs`) to store the values of all GPU registers (up to `0xFFFFF` bytes, covering a vast address space). It provides `ReadRawRegister` and `WriteRawRegister` methods for accessing these registers by their address/offset.
    *   **Register Access Logic:** The `ReadRawRegister` and `WriteRawRegister` methods contain extensive `switch` statements. These handle special read/write behaviors for specific registers. For example, reading the `RBBM_STATUS` register might update its internal flags to reflect the GPU's readiness state, or writing to an EDRAM register index (`RB_SIDEBAND_RD_ADDR`, `RB_SIDEBAND_WR_ADDR`) forwards the value to the `EDRAM` object.
    *   **State Tracking:** It holds numerous member variables that mirror specific register values or derived state (e.g., `fbSurfaceAddress` for the current framebuffer, `internalWidth`/`internalHeight` for display dimensions, various control register values like `depthControl`, `blendControl0`, etc.). This allows other components to quickly access important state without parsing the raw register array.
    *   **Integration:** It holds pointers to `RAM`, `EDRAM`, and `CommandProcessor`, facilitating interaction between GPU state and memory/storage subsystems.

3.  **`Xe::XGPU::CommandProcessor`**: This class is responsible for processing commands sent from the emulated CPU to the GPU. The CPU doesn't directly manipulate GPU state registers; instead, it writes command packets into a shared memory buffer (the Ring Buffer) which the GPU's Command Processor fetches and executes.
    *   **Ring Buffer Management:** It manages the GPU's Ring Buffer, a circular queue of commands in main memory. It tracks the base address (`cpRingBufferBasePtr`), size (`cpRingBufferSize`), and read/write pointers (`cpReadPtrIndex`, `cpWritePtrIndex`).
    *   **Worker Thread:** It runs its own worker thread (`cpWorkerThreadLoop`). This thread continuously checks the Ring Buffer for new commands (by comparing read and write pointers) and processes them.
    *   **Packet Execution:** The core logic involves reading packets from the Ring Buffer. Packets have types (Type-0, Type-1, Type-2, Type-3) and opcodes (defined in `PM4Opcodes.h` for Type-3). The `ExecutePacket` method dispatches to specific handlers (`ExecutePacketType0`, `ExecutePacketType3_DRAW_INDX`, etc.) based on the packet header.
    *   **Microcode Handling:** The Command Processor also handles the loading and execution of microcode for the internal microengines (ME - Micro Engine, PFP - Pre-Fetch Parser). The CPU writes microcode data via specific registers, which the `CommandProcessor` stores (`CPWriteMicrocodeData`). This microcode defines how certain complex operations or packet types are handled internally by the GPU hardware.
    *   **Rendering Interface:** Crucially, the `CommandProcessor` interacts with the rendering abstraction layer (`Render::Renderer`). When it encounters drawing commands (like `DRAW_INDX`), it gathers necessary state (shaders, textures, vertex/index buffers, render state from `XenosState`), prepares the data, and calls methods on the `Renderer` to perform the actual graphics API calls (e.g., OpenGL, Vulkan).

4.  **`Xe::XGPU::EDRAM`**: This class emulates the Embedded DRAM on the Xenos chip, used primarily for depth/stencil buffers and color buffers in certain tiling modes.
    *   It provides methods to read/write its internal registers (`ReadReg`, `WriteReg`) and data.
    *   It handles operations related to clearing, resolving, and copying data within its memory space.
    *   The `CommandProcessor` or `XenosState` can interact with it for EDRAM-related operations.

## Data Flow and Operation

1.  **Initialization:**
    *   During emulator startup (`XeMain::Create`), an `XGPU` instance is created.
    *   The `XGPU` constructor initializes its state, including setting up the PCI config space with the correct device/vendor IDs based on the emulated console revision.
    *   It initializes the `XenosState` and `CommandProcessor`.
    *   Clock-related registers are set.
    *   The VSync worker thread is started.

2.  **CPU-GPU Communication (Register Access):**
    *   The CPU interacts with the GPU by reading/writing to memory addresses that correspond to the GPU's BARs (configured via PCI).
    *   In `XGPU::Read`/`Write`, the address is checked against the mapped BAR ranges.
    *   If valid, the address is translated to a register index, and the call is forwarded to `XenosState::ReadRegister`/`WriteRegister`.
    *   `XenosState` performs the actual read/write on its internal `Regs` array, applying any special logic in `ReadRawRegister`/`WriteRawRegister`.

3.  **Command Submission (Rendering):**
    *   The CPU driver (running inside the emulated environment) prepares rendering commands.
    *   It writes these commands (packets) into a designated area of main system RAM (the Ring Buffer).
    *   The CPU updates the Ring Buffer's write pointer (a GPU register) to signal new commands are available.
    *   The `CommandProcessor`'s worker thread detects the updated write pointer.
    *   It calls `cpExecutePrimaryBuffer` which reads packets from the Ring Buffer.
    *   `ExecutePacket` determines the packet type and opcode.
    *   For example, a `PM4_DRAW_INDX` packet is handled by `ExecutePacketType3_DRAW_INDX`.
    *   This handler reads state from `XenosState` (shaders, render targets, viewport, etc.), prepares index/vertex data, and invokes the graphics backend (`render->DrawIndexed`) to render the primitives.

4.  **Rendering Backend:**
    *   The `Render::Renderer` (e.g., `OGLRenderer`) receives high-level draw calls from the `CommandProcessor`.
    *   It translates these into specific graphics API calls (OpenGL, etc.).
    *   It uses the emulated RAM pointer (`ramPtr`) to access vertex/index buffer data.
    *   It uses state information passed from the `CommandProcessor` (derived from `XenosState`) to configure the graphics pipeline (shaders, blend modes, depth testing, etc.).

5.  **VSync Simulation:**
    *   The `xeVSyncWorkerThreadLoop` in `XGPU` runs periodically (simulated at a target refresh rate, e.g., 60Hz).
    *   When a VSync occurs, it updates internal VBlank status registers (`vblankVlineStatus`) and sets flags in the interrupt mask (`d1modeIntMask`).
    *   It signals an interrupt to the CPU via the `PCIBridge` (`RouteInterrupt`), informing the emulated system that a frame has finished rendering.

## Key Emulation Details

*   **Register State:** The `XenosState` class is pivotal, maintaining a comprehensive snapshot of the GPU's internal state through its register array and specific state variables.
*   **Command Processing:** The `CommandProcessor` decouples CPU command submission (via Ring Buffer) from GPU command execution, mirroring the asynchronous nature of real hardware.
*   **Microcode:** Handling ME/PFP microcode allows for emulation of complex GPU-internal behaviors defined by the hardware's microcode.
*   **Integration with System:** As a PCI device, the `XGPU` integrates seamlessly with the emulator's memory and interrupt systems via the `RootBus` and `PCIBridge`.
*   **Graphics API Abstraction:** The use of `Render::Renderer` allows the core emulation logic to be graphics API agnostic, supporting different backends like OpenGL or potentially Vulkan.

## Conclusion

The Xenos GPU emulation is a complex system involving state management (`XenosState`), command processing (`CommandProcessor`), memory/storage (`EDRAM`), and integration with the host system's graphics API (`Renderer`). The `XGPU` class serves as the central device, managing its PCI identity and coordinating the other components. The use of a Ring Buffer for command submission and a dedicated worker thread for processing closely mimics the asynchronous operation of the real hardware, providing a robust foundation for rendering emulated Xbox 360 graphics content.