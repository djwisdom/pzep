# REPL Plugin targets
# Each plugin is a shared library that registers a REPL provider

set(PLUGIN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/plugins)

# Common plugin include directories
set(PLUGIN_INCLUDES
    ${ZEP_ROOT}/include
    ${ZEP_ROOT}/src
)

if(ENABLE_LUA_REPL)
    add_library(lua_plugin SHARED
        ${ZEP_ROOT}/src/plugins/lua_plugin.cpp
        ${ZEP_ROOT}/src/mode_lua_repl.cpp
    )
    target_include_directories(lua_plugin PRIVATE ${PLUGIN_INCLUDES})
    if(TARGET Lua::Lua)
        target_link_libraries(lua_plugin PRIVATE Zep Lua::Lua)
    else()
        target_link_libraries(lua_plugin PRIVATE Zep ${LUA_LIBRARIES})
        target_include_directories(lua_plugin PRIVATE ${LUA_INCLUDE_DIRS})
    endif()
    set_target_properties(lua_plugin PROPERTIES
        OUTPUT_NAME "lua"
        PREFIX ""
        SUFFIX ".plugin"
        RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIR}
    )
endif()

if(ENABLE_DUKTAPE_REPL)
    add_library(duktape_plugin SHARED
        ${ZEP_ROOT}/src/plugins/duktape_plugin.cpp
        ${ZEP_ROOT}/src/mode_duktape_repl.cpp
    )
    target_include_directories(duktape_plugin PRIVATE ${PLUGIN_INCLUDES})
    # Duktape may be provided as a target or via variables
    if(TARGET duktape_lib)
        target_link_libraries(duktape_plugin PRIVATE Zep duktape_lib)
    elseif(TARGET duktape)
        target_link_libraries(duktape_plugin PRIVATE Zep duktape)
    else()
        # Fallback: assume duktape is header-only or already in Zep
        target_link_libraries(duktape_plugin PRIVATE Zep)
    endif()
    set_target_properties(duktape_plugin PROPERTIES
        OUTPUT_NAME "duktape"
        PREFIX ""
        SUFFIX ".plugin"
        RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIR}
    )
endif()

if(ENABLE_QUICKJS_REPL)
    add_library(quickjs_plugin SHARED
        ${ZEP_ROOT}/src/plugins/quickjs_plugin.cpp
        ${ZEP_ROOT}/src/mode_quickjs_repl.cpp
    )
    target_include_directories(quickjs_plugin PRIVATE ${PLUGIN_INCLUDES})
    if(TARGET quickjs_lib)
        target_link_libraries(quickjs_plugin PRIVATE Zep quickjs_lib)
    elseif(TARGET qjs)
        target_link_libraries(quickjs_plugin PRIVATE Zep qjs)
    else()
        target_link_libraries(quickjs_plugin PRIVATE Zep)
    endif()
    set_target_properties(quickjs_plugin PROPERTIES
        OUTPUT_NAME "quickjs"
        PREFIX ""
        SUFFIX ".plugin"
        RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIR}
    )
endif()

# Ensure plugins directory exists
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_DIR})
