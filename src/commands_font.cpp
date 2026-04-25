#include "zep/commands_font.h"

#include "zep/editor.h"

#include <sstream>

namespace Zep
{

ZepExCommand_GetFonts::ZepExCommand_GetFonts(ZepEditor& editor)
    : ZepExCommand(editor)
{
}

const char* ZepExCommand_GetFonts::ExCommandName() const
{
    return "fonts";
}

void ZepExCommand_GetFonts::Run(const std::vector<std::string>& args)
{
    auto fonts = GetEditor().GetDisplay().GetAvailableMonospaceFonts();
    std::ostringstream str;
    str << "Available monospace fonts (" << fonts.size() << "):\n";
    for (const auto& f : fonts)
        str << "  " << f << "\n";
    GetEditor().SetCommandText(str.str());
}

} // namespace Zep
