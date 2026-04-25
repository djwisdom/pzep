cmake_minimum_required(VERSION 3.15)

message(STATUS "CMakeLists: Zep")

# Global Options
option(BUILD_QT "Make Qt Library" OFF)
option(BUILD_IMGUI "Make Imgui Library" OFF)
option(BUILD_DEMOS "Make the demo app" ON)
option(BUILD_TESTS "Make the tests" ON)
option(ZEP_FEATURE_CPP_FILE_SYSTEM "Default File system enabled" ON)

# REPL Provider Options
option(ENABLE_LUA_REPL "Enable Lua scripting support" OFF)
option(ENABLE_DUKTAPE_REPL "Enable Duktape JavaScript support" OFF)
option(ENABLE_QUICKJS_REPL "Enable QuickJS JavaScript support" OFF)

# Global Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "-lGLEW")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)

set(CMAKE_DEBUG_POSTFIX "-debug")
set(CMAKE_RELWITHDEBINFO_POSTFIX "-reldbg")

if (BUILD_DEMOS)
if (WIN32)
set(VCPKG_TARGET_TRIPLET x64-windows-static-md)
endif()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
endif()

project(Zep
    LANGUAGES CXX C
    VERSION 0.5.33
)

# Version components for use in code
add_compile_definitions(
    ZEP_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    ZEP_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    ZEP_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Find Lua if enabled
if(ENABLE_LUA_REPL)
    find_package(Lua REQUIRED)
    message(STATUS "Lua found: ${LUA_VERSION_STRING}")
endif()

# Find Duktape if enabled (header-only, but may have CMake package)
if(ENABLE_DUKTAPE_REPL)
    find_package(duktape CONFIG QUIET)
    if(duktape_FOUND)
        message(STATUS "Duktape found via CMake config")
    else()
        message(STATUS "Duktape not found via find_package - will use bundled or assume source available")
        # Duktape is typically single-source; if not installed, source must be present
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Generate version information with git hash
include(cmake/git_version.cmake)

# Can I remove these now?
if (ZEP_FEATURE_CPP_FILE_SYSTEM)
add_definitions(-DZEP_FEATURE_CPP_FILE_SYSTEM)
endif()

# config_app.h checks
# This makes a config_shared.h file which can be included for system settings
#  Must be after setting up the platforms
set (ZEP_ROOT ${CMAKE_CURRENT_LIST_DIR})

configure_file(${ZEP_ROOT}/cmake/config_app.h.cmake ${CMAKE_BINARY_DIR}/config_app.h)

include(GNUInstallDirs)

# Set all compiler flags
include(cmake/all.cmake)

# Functions for file copying
include(cmake/copy_files.cmake)

enable_testing()

# The main library
add_subdirectory(src)
add_subdirectory(tests)
if(EXISTS "${CMAKE_SOURCE_DIR}/apps/pzep-gui" AND BUILD_DEMOS)
    add_subdirectory(apps/pzep-gui)
endif()

# Make the CMake bits that ensure find_package does the right thing
# Skipping install rules for now to avoid dependency export issues
# install(EXPORT zep-targets
#     FILE zep-targets.cmake
#     NAMESPACE Zep::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zep
# )

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${ZEP_ROOT}/cmake/zep-config.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/zep-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zep
)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/cmake/zep-config-version.cmake
    VERSION 1.0
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES
        ${CMAKE_BINARY_DIR}/cmake/zep-config.cmake
        ${CMAKE_BINARY_DIR}/cmake/zep-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zep
)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/cpack_installer.cmake)
