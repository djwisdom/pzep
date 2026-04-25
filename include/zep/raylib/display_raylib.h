#pragma once

#include <memory>
#include <raylib.h>
#include <string>

#include "zep/display.h"

namespace Zep
{

// Raylib-based font implementation
class ZepFont_Raylib : public ZepFont
{
public:
    ZepFont_Raylib(ZepDisplay& display, ::Font font, int pixelHeight);
    virtual ~ZepFont_Raylib();

    virtual void SetPixelHeight(int height) override;
    virtual NVec2f GetTextSize(const uint8_t* pBegin, const uint8_t* pEnd = nullptr) const override;

    ::Font GetRaylibFont() const
    {
        return m_font;
    }

    ::Font GetFont() const
    {
        return m_font;
    }

private:
    ::Font m_font;
    int m_baseSize;
};

// Raylib-based display implementation
class ZepDisplay_Raylib : public ZepDisplay
{
public:
    ZepDisplay_Raylib(int width = 1024, int height = 768);
    virtual ~ZepDisplay_Raylib();

    // ZepDisplay virtual overrides
    virtual void DrawLine(const NVec2f& start, const NVec2f& end, const NVec4f& color = NVec4f(1.0f), float width = 1.0f) const override;
    virtual void DrawChars(ZepFont& font, const NVec2f& pos, const NVec4f& col, const uint8_t* text_begin, const uint8_t* text_end = nullptr) const override;
    virtual void DrawRectFilled(const NRectf& rc, const NVec4f& col = NVec4f(1.0f)) const override;
    virtual void SetClipRect(const NRectf& rc) override;
    virtual ZepFont& GetFont(ZepTextType type) override;

    // Clipboard
    std::string GetClipboardText() const override;
    void SetClipboardText(const std::string& text) override;

    // Raylib-specific methods (not overrides)
    bool ShouldClose() const;
    void ToggleFullscreen();
    void BeginFrame();
    void EndFrame();
    void ClearBackground(const NVec4f& col);

    int GetScreenWidth() const
    {
        return m_width;
    }
    int GetScreenHeight() const
    {
        return m_height;
    }

    // Input handling
    int GetKeyPressed();
    int GetCharPressed();
    bool IsKeyDown(int key);
    bool IsMouseButtonDown(int button);
    Vector2 GetMousePosition();

private:
    int m_width;
    int m_height;
    ::Font m_defaultFont;
    NRectf m_clipRect;
    bool m_clipEnabled;
    int m_lastWindowedWidth = 1024;
    int m_lastWindowedHeight = 768;
};

} // namespace Zep