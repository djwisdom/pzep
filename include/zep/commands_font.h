#pragma once

#include <vector>

#include "zep/editor.h"

namespace Zep
{

class ZepExCommand_GetFonts : public ZepExCommand
{
public:
    ZepExCommand_GetFonts(ZepEditor& editor);
    const char* ExCommandName() const override;
    void Run(const std::vector<std::string>& args) override;
};

} // namespace Zep
