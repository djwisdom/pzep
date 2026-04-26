#pragma once

#include "zep/editor.h"
#include <map>
#include <string>
#include <vector>

namespace Zep
{

struct ConfigEntry
{
    std::string key;
    std::string value;
    std::string type; // "bool", "int", "string", "font"
    std::vector<std::string> options; // for dropdowns
};

class OptionsDialog
{
public:
    OptionsDialog(ZepEditor& editor);
    ~OptionsDialog();

    void LoadConfigFile();
    void SaveConfigFile();
    void Draw();
    void Open()
    {
        m_open = true;
    }
    void Close()
    {
        m_open = false;
    }
    bool IsOpen() const
    {
        return m_open;
    }

private:
    ZepEditor& m_editor;
    bool m_open = false;
    std::vector<ConfigEntry> m_entries;
    std::map<std::string, std::string> m_modified;
};

} // namespace Zep