cmake_minimum_required(VERSION 3.15)
project(pzep-gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use vcpkg toolchain for dependencies
set(CMAKE_TOOLCHAIN_FILE C:/Users/casse/github/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(VCPKG_TARGET_TRIPLET x64-windows)

# Find raylib from vcpkg
find_package(raylib CONFIG REQUIRED)

# Include directories - use our built Zep
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    C:/Users/casse/github/zep/include
    C:/Users/casse/github/zep/include/zep
    C:/Users/casse/github/zep/include/zep/mcommon
)

# Pre-built library paths
set(ZEP_LIB C:/Users/casse/github/zep/build-pzep-gui/lib/Release/Zep.lib)

# Create executable
add_executable(pzep_gui pzep_gui.cpp)

# Link libraries
target_link_libraries(pzep_gui PRIVATE 
    "${ZEP_LIB}"
    raylib
    opengl32
    gdi32
)