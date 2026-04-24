#pragma once

#include <cstdint>

#include "zep/mode.h"

namespace Zep
{

class ZepMode_Tutorial : public ZepMode
{
public:
    ZepMode_Tutorial(ZepEditor& editor, int startLesson = 1);
    ~ZepMode_Tutorial() override = default;

    void Init() override;
    void Begin(ZepWindow* pWindow) override;
    const char* Name() const override
    {
        return "Tutorial";
    }
    EditorMode DefaultMode() const override
    {
        return EditorMode::Normal;
    }

    void AddKeyPress(uint32_t key, uint32_t modifierKeys = ModifierKey::None) override;

private:
    void NextLesson();
    void PrevLesson();
    void LoadLesson(int num);
    bool HasLesson(int num) const;

    // Demo feature triggers
    void DemoSplitHorizontal();
    void DemoSplitVertical();
    void DemoTabNew();
    void DemoTabClose();
    void DemoREPL();
    void DemoREPLLua();
    void DemoREPLDuktape();
    void DemoREPLQuickJS();

    int m_currentLesson = 1;
    bool m_demoMode = false;
};

} // namespace Zep
