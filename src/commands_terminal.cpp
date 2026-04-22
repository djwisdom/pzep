#include "zep/commands_terminal.h"
#include "zep/commands.h"
#include "zep/editor.h"
#include "zep/mode_terminal.h"
#include "zep/splits.h"
#include "zep/tab_window.h"
#include "zep/terminal.h"
#include "zep/window.h"

#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"

#include <cstdio>
#include <sstream>

// Undefine possible macro conflicts from Windows headers
#undef ERROR

namespace Zep
{

// ============ ZepExCommand_Terminal (:terminal) ============

ZepExCommand_Terminal::ZepExCommand_Terminal(ZepEditor& editor)
    : ZepExCommand(editor)
{
}

const char* ZepExCommand_Terminal::ExCommandName() const
{
    return "terminal";
}

void ZepExCommand_Terminal::Run(const std::vector<std::string>& args)
{
    // Parse optional shell argument
    std::string shell;
    if (args.size() > 1)
    {
        // args[0] is "terminal", args[1...] is shell and args
        // Join remaining args
        for (size_t i = 1; i < args.size(); i++)
        {
            if (i > 1)
                shell += " ";
            shell += args[i];
        }
    }

    auto pTab = GetEditor().GetActiveTabWindow();
    if (!pTab)
    {
        GetEditor().SetCommandText("No active tab window");
        return;
    }

    // Create a new buffer for the terminal
    auto pBuffer = GetEditor().GetEmptyBuffer("*terminal*");
    pBuffer->SetBufferType(BufferType::Terminal);

    // Create terminal object
    auto pTerminal = std::make_shared<ZepTerminal>(GetEditor(), pBuffer);
    if (!pTerminal->Start(shell))
    {
        GetEditor().SetCommandText("Failed to start terminal");
        return;
    }

    // Create terminal mode and associate with buffer
    auto pMode = std::make_shared<ZepMode_Terminal>(GetEditor(), *pTerminal);
    pBuffer->SetMode(pMode);

    // Set key notifier to send input to terminal
    pBuffer->SetPostKeyNotifier([term = pTerminal](uint32_t key, uint32_t modifier) -> bool {
        term->SendKey(key, modifier);
        return true; // Mark as handled to prevent mode from processing
    });

    // Add the buffer to a new window
    auto pWindow = pTab->AddWindow(pBuffer, nullptr, RegionLayoutType::VBox);
    if (pWindow)
    {
        // Set appropriate window flags maybe?
    }

    // Switch to the terminal window
    pTab->SetActiveWindow(pWindow);

    // Use Insert mode for terminal
    pMode->SwitchMode(EditorMode::Insert);

    ZLOG(INFO, "Terminal opened");
}

// ============ ZepExCommand_Shell (:!cmd) ============

ZepExCommand_Shell::ZepExCommand_Shell(ZepEditor& editor)
    : ZepExCommand(editor)
{
}

const char* ZepExCommand_Shell::ExCommandName() const
{
    return "!";
}

void ZepExCommand_Shell::Run(const std::vector<std::string>& args)
{
    // args[0] includes the colon and command, e.g. ":!make" or just "!" depending on how it's passed
    // Our dispatch now extracts first word after colon as commandKey, and passes full tokens.
    // The first token will be the command including colon? Actually for :!make, commandKey = "!" but args[0] = ":!make"? Let's see.
    // However, calling from mode: string_split(strCommand, " "); For strCommand=":!make", split gives {":!make"} (no spaces).
    // That's just one element containing the whole command with colon. We need to extract the command part.
    // Simpler: ignore args[0]; take all tokens after possible colon? But if there are no spaces, we have only one token.
    // Actually better: we know commandKey = "!". But Run doesn't receive commandKey. We'll just parse ourselves.
    // If args size is 1, that token is like ":!cmd" or ":!" maybe with args concatenated. That's tricky.
    // Better approach: combine all tokens and strip leading colon+!
    // Let's reconstruct full command string from args by joining with space.
    std::string fullCommand;
    for (size_t i = 0; i < args.size(); i++)
    {
        if (i > 0)
            fullCommand += " ";
        fullCommand += args[i];
    }
    // fullCommand now is something like ":!make build" or ":!cmd"
    // Remove leading colon
    if (!fullCommand.empty() && fullCommand[0] == ':')
    {
        fullCommand = fullCommand.substr(1);
    }
    // Now fullCommand starts with "!make build" or "!cmd"
    if (fullCommand.size() < 2 || fullCommand[0] != '!')
    {
        // Not a shell command?
        return;
    }
    // Remove leading "!"
    std::string cmd = fullCommand.substr(1);
    // Trim leading spaces
    while (!cmd.empty() && std::isspace(static_cast<unsigned char>(cmd.front())))
    {
        cmd.erase(0, 1);
    }

    if (cmd.empty())
    {
        GetEditor().SetCommandText("Usage: :!command");
        return;
    }

    GetEditor().SetCommandText("Running: " + cmd);

    std::string output = ExecuteCommand(cmd);

    auto pTab = GetEditor().GetActiveTabWindow();
    if (!pTab)
    {
        return;
    }

    auto pBuffer = GetEditor().GetFileBuffer(":! " + cmd, 0, true);
    pBuffer->SetText(output);
    pBuffer->SetFileFlags(FileFlags::ReadOnly);
    pTab->AddWindow(pBuffer, nullptr, RegionLayoutType::VBox);
}

std::string ZepExCommand_Shell::ExecuteCommand(const std::string& cmd)
{
#ifdef _WIN32
    // Use _popen for Windows
    std::string fullCmd = "cmd /c " + cmd;
    FILE* pipe = _popen(fullCmd.c_str(), "r");
    if (!pipe)
    {
        return "Error: Failed to execute command";
    }

    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }

    _pclose(pipe);
    return result;
#else
    // Use popen for Unix
    std::string fullCmd = cmd + " 2>&1"; // Include stderr
    FILE* pipe = popen(fullCmd.c_str(), "r");
    if (!pipe)
    {
        return "Error: Failed to execute command";
    }

    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }

    pclose(pipe);
    return result;
#endif
}

// ============ Registration ============

void RegisterTerminalCommands(ZepEditor& editor)
{
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Terminal>(editor));
    editor.RegisterExCommand(std::make_shared<ZepExCommand_Shell>(editor));
}

} // namespace Zep
