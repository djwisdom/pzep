#include "zep/editor.h"
#include "zep/mode_repl.h"
#include "zep/repl_plugin.h"
#include "zep/security.h"

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

// ============================================================
// Secure Plugin Loader
// ============================================================

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
        for (auto& plugin : m_plugins)
        {
            if (plugin.initialized && plugin.api && plugin.api->shutdown)
            {
                plugin.api->shutdown(nullptr);
            }
            if (plugin.handle)
            {
                PLUGIN_UNLOAD(plugin.handle);
            }
        }
        m_plugins.clear();
    }

    bool LoadPlugins(const std::string& pluginsDir, ZepEditor* pEditor)
    {
        std::cout << "[ReplPluginLoader] Scanning for plugins in: " << pluginsDir << std::endl;

        if (!fs::exists(pluginsDir))
        {
            std::cout << "[ReplPluginLoader] Plugin directory not found, skipping: " << pluginsDir << std::endl;
            return true;
        }

        for (const auto& entry : fs::directory_iterator(pluginsDir))
        {
            if (!entry.is_regular_file())
                continue;

            std::string ext = entry.path().extension().string();
            if (ext != PLUGIN_EXT)
                continue;

            // SECURITY: Check file size before loading
            try
            {
                auto fileSize = fs::file_size(entry.path());
                if (fileSize > Security::MAX_PLUGIN_FILE_SIZE)
                {
                    std::cerr << "[ReplPluginLoader] Plugin too large (" << fileSize << " bytes), skipping: "
                              << entry.path().string() << std::endl;
                    continue;
                }
            }
            catch (...)
            {
                continue;
            }

            std::string path = entry.path().string();
            std::cout << "[ReplPluginLoader] Found plugin: " << path << std::endl;

            if (!LoadPlugin(path, pEditor))
            {
                std::cerr << "[ReplPluginLoader] Failed to load plugin: " << path << std::endl;
            }
        }

        return true;
    }

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
    std::vector<LoadedPlugin> m_plugins;

    bool LoadPlugin(const std::string& path, ZepEditor* pEditor)
    {
        PLUGIN_HANDLE handle = PLUGIN_LOAD(path.c_str());
        if (!handle)
        {
            std::cerr << "[ReplPluginLoader] LoadLibrary failed for: " << path << std::endl;
            return false;
        }

        auto entryFunc = (ZepReplPluginEntryFunc)PLUGIN_GET_PROC(handle, "ZepReplPluginEntry");
        if (!entryFunc)
        {
            std::cerr << "[ReplPluginLoader] Entry point not found in: " << path << std::endl;
            PLUGIN_UNLOAD(handle);
            return false;
        }

        const ZepReplPluginAPI* api = entryFunc();
        if (!api || !api->init || !api->get_info)
        {
            std::cerr << "[ReplPluginLoader] Invalid plugin API: " << path << std::endl;
            PLUGIN_UNLOAD(handle);
            return false;
        }

        const ZepReplPluginInfo* info = api->get_info();
        if (!info || !info->name)
        {
            std::cerr << "[ReplPluginLoader] Plugin missing info: " << path << std::endl;
            PLUGIN_UNLOAD(handle);
            return false;
        }

        if (m_plugins.size() >= Security::MAX_PLUGINS_LOADED)
        {
            std::cerr << "[ReplPluginLoader] Maximum number of plugins reached, skipping: " << info->name << std::endl;
            PLUGIN_UNLOAD(handle);
            return false;
        }

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

        ZLOG(INFO, "Plugin loaded: " << plugin.info.name << " from " << path);

        return true;
    }
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
