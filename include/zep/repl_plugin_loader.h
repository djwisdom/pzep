#pragma once

#include <string>
#include <vector>

namespace Zep
{

class ZepEditor;

// Initialize the REPL plugin loader and scan the given directory for plugins.
// Call this once during editor startup, after creating the ZepEditor instance.
// Returns true if the loader initialized successfully (even if no plugins found).
void InitializeReplPluginLoader(ZepEditor* pEditor, const std::string& pluginsDir = "plugins");

// Get list of successfully loaded REPL plugin names (for debugging/info)
std::vector<std::string> GetLoadedReplPluginNames();

} // namespace Zep
