cmake_minimum_required(VERSION 3.15)

project(pzep-gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib is already found by top-level CMake; just use it
if(NOT TARGET raylib)
    find_package(raylib CONFIG REQUIRED)
endif()

if(WIN32)
    add_executable(pzep-gui WIN32
        main.cpp
        version_dialog.cpp
        ../../src/raylib/display_raylib.cpp
    )
else()
    add_executable(pzep-gui
        main.cpp
        version_dialog.cpp
        ../../src/raylib/display_raylib.cpp
    )
endif()

target_include_directories(pzep-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/raylib
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(pzep-gui PRIVATE
    Zep
    raylib
    # Windows system libs that raylib may need
    $<$<PLATFORM_ID:Windows>:opengl32>
    $<$<PLATFORM_ID:Windows>:gdi32>
    $<$<PLATFORM_ID:Windows>:winmm>
    $<$<PLATFORM_ID:Windows>:user32>
)

if(WIN32)
    # Build as Windows GUI app (no console window). Use main as entry point.
    set_target_properties(pzep-gui PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
    target_compile_definitions(pzep-gui PRIVATE _CRT_SECURE_NO_WARNINGS)
    
    # Copy raylib DLL to output directory if using dynamic build
    if(TARGET raylib)
        get_target_property(RAYLIB_TYPE raylib TYPE)
        if(RAYLIB_TYPE STREQUAL "SHARED_LIBRARY")
            add_custom_command(TARGET pzep-gui POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:raylib>
                    $<TARGET_FILE_DIR:pzep-gui>/
                COMMENT "Copying raylib.dll to output directory"
            )
        endif()
    endif()
endif()

message(STATUS "pZep-GUI built")