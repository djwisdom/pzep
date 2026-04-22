cmake_minimum_required(VERSION 3.15)

project(pzep-gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib QUIET)

if(WIN32)
    set(RAYLIB_INCLUDE "C:/vcpkg/installed/x64-windows/include")
    set(RAYLIB_LIB "C:/vcpkg/installed/x64-windows/lib")
endif()

add_executable(pzep-gui
    main.cpp
    ../../src/raylib/display_raylib.cpp
)

target_include_directories(pzep-gui PRIVATE
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/../../src
    ${CMAKE_SOURCE_DIR}/../../src/raylib
    ${RAYLIB_INCLUDE}
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(pzep-gui PRIVATE
    ${RAYLIB_LIB}/raylib.lib
    opengl32
    gdi32
    winmm
    user32
    ${CMAKE_SOURCE_DIR}/../../build/lib/Release/Zep.lib
)

if(WIN32)
    target_compile_definitions(pzep-gui PRIVATE _CRT_SECURE_NO_WARNINGS)
    
    # Copy DLLs to output directory after build
    add_custom_command(TARGET pzep-gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RAYLIB_LIB}/../bin/raylib.dll
            $<TARGET_FILE_DIR:pzep-gui>/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RAYLIB_LIB}/../bin/glfw3.dll
            $<TARGET_FILE_DIR:pzep-gui>/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RAYLIB_LIB}/../bin/freetype.dll
            $<TARGET_FILE_DIR:pzep-gui>/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RAYLIB_LIB}/../bin/libpng16.dll
            $<TARGET_FILE_DIR:pzep-gui>/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RAYLIB_LIB}/../bin/zlib1.dll
            $<TARGET_FILE_DIR:pzep-gui>/
        COMMENT "Copying Raylib dependencies to output directory"
    )
endif()

message(STATUS "pZep-GUI built")