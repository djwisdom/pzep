#include "zep/ftxui/display_ftxui.h"
#include <cstring>
#include <iostream>

namespace Zep
{

ZepFont_FTXUI::ZepFont_FTXUI(ZepDisplay& display, int pixelHeight)
    : ZepFont(display)
    , m_pixelHeight(pixelHeight)
{
}

ZepFont_FTXUI::~ZepFont_FTXUI()
{
}

void ZepFont_FTXUI::SetPixelHeight(int height)
{
    m_pixelHeight = height;
    m_widthCache.clear();
}

NVec2f ZepFont_FTXUI::GetTextSize(const uint8_t* pBegin, const uint8_t* pEnd) const
{
    if (pBegin == nullptr || pBegin == pEnd)
        return NVec2f(0, (float)m_pixelHeight);

    if (pEnd == nullptr)
        pEnd = pBegin + strlen((const char*)pBegin);

    std::string text(reinterpret_cast<const char*>(pBegin), pEnd - pBegin);

    auto it = m_widthCache.find(text);
    if (it != m_widthCache.end())
    {
        return NVec2f((float)it->second, (float)m_pixelHeight);
    }

    int width = (int)ftxui::string_width(text);
    m_widthCache[text] = width;

    return NVec2f((float)width, (float)m_pixelHeight);
}

ZepDisplay_FTXUI::ZepDisplay_FTXUI()
    : ZepDisplay()
{
    auto terminal = ftxui::Terminal::Get();
    m_terminalWidth = terminal.dim_x();
    m_terminalHeight = terminal.dim_y();

    m_spDefaultFont = std::make_shared<ZepFont_FTXUI>(*this, 16);
}

ZepDisplay_FTXUI::~ZepDisplay_FTXUI()
{
}

void ZepDisplay_FTXUI::DrawLine(const NVec2f& start, const NVec2f& end, const NVec4f& color, float width) const
{
}

void ZepDisplay_FTXUI::DrawChars(ZepFont& font, const NVec2f& pos, const NVec4f& col,
    const uint8_t* text_begin, const uint8_t* text_end) const
{
    if (!text_begin || !*text_begin)
        return;

    if (text_end == nullptr)
        text_end = text_begin + strlen((const char*)text_begin);

    auto ftCol = GetFTXUIColor(col);
    auto element = ftxui::text(std::string((const char*)text_begin, text_end - text_begin)) | ftxui::color(ftCol);

    m_currentLine += std::string((const char*)text_begin, text_end - text_begin);
}

void ZepDisplay_FTXUI::DrawRectFilled(const NRectf& rc, const NVec4f& col) const
{
    auto ftCol = GetFTXUIColor(col);
    int width = (int)rc.Width();
    if (width <= 0)
        width = 1;
    int height = (int)rc.Height();
    if (height <= 0)
        height = 1;

    std::string fill(width, ' ');
}

void ZepDisplay_FTXUI::SetClipRect(const NRectf& rc)
{
    m_clipRect = rc;
    m_clipEnabled = true;
}

ZepFont& ZepDisplay_FTXUI::GetFont(ZepTextType type)
{
    if (m_fonts[(int)type] == nullptr)
    {
        if (m_spDefaultFont == nullptr)
        {
            m_spDefaultFont = std::make_shared<ZepFont_FTXUI>(*this, 16);
        }
        return *m_spDefaultFont;
    }
    return *m_fonts[(int)type];
}

void ZepDisplay_FTXUI::BeginFrame()
{
    m_currentLine.clear();
}

void ZepDisplay_FTXUI::EndFrame()
{
    m_running = false;
}

bool ZepDisplay_FTXUI::ShouldClose() const
{
    return !m_running;
}

int ZepDisplay_FTXUI::GetKeyPressed()
{
    return 0;
}

int ZepDisplay_FTXUI::GetCharPressed()
{
    return 0;
}

bool ZepDisplay_FTXUI::IsKeyDown(int key)
{
    return false;
}

bool ZepDisplay_FTXUI::OnMouseDown(const NVec2f& mousePos, ZepMouseButton button)
{
    return false;
}

bool ZepDisplay_FTXUI::OnMouseUp(const NVec2f& mousePos, ZepMouseButton button)
{
    return false;
}

bool ZepDisplay_FTXUI::OnMouseMove(const NVec2f& mousePos)
{
    return false;
}

bool ZepDisplay_FTXUI::OnMouseWheel(const NVec2f& mousePos, float scrollDistance)
{
    return false;
}

void ZepDisplay_FTXUI::SetTerminalSize(int width, int height)
{
    m_terminalWidth = width;
    m_terminalHeight = height;
}

int ZepDisplay_FTXUI::GetColorIndex(const NVec4f& col) const
{
    int r = (int)(col.x * 255);
    int g = (int)(col.y * 255);
    int b = (int)(col.z * 255);

    if (r == 0 && g == 0 && b == 0)
        return 0;
    if (r == 128 && g == 0 && b == 0)
        return 1;
    if (r == 0 && g == 128 && b == 0)
        return 2;
    if (r == 128 && g == 128 && b == 0)
        return 3;
    if (r == 0 && g == 0 && b == 128)
        return 4;
    if (r == 128 && g == 0 && b == 128)
        return 5;
    if (r == 0 && g == 128 && b == 128)
        return 6;
    if (r == 192 && g == 192 && b == 192)
        return 7;

    return 8;
}

ftxui::Color ZepDisplay_FTXUI::GetFTXUIColor(const NVec4f& col) const
{
    return ftxui::Color::RGB(
        (int)(col.x * 255),
        (int)(col.y * 255),
        (int)(col.z * 255));
}

} // namespace Zep