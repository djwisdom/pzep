#include "zep/editor.h"
#include "zep/repl_plugin.h"

namespace Zep
{
void RegisterLuaReplProvider(ZepEditor& editor);
}

extern "C" ZEP_REPL_PLUGIN_EXPORT const ZepReplPluginAPI* ZepReplPluginEntry()
{
    static const ZepReplPluginAPI api = {
        GetPluginInfo,
        InitializePlugin,
        ShutdownPlugin
    };
    return &api;
}

extern "C" ZEP_REPL_PLUGIN_EXPORT const ZepReplPluginInfo* GetPluginInfo()
{
    static const ZepReplPluginInfo info = {
        "lua",
        "Lua REPL",
        "1.0",
        "Lua 5.4 scripting language REPL"
    };
    return &info;
}

extern "C" ZEP_REPL_PLUGIN_EXPORT int InitializePlugin(ZepEditorHandle editor)
{
    ZepEditor* pEditor = reinterpret_cast<ZepEditor*>(editor);
    if (!pEditor)
        return -1;
    Zep::RegisterLuaReplProvider(*pEditor);
    return 0;
}

extern "C" ZEP_REPL_PLUGIN_EXPORT void ShutdownPlugin(ZepEditorHandle editor)
{
    (void)editor; // unused
}
