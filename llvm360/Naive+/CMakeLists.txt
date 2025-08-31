cmake_minimum_required(VERSION 3.15)

project(Naive+)



# MSVC-specific options
#if(MSVC)
#    # Adjust compiler flags for better compatibility
#    set(CMAKE_C_FLAGS_DEBUG "/ZI /Od /Ob0 /DNDEBUG" CACHE STRING "" FORCE)
#    set(CMAKE_CXX_FLAGS_DEBUG "/ZI /Od /Ob0 /DNDEBUG" CACHE STRING "" FORCE)
#
#    if(LLVM_USE_CRT_RELEASE STREQUAL "MD")
#        set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
#    elseif(LLVM_USE_CRT_RELEASE STREQUAL "MT")
#        set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
#    else()
#        message(FATAL_ERROR "Unsupported LLVM_USE_CRT_RELEASE=${LLVM_USE_CRT_RELEASE}")
#    endif()
#endif()

set(XEX 
    src/Xex/XexLoader.h
    src/Xex/XexLoader.cpp
    src/Xex/XexImage.h
    src/Xex/ImportTable.h
    src/Xex/AES.h
    src/Xex/AES.cpp
)

set(MISC
    src/misc/Utils.h
    src/misc/Utils.cpp
)

set(DECODER
    src/Decoder/Instruction.h
    src/Decoder/InstructionDecoder.cpp
    src/Decoder/InstructionDecoder.h
)

set(IR
    src/IR/Unit/UnitTesting.h
    src/IR/InstructionEmitter.h
    src/IR/IRFunc.cpp
    src/IR/IRFunc.h
    src/IR/IRGenerator.cpp
    src/IR/IRGenerator.h
    src/IR/JumpTables.h
)

set(SRC
    src/LLVM360.cpp
    src/Util.h

    ${XEX}
    ${MISC}
    ${DECODER}
    ${IR}
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_executable(Naive+ ${SRC})
target_compile_definitions(Naive+ PRIVATE DISABLE_DEBUGBREAK=1)


# List of LLVM libs you need:
llvm_map_components_to_libnames(LLVM_LIBS
  core
  support
  irreader
  bitreader
  target
  x86codegen
  x86desc
  x86asmparser
  x86info  
  mc
  mcjit
  mcparser
  nativecodegen
  objwriter
  passes
  transformutils
)

# Optionally link LLD if available. Keep platform frontends as opt-in so we retain
# capability without forcing extra deps on all platforms.
set(LLD_LIBS "")
if(HAVE_LLD)
  option(EMBED_LLD_FRONTEND "Link against an LLD platform frontend" OFF)
  if(EMBED_LLD_FRONTEND)
    if(WIN32)
      list(APPEND LLD_LIBS lldCommon lldCOFF)
    elseif(APPLE)
      list(APPEND LLD_LIBS lldCommon lldMachO)
    else()
      list(APPEND LLD_LIBS lldCommon lldELF)
    endif()
  endif()
endif()

llvm_map_components_to_libnames(LLVM_LIBS support core irreader bitwriter)
target_link_libraries(Naive+ PRIVATE ${LLVM_LIBS} ${LLD_LIBS})
