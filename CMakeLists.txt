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

# LLD is required for this project; ensure it matches LLVM
set(HAVE_LLD FALSE)

# Derive the LLVM install prefix from LLVM_DIR (e.g., .../lib/cmake)
get_filename_component(_LLVM_PREFIX "${LLVM_DIR}/../.." ABSOLUTE)

# Build a curated list of candidate LLD cmake dirs that share the LLVM prefix
set(_LLD_HINTS)

if(DEFINED LLD_DIR)
  # Accept common shapes of LLD_DIR input and normalize to cmake dir candidates
  set(_CANDIDATES
    "${LLD_DIR}"
    "${LLD_DIR}/lib/cmake/lld"
    "${LLD_DIR}/cmake/lld")
  foreach(_C IN LISTS _CANDIDATES)
    # Compare parent two dirs (../..) to the LLVM prefix (../.. of LLVM_DIR)
    get_filename_component(_C_PREFIX "${_C}/../.." ABSOLUTE)
    if(_C_PREFIX STREQUAL _LLVM_PREFIX)
      list(APPEND _LLD_HINTS "${_C}")
    else()
      # Ignore user-provided LLD_DIR that doesn't match the LLVM prefix
      # to avoid pulling in an incompatible lld (e.g., Homebrew lld@19)
    endif()
  endforeach()
  if(_LLD_HINTS STREQUAL "")
    message(STATUS "Ignoring LLD_DIR='${LLD_DIR}' (prefix mismatch vs LLVM at '${_LLVM_PREFIX}').")
  endif()
endif()

# Always prefer the LLD next to the detected LLVM
list(APPEND _LLD_HINTS "${_LLVM_PREFIX}/lib/cmake/lld")

# Only search in our curated hints; do not fall back to system defaults
find_package(LLD CONFIG QUIET NO_DEFAULT_PATH HINTS ${_LLD_HINTS})
if(LLD_FOUND)
  message(STATUS "Found LLD at ${LLD_DIR}")
  set(HAVE_LLD TRUE)
else()
  message(FATAL_ERROR "LLD is required but a matching LLD wasn't found.\n"
    "Detected LLVM at '${_LLVM_PREFIX}'.\n"
    "Tried LLD at: ${_LLD_HINTS}.\n\n"
    "Fix suggestions:\n"
    "- macOS (Homebrew): brew install llvm\n"
    "  Then configure with:\n"
    "    -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm\n"
    "    -DLLD_DIR=$(brew --prefix llvm)/lib/cmake/lld\n"
    "- Avoid mixing versioned lld (e.g., lld@19) with LLVM ${LLVM_PACKAGE_VERSION}.\n")
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
