#pragma once

#include "zep/display.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Zep
{

class ZepFont_FTXUI : public ZepFont
{
public:
    ZepFont_FTXUI(ZepDisplay& display, int pixelHeight);
    virtual ~ZepFont_FTXUI();

    virtual void SetPixelHeight(int height) override;
    virtual NVec2f GetTextSize(const uint8_t* pBegin, const uint8_t* pEnd = nullptr) const override;

private:
    mutable std::unordered_map<std::string, int> m_widthCache;
};

class ZepDisplay_FTXUI : public ZepDisplay
{
public:
    ZepDisplay_FTXUI();
    virtual ~ZepDisplay_FTXUI();

    virtual void DrawLine(const NVec2f& start, const NVec2f& end, const NVec4f& color = NVec4f(1.0f), float width = 1.0f) const override;
    virtual void DrawChars(ZepFont& font, const NVec2f& pos, const NVec4f& col, const uint8_t* text_begin, const uint8_t* text_end = nullptr) const override;
    virtual void DrawRectFilled(const NRectf& rc, const NVec4f& col = NVec4f(1.0f)) const override;
    virtual void SetClipRect(const NRectf& rc) override;
    virtual ZepFont& GetFont(ZepTextType type) override;

    void BeginFrame();
    void EndFrame();
    bool ShouldClose() const;

    int GetKeyPressed();
    int GetCharPressed();
    bool IsKeyDown(int key);

    bool OnMouseDown(const NVec2f& mousePos, ZepMouseButton button);
    bool OnMouseUp(const NVec2f& mousePos, ZepMouseButton button);
    bool OnMouseMove(const NVec2f& mousePos);
    bool OnMouseWheel(const NVec2f& mousePos, float scrollDistance);

    void SetTerminalSize(int width, int height);
    int GetTerminalWidth() const
    {
        return m_terminalWidth;
    }
    int GetTerminalHeight() const
    {
        return m_terminalHeight;
    }

private:
    int GetColorIndex(const NVec4f& col) const;
    ftxui::Color GetFTXUIColor(const NVec4f& col) const;

    mutable int m_terminalWidth = 80;
    mutable int m_terminalHeight = 24;

    std::shared_ptr<ZepFont_FTXUI> m_spDefaultFont;
    NRectf m_clipRect;
    bool m_clipEnabled = false;

    std::vector<std::string> m_renderBuffer;
    mutable std::string m_currentLine;

    bool m_running = true;
};

} // namespace Zep