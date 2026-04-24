#pragma once

#include <cstdint>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to editor (C++ ZepEditor* cast to void*)
typedef void* ZepEditorHandle;

// Plugin metadata structure
typedef struct
{
    const char* name; // Short name, e.g. "lua", "duktape", "quickjs"
    const char* display_name; // Display name, e.g. "Lua REPL", "Duktape JavaScript"
    const char* version; // Plugin version string
    const char* description; // Short description
} ZepReplPluginInfo;

// Plugin lifecycle functions

// Initialize the plugin and register its commands with the editor.
// Returns 0 on success, non-zero on failure.
typedef int (*ZepReplPluginInitFunc)(ZepEditorHandle editor);

// Shutdown the plugin, unregister commands, and cleanup resources.
// Called when plugin is unloaded (or at program exit).
typedef void (*ZepReplPluginShutdownFunc)(ZepEditorHandle editor);

// Query plugin metadata (can be called without initialization)
typedef const ZepReplPluginInfo* (*ZepReplPluginInfoFunc)();

// Combined plugin interface structure
typedef struct
{
    ZepReplPluginInfoFunc get_info;
    ZepReplPluginInitFunc init;
    ZepReplPluginShutdownFunc shutdown;
} ZepReplPluginAPI;

// Standard plugin entry point - returns the plugin's API table
// Must be exported with plain C linkage and the name "ZepReplPluginEntry"
// Example:
//   extern "C" ZEP_REPL_PLUGIN_EXPORT const ZepReplPluginAPI* ZepReplPluginEntry()
typedef const ZepReplPluginAPI* (*ZepReplPluginEntryFunc)();

// Platform-specific export macros
#ifdef _WIN32
#ifdef ZEP_REPL_PLUGIN_BUILD
#define ZEP_REPL_PLUGIN_EXPORT __declspec(dllexport)
#else
#define ZEP_REPL_PLUGIN_EXPORT __declspec(dllimport)
#endif
#else
#ifdef ZEP_REPL_PLUGIN_BUILD
#define ZEP_REPL_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#define ZEP_REPL_PLUGIN_EXPORT
#endif
#endif

#ifdef __cplusplus
}
#endif
