#include "zep/commands_repl.h"
#include "zep/editor.h"

namespace Zep
{

// Lua command wrapper
ZepExCommand_Lua::ZepExCommand_Lua(ZepEditor& editor, IZepReplProvider* pProvider)
    : ZepReplExCommand(editor, pProvider)
{
}

// Duktape command wrapper
ZepExCommand_Duktape::ZepExCommand_Duktape(ZepEditor& editor, IZepReplProvider* pProvider)
    : ZepReplExCommand(editor, pProvider)
{
}

// QuickJS command wrapper
ZepExCommand_QuickJS::ZepExCommand_QuickJS(ZepEditor& editor, IZepReplProvider* pProvider)
    : ZepReplExCommand(editor, pProvider)
{
}

// Registration helpers
void RegisterLuaReplCommand(ZepEditor& editor, IZepReplProvider* pProvider)
{
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Lua>(editor, pProvider));
}

void RegisterDuktapeReplCommand(ZepEditor& editor, IZepReplProvider* pProvider)
{
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Duktape>(editor, pProvider));
}

void RegisterQuickJSReplCommand(ZepEditor& editor, IZepReplProvider* pProvider)
{
    editor.RegisterExCommand(std::make_shared<ZepExCommand_QuickJS>(editor, pProvider));
}

} // namespace Zep
