#include "zep/mode_terminal.h"
#include "zep/display.h"
#include "zep/editor.h"
#include "zep/window.h"

namespace Zep
{

ZepMode_Terminal::ZepMode_Terminal(ZepEditor& editor, ZepTerminal& terminal)
    : ZepMode(editor)
    , m_terminal(terminal)
{
    // Terminal mode: all input goes directly to terminal
    // We can set up key mappings for special keys
}

ZepMode_Terminal::~ZepMode_Terminal()
{
    // Ensure terminal is stopped when mode is destroyed
    if (m_terminal.IsRunning())
    {
        m_terminal.Stop();
    }
}

void ZepMode_Terminal::Begin(ZepWindow* pWindow)
{
    ZepMode::Begin(pWindow);
    // Switch to Insert mode for terminal input
    SwitchMode(EditorMode::Insert);
}

void ZepMode_Terminal::Notify(std::shared_ptr<ZepMessage> message)
{
    ZepMode::Notify(message);
}

void ZepMode_Terminal::AddCommandText(std::string strText)
{
    // Send typed characters directly to terminal
    m_terminal.SendInput(strText);
}

// PreDisplay is called before the window is rendered; update terminal buffer
void ZepMode_Terminal::PreDisplay(ZepWindow& window)
{
    ZepMode::PreDisplay(window);

    // Update terminal buffer from terminal state
    m_terminal.UpdateBuffer();
}

} // namespace Zep
