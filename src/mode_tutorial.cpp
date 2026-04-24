#include "zep/mode_tutorial.h"
#include "zep/buffer.h"
#include "zep/editor.h"
#include "zep/keymap.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#include <sstream>
#include <string>
#include <vector>

namespace Zep
{

static const std::vector<std::string> g_tutorLessons = {
    R"(Welcome to pzep tutorial - Lesson 1

This is a simple, modal editor inspired by vim.

Navigation:
- Use arrow keys, or h/j/k/l to move cursor
- PageUp/PageDown scroll by pages
- Press 'n' to go to the next lesson
- Press 'p' to go to the previous lesson
- Press 'q' to close the tutorial

Note: This buffer is read-only; you cannot edit the text.)",

    R"(Lesson 2: Basic concepts

pZep uses modes:
- Normal mode: for navigating and issuing commands
- Insert mode: for inserting text (disabled in this tutorial)

You can switch modes with keys like:
  i - insert mode (not allowed here)
  : - ex command line

Ex commands include:
  :w - save file
  :q - quit window
  :help - show help (if available)

Try moving around with hjkl or arrow keys.)",

    R"(Lesson 3: Getting help

- Type :help to view documentation (if configured)
- Type :q to quit this tutorial window
- Press 'n' or space to see next lesson
- Press 'p' or backspace to see previous lesson

Congratulations! You've completed the tutorial.)",

    R"(Lesson 4: Splitting windows

pZep can split the editor window into multiple panes.

Commands:
  :split      - split horizontally
  :vsplit     - split vertically
  Ctrl+w h/j/k/l - move between windows (vim-style)
  Ctrl+w arrow - move between windows

Demo keys in this tutorial:
  s - horizontal split
  v - vertical split
  q - close current window (if more than one)

Try pressing 's' now to see a horizontal split, then use Ctrl+w+hjkl to move focus.)",

    R"(Lesson 5: Tabs

pZep supports tabbed editing, similar to vim's tab pages.

Commands:
  :tabnew     - open a new tab
  :tabclose   - close the current tab
  gt          - go to next tab
  gT          - go to previous tab
  :tabn       - next tab
  :tabp       - previous tab

Demo keys:
  t - new tab
  x - close current tab
  Once you have multiple tabs, use gt/gT to switch

Try pressing 't' to open a new tab.)",

    R"(Lesson 6: REPL (Read-Eval-Print Loop)

pZep includes embedded REPLs for scripting languages.

Available REPLs depend on build configuration:
  :lua      - Lua REPL
  :duktape  - Duktape JavaScript REPL
  :quickjs  - QuickJS JavaScript REPL
  :ZRepl    - generic REPL (whichever is registered last)

Demo keys (press in Normal mode):
  1 - open Lua REPL (if enabled)
  2 - open Duktape REPL (if enabled)
  3 - open QuickJS REPL (if enabled)
  r - open default REPL

Once REPL opens, type an expression and press Enter:
  Lua:      1+1      => 2
  JavaScript: 2*3    => 6

This validates the REPL is working.)"
};

ZepMode_Tutorial::ZepMode_Tutorial(ZepEditor& editor, int startLesson)
    : ZepMode(editor)
    , m_currentLesson(startLesson)
{
}

void ZepMode_Tutorial::Init()
{
    // Set up key mappings for Normal mode operations

    // Allow entering ex mode with ':'
    keymap_add({ &m_normalMap }, { ":" }, id_ExMode);

    // Movement keys (reuse standard movement commands)
    keymap_add({ &m_normalMap }, { "h" }, id_MotionStandardLeft);
    keymap_add({ &m_normalMap }, { "j" }, id_MotionStandardDown);
    keymap_add({ &m_normalMap }, { "k" }, id_MotionStandardUp);
    keymap_add({ &m_normalMap }, { "l" }, id_MotionStandardRight);

    keymap_add({ &m_normalMap }, { "<Left>" }, id_MotionStandardLeft);
    keymap_add({ &m_normalMap }, { "<Right>" }, id_MotionStandardRight);
    keymap_add({ &m_normalMap }, { "<Up>" }, id_MotionStandardUp);
    keymap_add({ &m_normalMap }, { "<Down>" }, id_MotionStandardDown);

    keymap_add({ &m_normalMap }, { "<PageUp>" }, id_MotionPageBackward);
    keymap_add({ &m_normalMap }, { "<PageDown>" }, id_MotionPageForward);

    // Home and End
    keymap_add({ &m_normalMap }, { "<Home>" }, id_MotionLineHomeToggle);
    keymap_add({ &m_normalMap }, { "<End>" }, id_MotionLineBeyondEnd);
}

void ZepMode_Tutorial::Begin(ZepWindow* pWindow)
{
    ZepMode::Begin(pWindow);
    LoadLesson(m_currentLesson);
}

void ZepMode_Tutorial::AddKeyPress(uint32_t key, uint32_t modifierKeys)
{
    (void)modifierKeys; // unused

    // Demo feature keys - available at any time in tutorial mode
    switch (key)
    {
    case 's': // Horizontal split demo
        DemoSplitHorizontal();
        return;
    case 'v': // Vertical split demo
        DemoSplitVertical();
        return;
    case 't': // New tab demo
        DemoTabNew();
        return;
    case 'x': // Close current tab/window
        DemoTabClose();
        return;
    case 'r': // Default REPL demo (use first available)
        DemoREPL();
        return;
    case '1': // Lua REPL
        DemoREPLLua();
        return;
    case '2': // Duktape REPL
        DemoREPLDuktape();
        return;
    case '3': // QuickJS REPL
        DemoREPLQuickJS();
        return;
    }

    // Tutor navigation keys
    if (key == 'n' || key == ' ') // next lesson
    {
        NextLesson();
        return;
    }
    else if (key == 'p' || key == ExtKeys::BACKSPACE) // previous lesson
    {
        PrevLesson();
        return;
    }
    else if (key == 'q' || key == 'Q') // quit tutorial (close window)
    {
        auto pWindow = GetCurrentWindow();
        if (pWindow)
        {
            pWindow->GetTabWindow().CloseActiveWindow();
        }
        return;
    }

    // For all other keys, let base class handle (keymap lookup)
    ZepMode::AddKeyPress(key, modifierKeys);
}

void ZepMode_Tutorial::LoadLesson(int num)
{
    auto pWindow = GetCurrentWindow();
    if (!pWindow)
        return;

    auto& buffer = pWindow->GetBuffer();
    buffer.SetText(g_tutorLessons[num - 1]);

    // Update command line to show lesson number
    std::ostringstream ss;
    ss << "Tutorial Lesson " << num;
    GetEditor().SetCommandText(ss.str());
}

void ZepMode_Tutorial::DemoSplitHorizontal()
{
    auto pTab = GetEditor().GetActiveTabWindow();
    if (!pTab)
        return;

    // Execute :sp via the ex command system
    auto pEx = GetEditor().FindExCommand("sp");
    if (pEx)
    {
        pEx->Run({});
        GetEditor().SetCommandText("Demo: Horizontal split (:sp or :split). Press 'q' to close windows.");
    }
}

void ZepMode_Tutorial::DemoSplitVertical()
{
    auto pTab = GetEditor().GetActiveTabWindow();
    if (!pTab)
        return;

    auto pEx = GetEditor().FindExCommand("vsplit");
    if (pEx)
    {
        pEx->Run({});
        GetEditor().SetCommandText("Demo: Vertical split (:vsplit). Press 'q' to close windows.");
    }
}

void ZepMode_Tutorial::DemoTabNew()
{
    auto pTab = GetEditor().GetActiveTabWindow();
    if (!pTab)
        return;

    auto pEx = GetEditor().FindExCommand("tabnew");
    if (pEx)
    {
        pEx->Run({});
        GetEditor().SetCommandText("Demo: New tab (:tabnew). Use gt/gT to navigate tabs.");
    }
}

void ZepMode_Tutorial::DemoREPL()
{
    // Try to use a default; prefer Lua if available, else QuickJS, else Duktape
    auto pEx = GetEditor().FindExCommand("lua");
    if (!pEx)
        pEx = GetEditor().FindExCommand("quickjs");
    if (!pEx)
        pEx = GetEditor().FindExCommand("duktape");
    if (!pEx)
        pEx = GetEditor().FindExCommand("ZRepl");
    if (pEx)
    {
        pEx->Run({});
        GetEditor().SetCommandText("Demo: REPL. Try: <1> Lua, <2> Duktape, <3> QuickJS if available. Type 1+1 then Enter.");
    }
    else
    {
        GetEditor().SetCommandText("No REPL available. Enable a REPL provider.");
    }
}

void ZepMode_Tutorial::DemoREPLLua()
{
    auto pEx = GetEditor().FindExCommand("lua");
    if (pEx)
    {
        pEx->Run({});
        GetEditor().SetCommandText("Lua REPL. Test with: 1+1 (should print 2). Press Enter to evaluate.");
    }
    else
    {
        GetEditor().SetCommandText("Lua REPL not available (enable in CMake)");
    }
}

void ZepMode_Tutorial::DemoREPLDuktape()
{
    auto pEx = GetEditor().FindExCommand("duktape");
    if (pEx)
    {
        pEx->Run({});
        GetEditor().SetCommandText("Duktape REPL. Test with: 1+1 (should print 2). Press Enter to evaluate.");
    }
    else
    {
        GetEditor().SetCommandText("Duktape REPL not available (enable in CMake)");
    }
}

void ZepMode_Tutorial::DemoREPLQuickJS()
{
    auto pEx = GetEditor().FindExCommand("quickjs");
    if (pEx)
    {
        pEx->Run({});
        GetEditor().SetCommandText("QuickJS REPL. Test with: 1+1 (should print 2). Press Enter to evaluate.");
    }
    else
    {
        GetEditor().SetCommandText("QuickJS REPL not available (enable in CMake)");
    }
}

void ZepMode_Tutorial::DemoTabClose()
{
    auto pEx = GetEditor().FindExCommand("tabclose");
    if (pEx)
    {
        pEx->Run({});
    }
    else
    {
        GetEditor().SetCommandText("tabclose command not available");
    }
}

void ZepMode_Tutorial::NextLesson()
{
    if (HasLesson(m_currentLesson + 1))
    {
        m_currentLesson++;
        LoadLesson(m_currentLesson);
    }
}

void ZepMode_Tutorial::PrevLesson()
{
    if (HasLesson(m_currentLesson - 1))
    {
        m_currentLesson--;
        LoadLesson(m_currentLesson);
    }
}

bool ZepMode_Tutorial::HasLesson(int num) const
{
    return num >= 1 && num <= static_cast<int>(g_tutorLessons.size());
}

} // namespace Zep
