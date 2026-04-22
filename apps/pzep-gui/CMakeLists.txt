cmake_minimum_required(VERSION 3.15)

project(pzep_gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib QUIET)

if(WIN32)
    set(RAYLIB_INCLUDE "C:/vcpkg/installed/x64-windows/include")
    set(RAYLIB_LIB "C:/vcpkg/installed/x64-windows/lib")
endif()

add_executable(pzep_gui
    main.cpp
    ../../src/raylib/display_raylib.cpp
)

target_include_directories(pzep_gui PRIVATE
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/../../src
    ${CMAKE_SOURCE_DIR}/../../src/raylib
    ${RAYLIB_INCLUDE}
)

target_link_libraries(pzep_gui PRIVATE
    ${RAYLIB_LIB}/raylib.lib
    opengl32
    gdi32
    winmm
    ${CMAKE_SOURCE_DIR}/../../build/lib/Release/Zep.lib
)

if(WIN32)
    target_compile_definitions(pzep_gui PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

message(STATUS "pZep-GUI built")