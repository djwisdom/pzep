#pragma once

#include "zep/mode_repl.h"

namespace Zep
{

// Individual REPL command classes with unique names
class ZepExCommand_Lua : public ZepReplExCommand
{
public:
    ZepExCommand_Lua(ZepEditor& editor, IZepReplProvider* pProvider);
    const char* ExCommandName() const override
    {
        return "lua";
    }
};

class ZepExCommand_Duktape : public ZepReplExCommand
{
public:
    ZepExCommand_Duktape(ZepEditor& editor, IZepReplProvider* pProvider);
    const char* ExCommandName() const override
    {
        return "duktape";
    }
};

class ZepExCommand_QuickJS : public ZepReplExCommand
{
public:
    ZepExCommand_QuickJS(ZepEditor& editor, IZepReplProvider* pProvider);
    const char* ExCommandName() const override
    {
        return "quickjs";
    }
};

// Helper functions to register individual REPL commands with unique names
void RegisterLuaReplCommand(ZepEditor& editor, IZepReplProvider* pProvider);
void RegisterDuktapeReplCommand(ZepEditor& editor, IZepReplProvider* pProvider);
void RegisterQuickJSReplCommand(ZepEditor& editor, IZepReplProvider* pProvider);

} // namespace Zep
