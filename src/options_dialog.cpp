#include "zep/options_dialog.h"
#include "zep/commands.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace Zep
{

OptionsDialog::OptionsDialog(ZepEditor& editor)
    : m_editor(editor)
{
    LoadConfigFile();
}

OptionsDialog::~OptionsDialog()
{
}

void OptionsDialog::LoadConfigFile()
{
    m_entries.clear();
    m_entries.push_back({ "showWelcomeScreen", "true", "bool", { "true", "false" } });
    m_entries.push_back({ "showMinimap", "true", "bool", { "true", "false" } });
    m_entries.push_back({ "autoIndent", "true", "bool", { "true", "false" } });
    m_entries.push_back({ "tabWidth", "4", "int", { "2", "4", "8" } });
    m_entries.push_back({ "fontSize", "14", "int", { "8", "10", "12", "14", "16", "18", "20" } });
    m_entries.push_back({ "theme", "dark", "string", { "light", "dark" } });

    std::ifstream f(".pzeprc");
    if (!f.is_open())
        return;
    std::string line;
    while (std::getline(f, line))
    {
        size_t eq = line.find('=');
        if (eq != std::string::npos)
        {
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);
            for (auto& e : m_entries)
            {
                if (e.key == key)
                {
                    e.value = val;
                    break;
                }
            }
        }
    }
}

void OptionsDialog::SaveConfigFile()
{
    std::ofstream f(".pzeprc");
    if (!f.is_open())
        return;
    for (const auto& e : m_entries)
    {
        f << e.key << " = " << e.value << "\n";
    }
}

void OptionsDialog::Draw()
{
    if (!m_open)
        return;
    Rectangle bounds = { 150, 100, 500, 450 };
    if (GuiWindowBox(bounds, "Settings (.pzeprc)"))
    {
        m_open = false;
        SaveConfigFile();
    }
    int y = 130;
    for (auto& e : m_entries)
    {
        GuiLabel({ (float)170, (float)y, 150, 20 }, e.key.c_str());
        if (e.type == "bool")
        {
            bool b = (e.value == "true");
            if (GuiCheckBox("", &b, { 320, (float)y, 20, 20 }))
            {
                e.value = b ? "true" : "false";
                m_editor.SetCommandText(":set " + e.key + "=" + e.value);
            }
        }
        else if (e.type == "int" || e.type == "string")
        {
            if (!e.options.empty())
            {
                int idx = -1;
                for (size_t i = 0; i < e.options.size(); ++i)
                {
                    if (e.options[i] == e.value)
                    {
                        idx = (int)i;
                        break;
                    }
                }
                if (idx < 0)
                    idx = 0;
                if (GuiDropdownBox({ 320, (float)y, 120, 20 }, e.options.data(), &idx))
                {
                    e.value = e.options[idx];
                    m_editor.SetCommandText(":set " + e.key + "=" + e.value);
                }
            }
            else
            {
                static char buf[64];
                strncpy(buf, e.value.c_str(), sizeof(buf) - 1);
                if (GuiTextBox({ 320, (float)y, 120, 20 }, buf, sizeof(buf), false))
                {
                    e.value = buf;
                    m_editor.SetCommandText(":set " + e.key + "=" + e.value);
                }
            }
        }
        y += 30;
    }
    GuiLabel({ (float)170, (float)y, 300, 20 }, "(Changes saved to .pzeprc)");
}

} // namespace Zep