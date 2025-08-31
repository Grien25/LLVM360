# Xenon CPU Emulation in Xenia Canary

## Overview

The Xenon CPU is a custom 3-core PowerPC processor running at 3.2GHz, with each core having its own L1 cache and sharing a 1MB L2 cache. Xenia Canary implements sophisticated emulation of this processor through dynamic recompilation and careful modeling of the PowerPC architecture.

## Architecture Implementation

### PowerPC Frontend (`src/xenia/cpu/ppc`)

Xenia's PowerPC frontend implements a complete decoder for the PowerPC 64-bit instruction set, handling all 32 general-purpose registers (GPRs), 32 floating-point registers (FPRs), and 128 vector registers (VRs). The implementation can be found in `ppc_context.h`:

```cpp
typedef struct alignas(64) PPCContext_s {
  union {
    uint32_t value;
    struct {
      uint8_t cr0_lt;  // Negative (LT) - result is negative
      uint8_t cr0_gt;  // Positive (GT) - result is positive (and not zero)
      uint8_t cr0_eq;  // Zero (EQ) - result is zero or a stwcx/stdcx completed
      uint8_t cr0_so;  // Summary Overflow (SO) - copy of XER[SO]
    };
  } cr0;
  // ... other condition registers
  
  uint64_t r[32];  // General purpose registers
  uint64_t ctr;    // Count register
  uint64_t lr;     // Link register
  
  double f[32];     // Floating-point registers
  vec128_t v[128];  // VMX128 vector registers
} PPCContext;
```

### Dynamic Recompilation (JIT)

The heart of Xenia's CPU emulation is its dynamic recompiler, which translates PowerPC instructions to optimized x64 code at runtime. The process involves:

1. **Instruction Decoding**: PowerPC instructions are decoded in `ppc_frontend.cc` using a lookup table approach:

```cpp
// From ppc_frontend.cc
bool PPCFrontend::DecodeInstr(uint32_t address, InstrData* out_instr_data) {
  uint32_t code = xe::load_and_swap<uint32_t>(
      memory_->TranslateVirtual(address));
  out_instr_data->address = address;
  out_instr_data->code = code;
  out_instr_data->opcode = GetOpcodeInfo(code);
  // ...
}
```

2. **HIR Generation**: Instructions are converted to Xenia's High-level Intermediate Representation in `ppc_hir_builder.cc`:

```cpp
// Example from ppc_hir_builder.cc - handling an add instruction
int InstrEmit_add(PPCHIRBuilder& f, const InstrData& i) {
  // add RT,RA,RB
  Value* ra = f.LoadGPR(i.XO.RA);
  Value* rb = f.LoadGPR(i.XO.RB);
  Value* rt = f.Add(ra, rb);
  f.StoreGPR(i.XO.RT, rt);
  f.UpdateCR(rt, false);  // Update condition register
  return 0;
}
```

3. **x64 Code Generation**: The HIR is then compiled to x64 machine code in `x64_backend.cc`. Xenia uses a template-based approach for code generation:

```cpp
// Example from x64_sequences.cc - sequence for ADD instruction
struct ADD_I64_I64_I64 : Sequence<ADD_I64_I64_I64, I<OPCODE_ADD, V_I64, V_I64, V_I64>> {
  static void Emit(X64Emitter& e, const EmitArgType& i) {
    e.mov(e.rax, i.src1);
    e.add(e.rax, i.src2);
    e.mov(i.dest, e.rax);
    // Handle overflow flags, etc.
  }
};
```

### Memory Management

Xenia implements sophisticated memory management to handle the Xbox 360's virtual memory system:

```cpp
// From memory.h - Memory class handles virtual/physical memory mapping
class Memory {
 public:
  // Translates a guest virtual address to a host address
  template <typename T = uint8_t*>
  inline T TranslateVirtual(uint32_t guest_address) const {
    return reinterpret_cast<T>(virtual_membase_ + guest_address);
  }
  
  // Handles physical memory access
  template <typename T = uint8_t*>
  inline T TranslatePhysical(uint32_t guest_address) const {
    return reinterpret_cast<T>(physical_membase_ + (guest_address & 0x1FFFFFFF));
  }
};
```

### Threading and Synchronization

The Xenon processor supports hardware threads (fibers) which Xenia emulates through its threading system:

```cpp
// From xboxkrnl_threading.cc - thread creation
uint32_t ExCreateThread(xe::be<uint32_t>* handle_ptr, uint32_t stack_size,
                        xe::be<uint32_t>* thread_id_ptr,
                        uint32_t xapi_thread_startup, uint32_t start_address,
                        uint32_t start_context, uint32_t creation_flags) {
  // Create a new thread with specified parameters
  // ...
  thread->Create();
  // ...
}
```

## Performance Optimizations

### Block-Based Compilation

Xenia compiles code in blocks rather than individual instructions, allowing for better optimization opportunities:

```cpp
// From processor.cc - function compilation
bool Processor::DemandFunction(Function* function) {
  // Only compile if not already compiled
  if (function->is_compiled()) {
    return true;
  }
  
  // Compile the entire function
  return backend_->Compile(function);
}
```

### Profiling-Guided Optimization

Xenia can profile code execution to optimize frequently-used paths:

```cpp
// From processor.cc
void Processor::OnFunctionDefined(Function* function) {
  // Register function for potential optimization
  if (FLAGS_enable_profiling) {
    // Set up profiling counters
  }
}
```

### Register Caching

Xenia maintains a mapping between guest registers and host registers to minimize memory accesses:

```cpp
// From x64_emitter.h - register caching
class X64Emitter {
  // Cache of guest GPRs in host registers
  Value* guest_reg_cache_[32];
  // ...
};
```

## Challenges and Solutions

### Endianness Handling

The Xbox 360 uses big-endian format while x86 uses little-endian. Xenia handles this with byte-swapping utilities:

```cpp
// From base/memory.h
template <typename T>
inline T load_and_swap(const void* memory) {
  return byte_swap(*reinterpret_cast<const T*>(memory));
}
```

### Atomic Operations

Xenia carefully implements atomic operations to match the PowerPC architecture:

```cpp
// From processor.cc - atomic operations
uint32_t Processor::GuestAtomicIncrement32(ppc::PPCContext* context,
                                          uint32_t guest_address) {
  uint32_t* host_address = memory()->TranslateVirtual<uint32_t*>(guest_address);
  return xe::atomic_inc(host_address);
}
```

### Exception Handling

Xenia implements precise exception handling to match PowerPC behavior:

```cpp
// From processor.cc - exception handling
bool Processor::ExceptionCallback(Exception* ex) {
  // Handle various types of exceptions
  switch (ex->type()) {
    case Exception::kExceptionTypeAccessViolation:
      // Handle memory access violations
      return HandleAccessViolation(ex);
    // ...
  }
}
```

## Conclusion

Xenia's Xenon CPU emulation is a sophisticated implementation that combines dynamic recompilation with careful architectural modeling. Through its multi-stage compilation process (PPC → HIR → x64), intelligent caching, and precise emulation of PowerPC features, it achieves a balance between accuracy and performance that allows complex Xbox 360 games to run on modern hardware.