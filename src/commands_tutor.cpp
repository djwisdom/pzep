#include "zep/commands_tutor.h"
#include "zep/buffer.h"
#include "zep/filesystem.h"
#include "zep/mode_tutorial.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#include <algorithm>
#include <sstream>

namespace Zep
{

ZepExCommand_Tutor::ZepExCommand_Tutor(ZepEditor& editor)
    : ZepExCommand(editor)
{
}

void ZepExCommand_Tutor::Run(const std::vector<std::string>& args)
{
    // Parse lesson number, default to 1
    int lesson = 1;
    if (args.size() > 1)
    {
        try
        {
            lesson = std::stoi(args[1]);
            if (lesson < 1)
                lesson = 1;
        }
        catch (...)
        {
            lesson = 1;
        }
    }

    auto pEditor = &GetEditor();
    auto pTab = pEditor->GetActiveTabWindow();
    if (!pTab)
    {
        pEditor->SetCommandText("No active tab window");
        return;
    }

    // Create a buffer for the tutorial
    auto pBuffer = pEditor->GetEmptyBuffer("*tutorial*");
    pBuffer->SetBufferType(BufferType::Tutorial);
    pBuffer->SetFileFlags(FileFlags::ReadOnly | FileFlags::Locked);

    // Create tutorial mode with starting lesson
    auto pMode = std::make_shared<ZepMode_Tutorial>(*pEditor, lesson);
    pMode->Init();
    pBuffer->SetMode(pMode);

    // Add the buffer to a new window (or reuse default startup window)
    auto pWindow = pTab->AddWindow(pBuffer, nullptr, RegionLayoutType::VBox);
    if (!pWindow)
    {
        pEditor->SetCommandText("Failed to open tutorial window");
        return;
    }

    // Set active window to the tutorial
    pTab->SetActiveWindow(pWindow);
}

} // namespace Zep
