#include "zep/menu_layer.h"
#include "zep/commands.h"
#include <cstring>

namespace Zep
{

MenuLayer::MenuLayer(ZepEditor& editor)
    : m_editor(editor)
{
    BuildDefaultMenu();
}

MenuLayer::~MenuLayer()
{
}

void MenuLayer::BuildDefaultMenu()
{
    // File menu
    m_entries.push_back({ "File", { { "New", ":enew", "Ctrl+N" }, { "Open...", ":e", "Ctrl+O" }, { "Save", ":w", "Ctrl+S" }, { "Save As...", ":w ", "Ctrl+Shift+S" }, { "----" }, { "Quit", ":q", "Ctrl+Q" } } });

    // Edit menu
    m_entries.push_back({ "Edit", { { "Undo", "u", "Ctrl+Z" }, { "Redo", "Ctrl+R", "Ctrl+Y" }, { "----" }, { "Cut", "\"+x", "Ctrl+X" }, { "Copy", "\"+y", "Ctrl+C" }, { "Paste", "\"+p", "Ctrl+V" }, { "----" }, { "Find...", "/", "Ctrl+F" }, { "Replace...", ":s/", "Ctrl+H" } } });

    // Search menu
    m_entries.push_back({ "Search", { { "Find in Files...", ":vimgrep", "Ctrl+Shift+F" }, { "Go to Line...", ":goto", "Ctrl+G" } } });

    // Options menu
    m_entries.push_back({ "Options", { { "Settings...", "", "", nullptr, true }, { "Toggle Fullscreen", "", "F11" }, { "----" }, { "Auto-indent", ":set ai", "" }, { "Wrap lines", ":set wrap", "" } } });

    // Help menu
    m_entries.push_back({ "Help", { { "Interactive Tutorial", ":tutor", "F1" }, { "About pZep", "", "" } } });
}

void MenuLayer::Draw()
{
    if (!m_visible)
        return;

    DrawMenuBar();

    if (m_showOptionsDialog)
    {
        DrawOptionsMenu(); // will draw dialog
    }
}

void MenuLayer::DrawMenuBar()
{
    // Raygui menu bar
    int x = 0;
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        if (GuiButton({ (float)x, 0, 80, 20 }, m_entries[i].name.c_str()))
        {
            m_activeMenu = (m_activeMenu == (int)i) ? -1 : (int)i;
        }
        else if (m_activeMenu == (int)i)
        {
            // draw dropdown
            float dy = 20;
            for (size_t j = 0; j < m_entries[i].actions.size(); ++j)
            {
                const auto& act = m_entries[i].actions[j];
                if (act.label == "----")
                {
                    GuiLine({ (float)x, 20 + dy, 150, 1 });
                    dy += 4;
                    continue;
                }
                if (GuiButton({ (float)x, 20 + dy, 150, 20 }, act.label.c_str()))
                {
                    if (!act.command.empty())
                    {
                        m_editor.ExecuteCommand(act.command);
                    }
                    if (act.callback)
                        act.callback();
                    m_activeMenu = -1;
                }
                dy += 22;
            }
        }
        x += 80;
    }

    // Special hook for Options > Settings...
    if (m_activeMenu == 3)
    { // Options menu index
        for (size_t j = 0; j < m_entries[3].actions.size(); ++j)
        {
            if (m_entries[3].actions[j].label == "Settings...")
            {
                // We'll handle it via m_showOptionsDialog
            }
        }
    }
}

void MenuLayer::DrawFileMenu()
{
    // Handled in DrawMenuBar
}

void MenuLayer::DrawEditMenu()
{
    // Handled in DrawMenuBar
}

void MenuLayer::DrawSearchMenu()
{
    // Handled in DrawMenuBar
}

void MenuLayer::DrawOptionsMenu()
{
    // Draw settings dialog
    Rectangle bounds = { 100, 100, 500, 400 };
    if (GuiWindowBox(bounds, "Settings (.pzeprc)"))
    {
        m_showOptionsDialog = false;
    }

    // Display editable settings here (we'll expand this next)
    GuiLabel({ 120, 130, 200, 20 }, "Configurable options:");
}

void MenuLayer::DrawHelpMenu()
{
    // Handled in DrawMenuBar
}

} // namespace Zep