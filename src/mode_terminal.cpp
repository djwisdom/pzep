#include "zep/mode_terminal.h"
#include "zep/buffer.h"
#include "zep/display.h"
#include "zep/editor.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

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

    // Sync editor cursor with terminal cursor position
    auto& buffer = window.GetBuffer();
    auto& state = m_terminal.GetState();
    auto lineEnds = buffer.GetLineEnds();
    long row = state.cursorRow;
    if (row < 0)
        row = 0;
    if (!lineEnds.empty())
    {
        if (row >= long(lineEnds.size()))
            row = long(lineEnds.size()) - 1;
        ByteIndex lineStart = (row == 0) ? 0 : lineEnds[row - 1];
        ByteIndex lineEnd = lineEnds[row];
        long col = state.cursorCol;
        long maxCol = long(lineEnd - lineStart) - 1; // last character position
        if (col > maxCol)
            col = maxCol;
        if (col < 0)
            col = 0;
        GlyphIterator itr(&buffer, lineStart + col);
        window.SetBufferCursor(itr);
    }

    // If the terminal process has exited (shell closed), close this window
    if (!m_terminal.IsRunning())
    {
        auto pTab = GetEditor().GetActiveTabWindow();
        if (pTab && pTab->GetActiveWindow() == &window)
        {
            pTab->CloseActiveWindow();
        }
    }
}

} // namespace Zep
