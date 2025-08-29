cmake_minimum_required(VERSION 3.15)
project(Emulator)

# imgui
#set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../external/imgui)
#include_directories(${IMGUI_DIR})
#include_directories(${IMGUI_DIR}/backends)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/sdl2)
#set(IMGUI_SRC
#    ${IMGUI_DIR}/imgui.cpp
#	${IMGUI_DIR}/imgui_demo.cpp
#    ${IMGUI_DIR}/imgui_draw.cpp
#    ${IMGUI_DIR}/imgui_tables.cpp
#    ${IMGUI_DIR}/imgui_widgets.cpp
#    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp   
#    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp 
#)
#add_library(Runtime SHARED ${SOURCES} ${IMGUI_SRC})


SET(Graphics
    src/Graphics/ImGuiTest.h
    src/Graphics/ImGuiDebugger.h
    src/Graphics/ImGuiDebugger.cpp
    src/Graphics/GraphicsBackend.h
)

if(WIN32)
  list(APPEND Graphics
    src/Graphics/DX12Manager.h
    src/Graphics/DX12Manager.cpp
    src/Graphics/DX12BackendAdapter.h
    src/Graphics/DX12BackendAdapter.cpp)
elseif(APPLE)
  list(APPEND Graphics
    src/Graphics/PlumeBackendAdapter.h
    src/Graphics/PlumeBackendAdapter.cpp)
endif()

SET(Runtime
    src/Runtime/Runtime.h
    src/Runtime/Runtime.cpp
    src/Runtime/MemoryManager.h
    src/Runtime/Kernel/XKernel.h
    src/Runtime/Kernel/XKernel.cpp
    src/Runtime/Kernel/XeThread.h
    src/Runtime/Kernel/XeThread.cpp
    src/Runtime/Xen/Xam_Entry.h
    src/Runtime/Xen/Xam_Impl.h
    src/Runtime/Xen/XboxKrnl_Entry.h
    src/Runtime/Xen/XboxKrnl_Impl.h
    src/Runtime/Xen/XenUtils.h
)

SET(Emu_SRC
    ${Graphics}
    ${Runtime}
    src/pch.h 
    src/pch.cpp
    src/dllmain.cpp
)

add_executable(Emulator ${Emu_SRC})


if(MSVC)
    target_compile_definitions(Emulator PRIVATE "RUNTIME_EXPORTS")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
if(WIN32)
  target_link_libraries(Emulator PRIVATE d3d12 d3d11 dxgi dxguid)
endif()
target_precompile_headers(Emulator PRIVATE src/pch.h)
