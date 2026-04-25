#pragma once

#include <cstdint>
#include <string>

namespace Zep
{

namespace Security
{

// Maximum config file size (1 MB)
constexpr size_t MAX_CONFIG_FILE_SIZE = 1024 * 1024;

// Maximum nesting depth for TOML tables
constexpr int MAX_TOML_DEPTH = 32;

// Maximum string length in config values
constexpr size_t MAX_CONFIG_STRING_LENGTH = 4096;

// Lua REPL limits
constexpr int LUA_MAX_INSTRUCTIONS = 100000; // 100K instructions per execution
constexpr size_t LUA_MAX_MEMORY = 16 * 1024 * 1024; // 16 MB max memory per Lua state
constexpr int LUA_MAX_OUTPUT_SIZE = 1024 * 1024; // 1 MB max output capture
constexpr int LUA_EXECUTION_TIMEOUT_MS = 2000; // 2 second timeout

// Plugin security
constexpr size_t MAX_PLUGIN_FILE_SIZE = 10 * 1024 * 1024; // 10 MB max plugin size
constexpr int MAX_PLUGINS_LOADED = 16; // Maximum concurrent plugins
constexpr bool PLUGIN_LOADING_ENABLED_DEFAULT = true; // Enable by default (configurable)

// General REPL limits
constexpr int MAX_REPL_HISTORY = 1000; // Max history entries
constexpr int MAX_REPL_INPUT_SIZE = 64 * 1024; // 64 KB max input

// Audit logging
enum class SecurityEvent
{
    ConfigParse,
    LuaExecution,
    PluginLoad,
    PluginExecute,
    ResourceLimitExceeded,
    SandboxViolation
};

std::string SecurityEventToString(SecurityEvent event);

} // namespace Security

} // namespace Zep
