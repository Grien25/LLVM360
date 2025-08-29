cmake_minimum_required(VERSION 3.15)
project(LLVM360)

# LLVM discovery
# 1) Respect user-provided LLVM_DIR
# 2) On macOS, try llvm-config and common Homebrew prefixes
# 3) Fallback to a configurable Windows install hint

set(LLVM_DIR_INST "C:/LLVM19" CACHE PATH "Optional LLVM install hint (Windows)")

if(APPLE AND NOT DEFINED LLVM_DIR)
  find_program(LLVM_CONFIG_EXE NAMES llvm-config llvm-config-19 llvm-config-18
    PATHS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
  if(LLVM_CONFIG_EXE)
    execute_process(
      COMMAND ${LLVM_CONFIG_EXE} --cmakedir
      OUTPUT_VARIABLE LLVM_CMAKE_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(EXISTS "${LLVM_CMAKE_DIR}")
      set(LLVM_DIR "${LLVM_CMAKE_DIR}" CACHE PATH "Path to LLVMConfig.cmake")
    endif()
  endif()
  if(NOT DEFINED LLVM_DIR)
    if(EXISTS "/opt/homebrew/opt/llvm/lib/cmake/llvm")
      set(LLVM_DIR "/opt/homebrew/opt/llvm/lib/cmake/llvm" CACHE PATH "")
    elseif(EXISTS "/usr/local/opt/llvm/lib/cmake/llvm")
      set(LLVM_DIR "/usr/local/opt/llvm/lib/cmake/llvm" CACHE PATH "")
    endif()
  endif()
endif()

if(NOT DEFINED LLVM_DIR)
  set(LLVM_DIR ${LLVM_DIR_INST}/lib/cmake/llvm CACHE PATH "")
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# LLD is optional; use if available
if(NOT DEFINED LLD_DIR)
  # Try to derive from LLVM_DIR
  get_filename_component(_LLVM_PREFIX "${LLVM_DIR}/../.." ABSOLUTE)
  if(EXISTS "${_LLVM_PREFIX}/lib/cmake/lld")
    set(LLD_DIR "${_LLVM_PREFIX}/lib/cmake/lld" CACHE PATH "")
  elseif(EXISTS "${LLVM_DIR_INST}/lib/cmake/lld")
    set(LLD_DIR "${LLVM_DIR_INST}/lib/cmake/lld" CACHE PATH "")
  endif()
endif()
find_package(LLD CONFIG QUIET)
if(LLD_FOUND)
  message(STATUS "Found LLD at ${LLD_DIR}")
  set(HAVE_LLD TRUE)
else()
  message(STATUS "LLD not found; continuing without LLD")
endif()

# Platform-specific definitions
if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  # Static linkage specifics only under MSVC
  if(MSVC)
    add_definitions(-DLLVM_STATIC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    # Windows SDK version hint
    set(CMAKE_SYSTEM_VERSION 10.0.19041.0)
  endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(REMOVE_TARGETS
    AArch64TargetParserTableGen
    acc_gen
    ARMTargetParserTableGen
    intrinsics_gen
    omp_gen
    RISCVTargetParserTableGen
)
foreach(TARGET_NAME IN LISTS REMOVE_TARGETS)
    if(TARGET ${TARGET_NAME})
        set_property(TARGET ${TARGET_NAME} PROPERTY EXCLUDE_FROM_ALL TRUE)
        set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "llvm_gen")
    endif()
endforeach()

add_subdirectory(llvm360/Naive+)

# Emulator is Windows-only (uses Win32 + D3D12)
if(WIN32)
  add_subdirectory(llvm360/Emulator)
endif()
