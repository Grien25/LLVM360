# Xenon CPU (Xenon) Implementation Analysis

## Overview

The Xenon CPU implementation in the emulator aims to accurately replicate the behavior of the tri-core PowerPC-based processor found in the Xbox 360. This analysis delves into the architecture, key components, execution modes, and memory interaction of the emulated CPU.

## Files Analyzed

- `Xenon/Core/XCPU/Xenon.h`
- `Xenon/Core/XCPU/Xenon.cpp`
- `Xenon/Core/XCPU/PPU/PPU.h`
- `Xenon/Core/XCPU/PPU/PPU.cpp`
- `Xenon/Core/XCPU/PPU/PowerPC.h`
- `Xenon/Core/XCPU/Interpreter/PPCInterpreter.h`
- `Xenon/Core/XCPU/Interpreter/PPCInterpreter.cpp`
- `Xenon/Core/XCPU/Interpreter/PPCInternal.h`
- `Xenon/Core/XCPU/IIC/IIC.h`
- `Xenon/Core/XCPU/IIC/IIC.cpp`
- `Xenon/Core/XCPU/eFuse.h`
- `Xenon/Core/XCPU/XenonSOC.h`
- `Xenon/Core/XCPU/XenonSOC.cpp`

## Architecture and Components

The `Xenon` class (`Xenon.h`, `Xenon.cpp`) serves as the central manager for the CPU emulation. Its primary responsibilities include:

1.  **Initialization:** It initializes the core components upon construction. This involves:
    *   Setting up the `RootBus` pointer for memory/IO access.
    *   Initializing internal SRAM (`xenonContext.SRAM`) and SROM (`xenonContext.SROM`). The SROM is populated by loading the `1bl.bin` file, which contains the initial bootloader code executed by the hardware.
    *   Loading and parsing the eFuse data from `fuses.txt` into the `xenonContext.socSecOTPBlock`. These fuses define various hardware characteristics and security keys.
    *   Configuring the `PPCInterpreter`'s global context pointers (`CPUContext`, `sysBus`, `ramPtr`).
    *   Setting initial values for specific System-on-Chip (SoC) registers, like the Power Management Control register.

2.  **PPU Core Management:** The Xenon CPU consists of three PowerPC Processing Units (PPUs). The `Xenon` class manages these:
    *   It holds `std::unique_ptr`s to `PPU` objects (`ppu0`, `ppu1`, `ppu2`), one for each core.
    *   The `Start` method creates these PPU instances. Each PPU is initialized with a pointer to the shared `XENON_CONTEXT` (`xenonContext`), the `RootBus`, a reset vector (typically `0x20000000100` for normal boot or `0x100` for 1BL simulation), and a base PIR (Processor Identification Register) value indicating the core ID (0, 2, 4). This PIR value is likely related to the SMT (Simultaneous Multithreading) implementation where each physical core handles two logical threads.
    *   After creation, it starts the execution loop for the first PPU (`ppu0->StartExecution()`), retrieves its calculated Clocks Per Instruction (CPI), propagates this CPI to the other PPUs, and then starts their execution loops as well. This ensures synchronized timing across cores.

3.  **Execution Control:**
    *   `LoadElf`: Allows loading and executing an ELF binary directly, bypassing the normal boot process. It creates the PPU instances and uses `ppu0->loadElfImage` to load the binary into the emulated RAM and set the entry point.
    *   `Reset`: Resets all three PPU cores by calling their respective `Reset` methods.
    *   `Halt`, `Continue`, `ContinueFromException`, `Step`: Provide debugging control over all PPU cores. `Halt` stops execution (potentially at a specific address or on guest request), `Continue` resumes from a halt, `ContinueFromException` is used after handling an exception, and `Step` executes a specified number of instructions.
    *   `IsHalted`, `IsHaltedByGuest`: Query the halted state of any of the PPU cores.
    *   `GetPPU`: Retrieves a pointer to a specific PPU core (0, 1, or 2) by its ID.

## PPU (PowerPC Processing Unit)

The `PPU` class (`PPU.h`, `PPU.cpp`) represents a single core of the Xenon CPU.

### Core Components

1.  **State Management:**
    *   `PPU_STATE`: Holds the complete architectural state of the core, including General Purpose Registers (GPR), Floating-Point Registers (FPR), Vector Registers (VR), Special Purpose Registers (SPR) like MSR, HID, XER, and various control registers. This state is defined in `PPU.h`.
    *   `XENON_CONTEXT`: A shared context (`xenonContext` pointer) containing memory-mapped SoC registers (like IIC, PCI bridge registers), SRAM, SROM, and eFuse data. This context is shared among all PPU cores.
    *   `eThreadState`: An atomic state machine (`ppuThreadState`) manages the core's lifecycle (None, Sleeping, Halted, Running, Executing, Resetting, Quiting).

2.  **Execution Engine:**
    *   **Thread:** Each PPU runs its execution loop (`ThreadLoop`) in a dedicated `std::thread` (`ppuThread`). This loop continuously fetches, decodes, and executes instructions based on the current `eThreadState`.
    *   **Execution Modes:** The PPU supports different execution strategies controlled by `eExecutorMode` (Interpreter, JIT, Hybrid). The current implementation primarily uses the interpreter.
    *   **Interpreter:** The core execution logic for the interpreter resides in `PPCInterpreter` (namespace in `PPCInterpreter.h/cpp`). It contains functions for decoding PowerPC opcodes (using structures defined in `PowerPC.h` and `PPC_Instruction.h`) and implementing the behavior of each instruction. Helper macros in `PPCInterpreter.h` simplify access to the current PPU state (`curThread`, `GPR`, `FPR`, etc.).
    *   **CPI (Clocks Per Instruction):** The `clocksPerInstruction` member controls the timing simulation. It determines how many "ticks" (representing CPU cycles) are consumed per instruction executed. This is used in conjunction with the decrementer and timebase registers to simulate timing.

3.  **Memory Management Unit (MMU) Interaction:**
    *   The PPU relies heavily on the `PPCInterpreter` namespace for memory access.
    *   Functions like `MMUTranslateAddress`, `MMURead`, `MMUWrite`, `MMURead8/16/32/64`, `MMUWrite8/16/32/64` handle virtual-to-physical address translation and actual data transfer.
    *   These MMU functions interact with the `RootBus` (`sysBus`) to route memory requests to the appropriate device (RAM, I/O registers, PCI devices). This is crucial for accessing not just main RAM but also the various hardware components mapped into the CPU's address space.

4.  **Exception Handling:**
    *   The PPU implements various exception handlers (System Reset, Instruction Storage, Data Storage, Decrementer, System Call, etc.) as methods (`PPUSystemResetException`, `PPUDataStorageException`, etc.).
    *   These handlers are called when specific conditions are met (e.g., accessing unmapped memory triggers a Data Storage Exception). They update the processor state (like SRR0, SRR1) and transfer control to the appropriate exception handler address.

5.  **Debugging and Control:**
    *   Methods like `Halt`, `Continue`, `Step` allow external control for debugging.
    *   `PPUCheckInterrupts` and `PPUCheckExceptions` are called periodically within the execution loop to determine if an interrupt or exception needs to be serviced.

## Key Emulation Details

*   **SMT (Simultaneous Multithreading):** While the `Xenon` class manages 3 PPU objects, the PPU constructor takes a `PIR` base and internally manages two "threads" (`PPU_THREAD_REGISTERS`). This reflects the hardware's ability to execute two logical threads per physical core.
*   **Boot Process:** The emulation can start via the `1bl.bin` loaded into SROM (simulating hardware boot) or directly load an ELF file. The `Simulate1Bl` function within the PPU handles the internal logic for executing the 1BL code if that path is taken.
*   **eFuses:** Loading eFuse data is critical for emulating hardware-specific behavior and security checks performed by the bootloader and potentially the OS.
*   **SoC Integration:** The `XENON_CONTEXT` tightly integrates the CPU cores with the rest of the emulated System-on-Chip, including the IIC (Inter-Integrated Circuit controller for CPU communication) and various memory-mapped I/O regions.

## Conclusion

The Xenon CPU implementation provides a structured approach to emulating the multi-core PowerPC architecture. The `Xenon` class orchestrates the cores, manages boot processes, and provides high-level control. The `PPU` class encapsulates the logic for a single core, including its state, execution loop, MMU interaction, and exception handling. The interpreter (`PPCInterpreter`) forms the core of instruction execution, translating opcodes into emulated behavior. The integration with `XENON_CONTEXT` and `RootBus` is essential for accessing the complete emulated hardware environment.