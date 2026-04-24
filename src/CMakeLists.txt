
SET(ZEP_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

SET(ZEP_SOURCE
${ZEP_ROOT}/include/zep/buffer.h
${ZEP_ROOT}/include/zep/range_markers.h
${ZEP_ROOT}/include/zep/glyph_iterator.h
${ZEP_ROOT}/include/zep/commands.h
${ZEP_ROOT}/include/zep/display.h
${ZEP_ROOT}/include/zep/editor.h
${ZEP_ROOT}/include/zep/filesystem.h
${ZEP_ROOT}/include/zep/indexer.h
${ZEP_ROOT}/include/zep/keymap.h
${ZEP_ROOT}/include/zep/line_widgets.h
${ZEP_ROOT}/include/zep/mcommon/animation/timer.h
${ZEP_ROOT}/include/zep/mcommon/file/cpptoml.h
${ZEP_ROOT}/include/zep/mcommon/file/path.h
${ZEP_ROOT}/include/zep/mcommon/logger.h
${ZEP_ROOT}/include/zep/mcommon/string/stringutils.h
${ZEP_ROOT}/include/zep/mcommon/threadutils.h
${ZEP_ROOT}/include/zep/mode.h
${ZEP_ROOT}/include/zep/mode_search.h
${ZEP_ROOT}/include/zep/mode_standard.h
${ZEP_ROOT}/include/zep/mode_tutorial.h
${ZEP_ROOT}/include/zep/mode_tree.h
${ZEP_ROOT}/include/zep/mode_vim.h
${ZEP_ROOT}/include/zep/mode_repl.h
${ZEP_ROOT}/include/zep/mode_terminal.h
${ZEP_ROOT}/include/zep/regress.h
${ZEP_ROOT}/include/zep/scroller.h
${ZEP_ROOT}/include/zep/splits.h
${ZEP_ROOT}/include/zep/syntax.h
${ZEP_ROOT}/include/zep/syntax_providers.h
${ZEP_ROOT}/include/zep/syntax_rainbow_brackets.h
${ZEP_ROOT}/include/zep/syntax_tree.h
${ZEP_ROOT}/include/zep/syntax_markdown.h
${ZEP_ROOT}/include/zep/tab_window.h
${ZEP_ROOT}/include/zep/theme.h
${ZEP_ROOT}/include/zep/repl_plugin.h
${ZEP_ROOT}/include/zep/window.h
${ZEP_ROOT}/include/zep/git.h
${ZEP_ROOT}/include/zep/commands_repl.h
${ZEP_ROOT}/include/zep/commands_tutor.h
${ZEP_ROOT}/include/zep/terminal.h
${ZEP_ROOT}/src/CMakeLists.txt
${ZEP_ROOT}/src/buffer.cpp
${ZEP_ROOT}/src/range_markers.cpp
${ZEP_ROOT}/src/glyph_iterator.cpp
${ZEP_ROOT}/src/commands.cpp
${ZEP_ROOT}/src/display.cpp
${ZEP_ROOT}/src/editor.cpp
${ZEP_ROOT}/src/filesystem.cpp
${ZEP_ROOT}/src/git.cpp
${ZEP_ROOT}/src/fold.cpp
${ZEP_ROOT}/src/raylib/display_raylib.cpp
${ZEP_ROOT}/src/indexer.cpp
${ZEP_ROOT}/src/keymap.cpp
${ZEP_ROOT}/src/line_widgets.cpp
${ZEP_ROOT}/src/mcommon/animation/timer.cpp
${ZEP_ROOT}/src/mcommon/string/stringutils.cpp
${ZEP_ROOT}/src/mcommon/file/path.cpp
${ZEP_ROOT}/src/mode.cpp
${ZEP_ROOT}/src/mode_search.cpp
${ZEP_ROOT}/src/mode_standard.cpp
${ZEP_ROOT}/src/mode_tree.cpp
    ${ZEP_ROOT}/src/mode_vim.cpp
    ${ZEP_ROOT}/src/mode_repl.cpp
    ${ZEP_ROOT}/src/mode_terminal.cpp
    ${ZEP_ROOT}/src/mode_tutorial.cpp
    ${ZEP_ROOT}/src/regress.cpp
${ZEP_ROOT}/src/scroller.cpp
${ZEP_ROOT}/src/splits.cpp
${ZEP_ROOT}/src/syntax.cpp
${ZEP_ROOT}/src/syntax_providers.cpp
${ZEP_ROOT}/src/syntax_rainbow_brackets.cpp
${ZEP_ROOT}/src/syntax_tree.cpp
${ZEP_ROOT}/src/syntax_markdown.cpp
${ZEP_ROOT}/src/tab_window.cpp
${ZEP_ROOT}/src/theme.cpp
${ZEP_ROOT}/src/window.cpp
${ZEP_ROOT}/src/commands_tutor.cpp
${ZEP_ROOT}/src/commands_repl.cpp
${ZEP_ROOT}/src/repl_plugin_loader.cpp
${ZEP_ROOT}/src/commands_terminal.cpp
${ZEP_ROOT}/src/terminal.cpp
)

# Conditionally include REPL providers based on CMake options
# If BUILD_REPL_PLUGINS is ON, providers are built as separate plugins and excluded from static library
if(NOT BUILD_REPL_PLUGINS)
    if(ENABLE_LUA_REPL)
        list(APPEND ZEP_SOURCE ${ZEP_ROOT}/src/mode_lua_repl.cpp)
    endif()
    if(ENABLE_DUKTAPE_REPL)
        list(APPEND ZEP_SOURCE ${ZEP_ROOT}/src/mode_duktape_repl.cpp)
    endif()
    if(ENABLE_QUICKJS_REPL)
        list(APPEND ZEP_SOURCE ${ZEP_ROOT}/src/mode_quickjs_repl.cpp)
    endif()
endif()

set(SOURCES ${ZEP_SOURCE})

LIST(APPEND SRC_INCLUDE ${ZEP_ROOT}/src ${ZEP_ROOT}/src/mcommon)

# Create the library - No depenencies on anything else in this file
 add_library(Zep STATIC ${SOURCES})
 add_library(Zep::Zep ALIAS Zep)

 # Ensure architecture macro is defined for Windows headers
 if(MSVC)
     if(CMAKE_SIZEOF_VOID_P EQUAL 8)
         target_compile_definitions(Zep PRIVATE _AMD64_)
     else()
         target_compile_definitions(Zep PRIVATE _X86_)
     endif()
 endif()

target_include_directories(Zep 
    PRIVATE
        ${ZEP_ROOT}/src/mcommon
        ${ZEP_ROOT}/src
        ${SRC_INCLUDE}
        ${CMAKE_BINARY_DIR}
        ${ZEP_ROOT}/include
        C:/Users/casse/github/vcpkg/installed/x64-windows/include
    PUBLIC
        $<INSTALL_INTERFACE:include>
      )

if (TARGET_LINUX)
target_link_libraries(Zep
	PUBLIC
	stdc++fs
)
endif()

# Link REPL provider libraries
if(ENABLE_LUA_REPL)
    # Lua: vcpkg provides LUA_INCLUDE_DIR and LUA_LIBRARIES variables
    if(TARGET Lua::Lua)
        target_link_libraries(Zep PRIVATE Lua::Lua)
    elseif(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
        target_include_directories(Zep PRIVATE ${LUA_INCLUDE_DIR})
        target_link_libraries(Zep PRIVATE ${LUA_LIBRARIES})
    else()
        message(FATAL_ERROR "Lua not found. Install Lua or set LUA_INCLUDE_DIR and LUA_LIBRARIES.")
    endif()
    if(NOT BUILD_REPL_PLUGINS)
        if(NOT BUILD_REPL_PLUGINS)
        target_compile_definitions(Zep PUBLIC ZEP_ENABLE_LUA_REPL)
    endif()
    endif()
endif()

if(ENABLE_DUKTAPE_REPL)
    # Duktape: vcpkg provides DUKTAPE_INCLUDE_DIRS and DUKTAPE_LIBRARIES
    find_package(duktape CONFIG QUIET)
    if(duktape_FOUND AND DUKTAPE_INCLUDE_DIRS AND DUKTAPE_LIBRARIES)
        target_include_directories(Zep PRIVATE ${DUKTAPE_INCLUDE_DIRS})
        target_link_libraries(Zep PRIVATE ${DUKTAPE_LIBRARIES})
    else()
        # Fallback: build from bundled source
        if(NOT TARGET duktape_lib)
            add_library(duktape_lib STATIC ${ZEP_ROOT}/third_party/duktape/src/duktape.c)
            target_include_directories(duktape_lib PUBLIC ${ZEP_ROOT}/third_party/duktape/src)
        endif()
        target_link_libraries(Zep PRIVATE duktape_lib)
    endif()
    target_compile_definitions(Zep PUBLIC ZEP_ENABLE_DUKTAPE_REPL)
endif()

if(ENABLE_QUICKJS_REPL)
    # QuickJS-NG: vcpkg provides qjs target and QJS_INCLUDE_DIR, QJS_LIBRARY
    find_package(qjs CONFIG QUIET)
    if(TARGET qjs)
        target_link_libraries(Zep PRIVATE qjs)
    else()
        # Fallback: build from bundled original QuickJS source
        if(NOT TARGET quickjs_lib)
            add_library(quickjs_lib STATIC
                ${ZEP_ROOT}/third_party/quickjs/quickjs.c
                ${ZEP_ROOT}/third_party/quickjs/quickjs-libc.c
                ${ZEP_ROOT}/third_party/quickjs/cutils.c
                ${ZEP_ROOT}/third_party/quickjs/dtoa.c
                ${ZEP_ROOT}/third_party/quickjs/libregexp.c
                ${ZEP_ROOT}/third_party/quickjs/libunicode.c
            )
            target_include_directories(quickjs_lib PUBLIC ${ZEP_ROOT}/third_party/quickjs)
        endif()
        target_link_libraries(Zep PRIVATE quickjs_lib)
    endif()
    target_compile_definitions(Zep PUBLIC ZEP_ENABLE_QUICKJS_REPL)
endif()

# Set locations for components
set_target_properties(Zep PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install the binary
# Skipping install to avoid dependency export issues during development
# install(TARGETS Zep
#     EXPORT zep-targets
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#     INCLUDES DESTINATION ${LIBLEGACY_INCLUDE_DIRS}
# )

# Workaround for if this file is included as part of another project 
if (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
set (CMAKE_INSTALL_INCLUDEDIR ${ZEP_ROOT}/bin/zep)
endif()

# Install the include files
install(DIRECTORY ${ZEP_ROOT}/include/zep
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)


# For IDE
source_group (Zep\\common REGULAR_EXPRESSION "src/mcommon/.*")
source_group (Zep\\common REGULAR_EXPRESSION "include/zep/mcommon/.*")
source_group (Zep REGULAR_EXPRESSION "src/.*")
source_group (Zep\\include REGULAR_EXPRESSION "include/zep/.*")
source_group (Zep\\repl REGULAR_EXPRESSION "src/mode_lua_repl.cpp")
source_group (Zep\\repl REGULAR_EXPRESSION "src/mode_duktape_repl.cpp")
source_group (Zep\\repl REGULAR_EXPRESSION "src/mode_quickjs_repl.cpp")

