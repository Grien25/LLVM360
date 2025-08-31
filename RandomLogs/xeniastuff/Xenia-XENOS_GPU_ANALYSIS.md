# Xenos GPU Emulation in Xenia Canary

## Overview

The Xenos is a custom ATI graphics processor based on the R400 architecture, specifically designed for the Xbox 360. It features unique capabilities like 10MB of embedded DRAM (EDRAM) and specialized tiling for efficient memory access. Xenia Canary implements comprehensive emulation of the Xenos through command processing, shader translation, and render state management.

## Architecture Implementation

### Command Processor (`src/xenia/gpu`)

The command processor interprets GPU command packets from the ring buffer, processing draw calls, state changes, and memory transfers. The implementation follows the PM4 packet protocol used by the Xbox 360 GPU:

```cpp
// From command_processor.cc - handling PM4 packets
bool CommandProcessor::ExecutePrimaryBuffer(uint32_t start_index,
                                           uint32_t end_index) {
  // Process commands from the ring buffer
  while (read_ptr_ != end_index) {
    uint32_t packet = ReadRegister(CP_RB_RPTR_ADDR);
    uint32_t packet_type = packet >> 30;
    
    switch (packet_type) {
      case 0x00:  // Type-0 (single register write)
        // Handle register write
        break;
      case 0x01:  // Type-1 (consecutive register writes)
        // Handle multiple register writes
        break;
      case 0x02:  // Type-2 (unused)
        break;
      case 0x03:  // Type-3 (operation packets)
        // Handle complex operations like draws
        break;
    }
  }
}
```

### Register Management

The Xenos GPU has hundreds of registers for controlling various aspects of the graphics pipeline. Xenia implements these through a register file system:

```cpp
// From registers.h - register definitions
union alignas(uint32_t) RB_SURFACE_INFO {
  uint32_t value;
  struct {
    uint32_t surface_pitch : 14;    // +0
    uint32_t _pad_14 : 16;          // +14
    xenos::MsaaSamples msaa_samples : 2;  // +30
  };
};

// From register_file.h - register file implementation
class RegisterFile {
 public:
  uint32_t values_[0x10000];  // All registers
  
  template <typename T>
  T Get(uint32_t addr) const {
    return *reinterpret_cast<const T*>(&values_[addr]);
  }
  
  template <typename T>
  void Set(uint32_t addr, T value) {
    *reinterpret_cast<T*>(&values_[addr]) = value;
  }
};
```

## Shader Translation

### Ucode Parsing

Xenos shaders are stored as microcode (ucode) which Xenia parses and translates:

```cpp
// From ucode.cc - ucode disassembly
void UcodeDisassembler::Disasm(uint32_t addr, uint32_t* ucode) {
  // Each instruction is 3 dwords
  uint32_t word0 = ucode[0];
  uint32_t word1 = ucode[1];
  uint32_t word2 = ucode[2];
  
  // Parse instruction type
  uint32_t opcode = word0 & 0x7F;
  switch (opcode) {
    case 0x00:  // NOP
      // ...
    case 0x01:  // MOV
      // ...
    // ... other opcodes
  }
}
```

### SPIR-V Translation

Xenia translates Xenos shaders to SPIR-V for use with modern graphics APIs:

```cpp
// From spirv_shader_translator.cc - SPIR-V generation
class SpirvShaderTranslator {
  // Generate SPIR-V for ALU instructions
  void ProcessVectorAluInstruction(const ParsedAluInstruction& instr) {
    switch (instr.opcode) {
      case AluVectorOpcode::kAdd:
        // Generate SPIR-V for addition
        current_block_->AppendOpAdd(result_type, result_id, src0_id, src1_id);
        break;
      // ... other operations
    }
  }
};
```

### Vertex Shader Export

Vertex shaders in Xenos can export multiple values, which Xenia maps to modern vertex attributes:

```cpp
// From spirv_shader_translator.cc - vertex exports
void SpirvShaderTranslator::ExportVertexData(uint32_t export_index,
                                            uint32_t semantic,
                                            Value* value) {
  switch (export_index) {
    case 0:  // Position
      // Export as gl_Position
      break;
    case 1:  // Point size
      // Export as gl_PointSize
      break;
    default:  // Generic attributes
      // Export as vertex outputs
      break;
  }
}
```

## Render State Management

### Blend States

Xenia implements all of the Xenos blend modes and operations:

```cpp
// From xenos.h - blend operations
enum class BlendOp : uint32_t {
  kAdd = 0,
  kSubtract = 1,
  kMin = 2,
  kMax = 3,
  kRevSubtract = 4,
};

// From vulkan_command_processor.cc - applying blend states
void VulkanCommandProcessor::UpdateBlendMode() {
  const auto& regs = *register_file();
  auto blend_control = regs.Get<reg::RB_BLEND_CONTROL>();
  
  // Convert Xenos blend factors to Vulkan
  VkBlendFactor src_blend = ConvertXenosBlendFactor(blend_control.color_src_blend);
  VkBlendFactor dst_blend = ConvertXenosBlendFactor(blend_control.color_dst_blend);
  VkBlendOp blend_op = ConvertXenosBlendOp(blend_control.color_comb_fcn);
  
  // Apply to graphics pipeline
}
```

### Depth/Stencil States

The depth and stencil testing capabilities are fully implemented:

```cpp
// From xenos.h - stencil operations
enum class StencilOp : uint32_t {
  kKeep = 0,
  kZero = 1,
  kReplace = 2,
  kIncrementClamp = 3,
  kDecrementClamp = 4,
  kInvert = 5,
  kIncrementWrap = 6,
  kDecrementWrap = 7,
};

// From vulkan_command_processor.cc - stencil state handling
void VulkanCommandProcessor::UpdateDepthStencilMode() {
  const auto& regs = *register_file();
  auto depth_control = regs.Get<reg::RB_DEPTHCONTROL>();
  
  // Configure depth/stencil tests
  VkStencilOpState front_stencil = {};
  front_stencil.failOp = ConvertXenosStencilOp(depth_control.stencil_fail);
  front_stencil.passOp = ConvertXenosStencilOp(depth_control.stencil_zpass);
  front_stencil.depthFailOp = ConvertXenosStencilOp(depth_control.stencil_zfail);
  // ...
}
```

## EDRAM (Embedded DRAM) Emulation

One of the most unique features of the Xenos is its 10MB of embedded DRAM, which Xenia emulates through careful tiling management:

```cpp
// From xenos.h - EDRAM properties
constexpr uint32_t kEdramTileWidthSamples = 80;
constexpr uint32_t kEdramTileHeightSamples = 16;
constexpr uint32_t kEdramTileCount = 2048;
constexpr uint32_t kEdramSizeBytes = kEdramTileCount * kEdramTileHeightSamples *
                                   kEdramTileWidthSamples * sizeof(uint32_t);

// From render_target_cache.cc - EDRAM tile management
class RenderTargetCache {
  // Manage EDRAM tile allocation
  uint32_t AllocateEdramRegion(uint32_t tile_count) {
    // Find free contiguous tiles
    for (uint32_t i = 0; i < kEdramTileCount; i++) {
      if (IsEdramRegionFree(i, tile_count)) {
        MarkEdramRegionUsed(i, tile_count);
        return i;
      }
    }
    return UINT32_MAX;  // No space
  }
};
```

## Texture System

### Texture Format Support

Xenia supports all Xenos texture formats, including compressed and packed formats:

```cpp
// From xenos.h - texture formats
enum class TextureFormat : uint32_t {
  k_1_REVERSE = 0,
  k_1 = 1,
  k_8 = 2,
  k_1_5_5_5 = 3,
  k_5_6_5 = 4,
  // ... many more formats
  k_DXT1 = 18,
  k_DXT2_3 = 19,
  k_DXT4_5 = 20,
  // ...
};

// From texture_info.cc - format conversion
TextureFormatInfo GetTextureFormatInfo(TextureFormat format) {
  switch (format) {
    case TextureFormat::k_8:
      return {1, 1, 1};  // 1 component, 1 byte per pixel
    case TextureFormat::k_5_6_5:
      return {3, 2, 1};  // 3 components, 2 bytes per pixel
    // ...
  }
}
```

### Tiled Texture Handling

Xenos uses tiled memory layouts for better cache performance, which Xenia emulates:

```cpp
// From texture_info.cc - tiled addressing
uint32_t GetTiledOffset2D(uint32_t x, uint32_t y, uint32_t width, uint32_t bytes_per_block) {
  // Convert linear coordinates to tiled offset
  uint32_t macro = ((x >> 5) + (y >> 5) * ((width + 31) >> 5)) << 12;
  uint32_t micro = ((x & 7) + ((y & 0x1F) << 3) + ((x >> 3) & 7) * 64) * bytes_per_block;
  return macro + micro;
}
```

## Memory Export (MemExport)

The Xenos supports writing arbitrary formatted data with random access, which Xenia implements:

```cpp
// From xenos.h - memory export stream
union alignas(uint32_t) xe_gpu_memexport_stream_t {
  struct {
    uint32_t base_address : 30;  // Physical address >> 2
    uint32_t const_0x1 : 2;
    // ...
    Endian128 endianness : 3;
    ColorFormat format : 6;
    // ...
  };
};

// From command_processor.cc - handling memory exports
void CommandProcessor::ExecuteMemExport(uint32_t stream, const float* data) {
  // Decode the stream constant
  auto stream_const = *reinterpret_cast<const xe_gpu_memexport_stream_t*>(&stream);
  
  // Write data to guest memory with proper format conversion
  uint32_t address = stream_const.base_address << 2;
  WriteFormattedData(address, data, stream_const.format, stream_const.endianness);
}
```

## Performance Optimizations

### Pipeline Caching

Xenia caches graphics pipelines to avoid expensive state transitions:

```cpp
// From vulkan_pipeline_cache.cc - pipeline caching
class VulkanPipelineCache {
  // Key for pipeline lookup
  struct PipelineKey {
    uint64_t render_target_key;
    uint64_t depth_stencil_key;
    uint64_t blend_key;
    // ...
  };
  
  VkPipeline GetPipeline(const PipelineKey& key) {
    auto it = pipeline_map_.find(key);
    if (it != pipeline_map_.end()) {
      return it->second;
    }
    
    // Create new pipeline
    VkPipeline pipeline = CreatePipeline(key);
    pipeline_map_[key] = pipeline;
    return pipeline;
  }
};
```

### Texture Caching

Textures are cached to avoid redundant processing:

```cpp
// From texture_cache.cc - texture caching
class TextureCache {
  struct TextureKey {
    uint32_t fetch_address;
    uint32_t format;
    uint32_t width, height;
    // ...
  };
  
  TextureEntry* FindOrCreateTexture(const TextureKey& key) {
    auto it = texture_map_.find(key);
    if (it != texture_map_.end()) {
      return it->second.get();
    }
    
    // Create new texture
    auto entry = std::make_unique<TextureEntry>();
    // ... initialize texture
    texture_map_[key] = std::move(entry);
    return entry.get();
  }
};
```

## Challenges and Solutions

### Format Conversion

Converting between Xenos formats and modern APIs requires careful handling:

```cpp
// From texture_conversion.cc - format conversion
void ConvertTextureData(TextureFormat src_format, TextureFormat dst_format,
                       const void* src_data, void* dst_data,
                       uint32_t width, uint32_t height) {
  switch (src_format) {
    case TextureFormat::k_2_10_10_10:
      // Convert 2_10_10_10 to RGBA8
      Convert2_10_10_10ToRGBA8(src_data, dst_data, width, height);
      break;
    // ... other conversions
  }
}
```

### MSAA Handling

Multisample anti-aliasing requires special handling in the resolve process:

```cpp
// From command_processor.cc - resolve operations
void CommandProcessor::Resolve() {
  const auto& regs = *register_file();
  auto copy_control = regs.Get<reg::RB_COPY_CONTROL>();
  
  // Handle different MSAA sample modes
  switch (copy_control.copy_sample_select) {
    case CopySampleSelect::k0:
      // Resolve single sample
      break;
    case CopySampleSelect::k0123:
      // Average all samples
      break;
  }
}
```

## Conclusion

Xenia's Xenos GPU emulation is a comprehensive implementation that handles the unique features of the Xbox 360's graphics processor. Through careful modeling of the command processor, shader translation, render state management, and EDRAM emulation, it successfully recreates the graphics capabilities of the original hardware while leveraging modern graphics APIs for performance.