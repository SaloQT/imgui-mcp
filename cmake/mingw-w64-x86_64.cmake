# CMake toolchain file for cross-compiling to Windows x64 with MinGW
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# SDL2 location (vendored in cross/)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(SDL2_DIR "${PROJECT_ROOT}/cross")
set(SDL2_INCLUDE_DIR "${PROJECT_ROOT}/cross/include/SDL2;${PROJECT_ROOT}/cross/include")
set(SDL2_LIBRARY "${PROJECT_ROOT}/cross/lib/libSDL2.dll.a")

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Static link libgcc/libstdc++ for portable binary
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
