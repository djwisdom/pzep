#pragma once

#include <vector>

#include "zep/editor.h"

namespace Zep
{

class ZepExCommand_Tutor : public ZepExCommand
{
public:
    ZepExCommand_Tutor(ZepEditor& editor);
    ~ZepExCommand_Tutor() override = default;

    const char* ExCommandName() const override
    {
        return "tutor";
    }

    void Run(const std::vector<std::string>& args) override;
};

} // namespace Zep
