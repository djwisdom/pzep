#pragma once

#include "zep/mode.h"
#include "zep/terminal.h"

namespace Zep
{

class ZepMode_Terminal : public ZepMode
{
public:
    ZepMode_Terminal(ZepEditor& editor, ZepTerminal& terminal);
    virtual ~ZepMode_Terminal();

    virtual const char* Name() const override
    {
        return "Terminal";
    }

    virtual EditorMode DefaultMode() const override
    {
        return EditorMode::Insert;
    }

    virtual void Begin(ZepWindow* pWindow) override;
    virtual void Notify(std::shared_ptr<ZepMessage> message) override;
    virtual void PreDisplay(ZepWindow& window) override;

    // Override key handling to send to terminal
    virtual void AddCommandText(std::string strText) override;

private:
    ZepTerminal& m_terminal;
    bool m_handlingInput = false;
};

} // namespace Zep
