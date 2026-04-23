
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
${ZEP_ROOT}/include/zep/window.h
${ZEP_ROOT}/include/zep/git.h
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
${ZEP_ROOT}/src/commands_terminal.cpp
    ${ZEP_ROOT}/src/terminal.cpp
)

# Conditionally include REPL providers based on CMake options
if(ENABLE_LUA_REPL)
    list(APPEND ZEP_SOURCE ${ZEP_ROOT}/src/mode_lua_repl.cpp)
endif()
if(ENABLE_DUKTAPE_REPL)
    list(APPEND ZEP_SOURCE ${ZEP_ROOT}/src/mode_duktape_repl.cpp)
endif()
if(ENABLE_QUICKJS_REPL)
    list(APPEND ZEP_SOURCE ${ZEP_ROOT}/src/mode_quickjs_repl.cpp)
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
    target_link_libraries(Zep PRIVATE Lua::Lua)
    target_compile_definitions(Zep INTERFACE ZEP_ENABLE_LUA_REPL)
endif()
if(ENABLE_DUKTAPE_REPL)
    # Duktape is typically header-only or single-source; if found via vcpkg use imported target
    if(TARGET duktape::duktape)
        target_link_libraries(Zep PRIVATE duktape::duktape)
    else()
        # Assume duktape.c will be compiled separately or is included elsewhere
        message(WARNING "Duktape target not found – ensure duktape.c is compiled")
    endif()
    target_compile_definitions(Zep INTERFACE ZEP_ENABLE_DUKTAPE_REPL)
endif()
if(ENABLE_QUICKJS_REPL)
    # QuickJS: try to find via pkg-config or vcpkg
    find_package(QuickJS QUIET)
    if(QuickJS_FOUND)
        target_link_libraries(Zep PRIVATE QuickJS::QuickJS)
    else()
        # Look for system quickjs.h and libquickjs
        find_path(QUICKJS_INCLUDE_DIR NAMES quickjs.h)
        find_library(QUICKJS_LIBRARY NAMES quickjs)
        if(QUICKJS_INCLUDE_DIR AND QUICKJS_LIBRARY)
            target_include_directories(Zep PRIVATE ${QUICKJS_INCLUDE_DIR})
            target_link_libraries(Zep PRIVATE ${QUICKJS_LIBRARY})
        else()
            message(WARNING "QuickJS not found – either install via vcpkg (quickjs) or set QUICKJS_INCLUDE_DIR/QUICKJS_LIBRARY")
        endif()
    endif()
    target_compile_definitions(Zep INTERFACE ZEP_ENABLE_QUICKJS_REPL)
endif()

# Set locations for components
set_target_properties(Zep PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install the binary
install(TARGETS Zep
    EXPORT zep-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${LIBLEGACY_INCLUDE_DIRS}
)

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

