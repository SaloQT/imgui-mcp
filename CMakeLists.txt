cmake_minimum_required(VERSION 3.16)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" IMGUI_MCP_VERSION LIMIT_COUNT 1)
project(imgui_mcp VERSION "${IMGUI_MCP_VERSION}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Find SDL2 (cross-platform) ──────────────────────────────────────────────
if(WIN32)
    # Cross-compilation: toolchain file sets SDL2_INCLUDE_DIR and SDL2_LIBRARY
    if(DEFINED SDL2_INCLUDE_DIR AND DEFINED SDL2_LIBRARY)
        set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
        set(SDL2_LIBRARIES ${SDL2_LIBRARY})
        set(SDL2_FOUND TRUE)
        message(STATUS "Using SDL2 from toolchain: ${SDL2_INCLUDE_DIR}")
    else()
        # Native Windows: try find_package (vcpkg), then manual search
        find_package(SDL2 QUIET)
        if(NOT SDL2_FOUND)
            set(SDL2_SEARCH_PATHS
                "C:/SDL2"
                "C:/vcpkg/installed/x64-windows"
                "$ENV{USERPROFILE}/vcpkg/installed/x64-windows"
                "C:/Program Files/SDL2"
            )
            foreach(p ${SDL2_SEARCH_PATHS})
                if(EXISTS "${p}/include/SDL2/SDL.h")
                    set(SDL2_INCLUDE_DIRS "${p}/include/SDL2;${p}/include")
                    set(SDL2_LIBRARIES "${p}/lib/x64/SDL2.lib;${p}/lib/x64/SDL2main.lib")
                    set(SDL2_FOUND TRUE)
                    message(STATUS "Found SDL2 manually: ${p}")
                    break()
                endif()
            endforeach()
        endif()
        if(NOT SDL2_FOUND)
            message(FATAL_ERROR "SDL2 not found. Install via: vcpkg install sdl2:x64-windows")
        endif()
    endif()
else()
    # Linux/macOS: use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# ─── Find OpenGL ─────────────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)

# ─── Dear ImGui sources (vendored v1.92.8) ───────────────────────────────────
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ─── Main application ────────────────────────────────────────────────────────
add_executable(imgui_mcp_app
    src/main.cpp
    src/render.cpp
    src/commands.cpp
    ${IMGUI_SOURCES}
)

if(WIN32)
    enable_language(RC)
    target_sources(imgui_mcp_app PRIVATE assets/windows/app.rc)
endif()

target_include_directories(imgui_mcp_app PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    src
)

# Platform-specific linking
if(WIN32)
    # Console subsystem (app uses stdin/stdout for MCP IPC)
    if(MINGW)
        target_link_options(imgui_mcp_app PRIVATE -mconsole -Wl,--no-insert-timestamp)
    elseif(MSVC)
        target_link_options(imgui_mcp_app PRIVATE /Brepro)
    endif()
    target_link_libraries(imgui_mcp_app PRIVATE
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        opengl32
        gdi32
        dwmapi
    )
    target_compile_definitions(imgui_mcp_app PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_link_libraries(imgui_mcp_app PRIVATE
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        pthread
    )
    target_compile_definitions(imgui_mcp_app PRIVATE ${SDL2_CFLAGS_OTHER})
endif()

target_compile_definitions(imgui_mcp_app PRIVATE
    IMGUI_MCP_VERSION="${PROJECT_VERSION}"
)

# ─── Install ─────────────────────────────────────────────────────────────────
install(TARGETS imgui_mcp_app RUNTIME DESTINATION bin)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME python_and_native_protocol
            COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests -v
        )
        set_tests_properties(python_and_native_protocol PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            ENVIRONMENT "IMGUI_MCP_APP=$<TARGET_FILE:imgui_mcp_app>;IMGUI_MCP_REQUIRE_NATIVE=1"
            TIMEOUT 60
        )
    endif()
endif()
