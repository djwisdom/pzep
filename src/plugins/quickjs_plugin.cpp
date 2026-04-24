#include "zep/commands_repl.h"
#include "zep/mode_repl.h"
#include "zep/repl_plugin.h"

// Forward declaration of the QuickJS provider registration function
// Defined in mode_quickjs_repl.cpp
namespace Zep
{
void RegisterQuickJSEvalReplProvider(ZepEditor& editor);
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
        "quickjs",
        "QuickJS JavaScript REPL",
        "1.0",
        "QuickJS ECMAScript engine REPL"
    };
    return &info;
}

extern "C" ZEP_REPL_PLUGIN_EXPORT int InitializePlugin(ZepEditorHandle editor)
{
    ZepEditor* pEditor = reinterpret_cast<ZepEditor*>(editor);
    if (!pEditor)
        return -1;
    Zep::RegisterQuickJSEvalReplProvider(*pEditor);
    return 0;
}

extern "C" ZEP_REPL_PLUGIN_EXPORT void ShutdownPlugin(ZepEditorHandle editor)
{
    ZEP_UNUSED(editor);
    // Cleanup if needed
}
