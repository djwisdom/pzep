#pragma once

#include "zep/editor.h"

namespace Zep
{

class ZepExCommand_Terminal : public ZepExCommand
{
public:
    ZepExCommand_Terminal(ZepEditor& editor);
    const char* ExCommandName() const override;
    void Run(const std::vector<std::string>& args) override;
};

class ZepExCommand_Shell : public ZepExCommand
{
public:
    ZepExCommand_Shell(ZepEditor& editor);
    const char* ExCommandName() const override;
    void Run(const std::vector<std::string>& args) override;

private:
    std::string ExecuteCommand(const std::string& cmd);
};

// Register terminal-related ex commands (:terminal, :!)
void RegisterTerminalCommands(ZepEditor& editor);

} // namespace Zep
