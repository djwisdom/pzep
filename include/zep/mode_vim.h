#pragma once

#include "mode.h"
#include "zep/keymap.h"

#include <map>
#include <string>

class Timer;

namespace Zep
{

struct SpanInfo;

enum class VimMotion
{
    LineBegin,
    LineEnd,
    NonWhiteSpaceBegin,
    NonWhiteSpaceEnd
};

class ZepMode_Vim : public ZepMode
{
public:
    ZepMode_Vim(ZepEditor& editor);
    virtual ~ZepMode_Vim();

    static const char* StaticName()
    {
        return "Vim";
    }

    // Zep Mode
    virtual void Init() override;
    virtual void Begin(ZepWindow* pWindow) override;
    virtual const char* Name() const override
    {
        return StaticName();
    }
    virtual EditorMode DefaultMode() const override
    {
        return EditorMode::Normal;
    }
    virtual void PreDisplay(ZepWindow& win) override;
    virtual void AddKeyPress(uint32_t key, uint32_t modifierKeys = ModifierKey::None) override;
    virtual bool GetCommand(CommandContext& context) override;
    virtual void SetupKeyMaps();
    virtual void AddOverStrikeMaps();
    virtual void AddCopyMaps();
    virtual void AddPasteMaps();
    virtual void AddGlobalKeyMaps();
    virtual void AddNavigationKeyMaps(bool allowInVisualMode = true);
    virtual void AddSearchKeyMaps();
    virtual bool UsesRelativeLines() const override
    {
        return m_useRelativeLineNumbers;
    }
    void SetUseRelativeLineNumbers(bool use)
    {
        m_useRelativeLineNumbers = use;
    }

private:
    bool m_useRelativeLineNumbers = true;

private:
    bool IsValidRegister(char reg) const;
    void StartRecording(char reg);
    void StopRecording();
    void PlayMacro(char reg, int count);
    std::string GetRegisterValue(char reg) const;

    std::map<char, std::string> m_macros;
    char m_recordingRegister = 0;
    char m_lastPlaybackRegister = 0;
};

} // namespace Zep
