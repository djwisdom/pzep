#pragma once

#include "zep/editor.h"
#include <functional>
#include <string>
#include <vector>

// raygui single-header (local in third_party)
#define RAYGUI_IMPLEMENTATION
#include "../../third_party/raygui/raygui.h"

namespace Zep
{

struct MenuAction
{
    std::string label; // e.g. "Open"
    std::string command; // e.g. ":e"
    std::string shortcut; // e.g. "Ctrl+O"
    std::function<void()> callback; // optional override
    bool enabled = true;
};

struct MenuEntry
{
    std::string name; // "File", "Edit", etc.
    std::vector<MenuAction> actions;
};

class MenuLayer
{
public:
    MenuLayer(ZepEditor& editor);
    ~MenuLayer();

    void Draw();
    void ToggleVisible()
    {
        m_visible = !m_visible;
    }
    bool IsVisible() const
    {
        return m_visible;
    }

    void BuildDefaultMenu();

private:
    void DrawMenuBar();
    void DrawFileMenu();
    void DrawEditMenu();
    void DrawSearchMenu();
    void DrawOptionsMenu();
    void DrawHelpMenu();

    ZepEditor& m_editor;
    bool m_visible = true;
    bool m_showOptionsDialog = false;

    std::vector<MenuEntry> m_entries;
    int m_activeMenu = -1;
};

} // namespace Zep
