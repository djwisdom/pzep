#include "zep/editor.h"
#include "zep/mode_repl.h"
#include "zep/repl_plugin.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#define PLUGIN_HANDLE HMODULE
#define PLUGIN_LOAD(path) LoadLibraryA(path)
#define PLUGIN_GET_PROC(handle, name) GetProcAddress(handle, name)
#define PLUGIN_UNLOAD(handle) FreeLibrary(handle)
#define PLUGIN_EXT ".dll"
#else
#include <dlfcn.h>
#define PLUGIN_HANDLE void*
#define PLUGIN_LOAD(path) dlopen(path, RTLD_NOW | RTLD_LOCAL)
#define PLUGIN_GET_PROC(handle, name) dlsym(handle, name)
#define PLUGIN_UNLOAD(handle) dlclose(handle)
#define PLUGIN_EXT ".so"
#endif

namespace fs = std::filesystem;

namespace Zep
{

// Structure to hold loaded plugin state
struct LoadedPlugin
{
    std::string path;
    PLUGIN_HANDLE handle = nullptr;
    const ZepReplPluginAPI* api = nullptr;
    ZepReplPluginInfo info;
    bool initialized = false;
};

class ReplPluginLoader
{
public:
    ~ReplPluginLoader()
    {
        // Unload all plugins
        for (auto& plugin : m_plugins)
        {
            if (plugin.initialized && plugin.api && plugin.api->shutdown)
            {
                plugin.api->shutdown(nullptr); // Editor already gone
            }
            if (plugin.handle)
            {
                PLUGIN_UNLOAD(plugin.handle);
            }
        }
        m_plugins.clear();
    }

    // Scan plugins directory and load all REPL plugins
    bool LoadPlugins(const std::string& pluginsDir, ZepEditor* pEditor)
    {
        std::cout << "[ReplPluginLoader] Scanning for plugins in: " << pluginsDir << std::endl;

        if (!fs::exists(pluginsDir))
        {
            std::cout << "[ReplPluginLoader] Plugin directory not found, skipping: " << pluginsDir << std::endl;
            return true; // Not an error, just no plugins
        }

        for (const auto& entry : fs::directory_iterator(pluginsDir))
        {
            if (!entry.is_regular_file())
                continue;

            std::string ext = entry.path().extension().string();
#ifdef _WIN32
            if (ext != PLUGIN_EXT)
#else
            if (ext != PLUGIN_EXT)
#endif
                continue;

            std::string path = entry.path().string();
            std::cout << "[ReplPluginLoader] Found plugin: " << path << std::endl;

            if (!LoadPlugin(path, pEditor))
            {
                std::cerr << "[ReplPluginLoader] Failed to load plugin: " << path << std::endl;
            }
        }

        return true;
    }

    // Get list of loaded plugin names
    std::vector<std::string> GetLoadedPluginNames() const
    {
        std::vector<std::string> names;
        for (const auto& plugin : m_plugins)
        {
            names.push_back(plugin.info.name);
        }
        return names;
    }

private:
    bool LoadPlugin(const std::string& path, ZepEditor* pEditor)
    {
        PLUGIN_HANDLE handle = PLUGIN_LOAD(path.c_str());
        if (!handle)
        {
            std::cerr << "[ReplPluginLoader] LoadLibrary failed for: " << path << std::endl;
            return false;
        }

        // Get the standard entry point
        auto entryFunc = (ZepReplPluginEntryFunc)PLUGIN_GET_PROC(handle, "ZepReplPluginEntry");
        if (!entryFunc)
        {
            std::cerr << "[ReplPluginLoader] Entry point not found in: " << path << std::endl;
            PLUGIN_UNLOAD(handle);
            return false;
        }

        // Get the plugin API
        const ZepReplPluginAPI* api = entryFunc();
        if (!api || !api->init || !api->get_info)
        {
            std::cerr << "[ReplPluginLoader] Invalid plugin API: " << path << std::endl;
            PLUGIN_UNLOAD(handle);
            return false;
        }

        // Query plugin info
        const ZepReplPluginInfo* info = api->get_info();
        if (!info || !info->name)
        {
            std::cerr << "[ReplPluginLoader] Plugin missing info: " << path << std::endl;
            PLUGIN_UNLOAD(handle);
            return false;
        }

        // Initialize plugin (this will register its commands with the editor)
        // Cast ZepEditor* to ZepEditorHandle (void*) - safe because plugin side will cast back
        int initResult = api->init(reinterpret_cast<ZepEditorHandle>(pEditor));
        if (initResult != 0)
        {
            std::cerr << "[ReplPluginLoader] Plugin init failed: " << info->name << " (code " << initResult << ")" << std::endl;
            PLUGIN_UNLOAD(handle);
            return false;
        }

        LoadedPlugin plugin;
        plugin.path = path;
        plugin.handle = handle;
        plugin.api = api;
        plugin.info.name = info->name;
        plugin.info.display_name = info->display_name ? info->display_name : info->name;
        plugin.info.version = info->version ? info->version : "unknown";
        plugin.info.description = info->description ? info->description : "";
        plugin.initialized = true;

        m_plugins.push_back(std::move(plugin));

        std::cout << "[ReplPluginLoader] Loaded REPL plugin: " << plugin.info.name
                  << " (" << plugin.info.display_name << ") v" << plugin.info.version << std::endl;

        return true;
    }

    std::vector<LoadedPlugin> m_plugins;
};

// Singleton access
static std::unique_ptr<ReplPluginLoader> g_pluginLoader;

void InitializeReplPluginLoader(ZepEditor* pEditor, const std::string& pluginsDir)
{
    g_pluginLoader = std::make_unique<ReplPluginLoader>();
    g_pluginLoader->LoadPlugins(pluginsDir, pEditor);
}

std::vector<std::string> GetLoadedReplPluginNames()
{
    if (g_pluginLoader)
        return g_pluginLoader->GetLoadedPluginNames();
    return {};
}

} // namespace Zep
