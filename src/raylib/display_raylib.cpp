#include "zep/raylib/display_raylib.h"
#include <cstdio>
#include <cstring>

// Note: Don't include windows.h - it conflicts with raylib function names

namespace Zep
{

//------------------------------------------------------------------------------------------
// Simple helper to get glyph index for a codepoint
//------------------------------------------------------------------------------------------
static int GetGlyphIndexOrZero(::Font font, int codepoint)
{
    // Simple approach: check if codepoint is in basic ASCII range
    if (codepoint >= 32 && codepoint <= 126)
        return codepoint - 32;
    // For other chars, just return 0 (will show as missing glyph)
    return 0;
}

//------------------------------------------------------------------------------------------
// ZepFont_Raylib
//------------------------------------------------------------------------------------------

ZepFont_Raylib::ZepFont_Raylib(ZepDisplay& display, ::Font font, int pixelHeight)
    : ZepFont(display)
    , m_font(font)
    , m_baseSize(pixelHeight)
{
    m_pixelHeight = pixelHeight;
}

ZepFont_Raylib::~ZepFont_Raylib()
{
}

void ZepFont_Raylib::SetPixelHeight(int height)
{
    m_pixelHeight = height;
    m_charCacheDirty = true;
}

NVec2f ZepFont_Raylib::GetTextSize(const uint8_t* pBegin, const uint8_t* pEnd) const
{
    if (pBegin == nullptr || pBegin == pEnd)
        return NVec2f(0, (float)m_pixelHeight);

    if (pEnd == nullptr)
        pEnd = pBegin + strlen((const char*)pBegin);

    // Use m_pixelHeight which is now 32 (matches font atlas)
    float width = MeasureTextEx(m_font, (const char*)pBegin, (float)m_pixelHeight, 1.0f).x;
    return NVec2f(width, (float)m_pixelHeight);
}

//------------------------------------------------------------------------------------------
// ZepDisplay_Raylib
//------------------------------------------------------------------------------------------

ZepDisplay_Raylib::ZepDisplay_Raylib(int width, int height)
    : ZepDisplay()
    , m_width(width)
    , m_height(height)
    , m_clipEnabled(false)
{
    SetExitKey(0);
    InitWindow(width, height, "pZep-GUI - Vim-like Editor");
    SetTargetFPS(60);

    // Load font at size 16 with ASCII codepoints
    // Smaller size for a more standard editor appearance
    std::vector<int> codepoints;
    for (int c = 32; c <= 126; c++)
        codepoints.push_back(c);

    m_defaultFont = LoadFontEx("C:/Windows/Fonts/CascadiaMono.ttf", 16, codepoints.data(), (int)codepoints.size());
    printf("INFO: FONT: CascadiaMono loaded: baseSize=%d, glyphCount=%d\n", m_defaultFont.baseSize, m_defaultFont.glyphCount);
    fflush(stdout);

    // If load failed, fall back to default raylib font
    if (m_defaultFont.glyphCount < 10)
    {
        if (m_defaultFont.texture.id != 0)
            UnloadFont(m_defaultFont);
        m_defaultFont = GetFontDefault();
    }

    // Create default font entry - tell Zep the pixel height is 16
    // This matches the font atlas size, so measurement and drawing are consistent
    m_spDefaultFont = std::make_shared<ZepFont_Raylib>(*this, m_defaultFont, 16);
    printf("INFO: FONT: ZepFont_Raylib created with m_pixelHeight=16\n");
    fflush(stdout);
}

ZepDisplay_Raylib::~ZepDisplay_Raylib()
{
    if (m_defaultFont.texture.id != 0 && m_defaultFont.texture.id != GetFontDefault().texture.id)
    {
        UnloadFont(m_defaultFont);
    }
    CloseWindow();
}

void ZepDisplay_Raylib::DrawLine(const NVec2f& start, const NVec2f& end, const NVec4f& color, float width) const
{
    Color c = {
        (unsigned char)(color.x * 255.0f),
        (unsigned char)(color.y * 255.0f),
        (unsigned char)(color.z * 255.0f),
        (unsigned char)(color.w * 255.0f)
    };
    DrawLineEx({ start.x, start.y }, { end.x, end.y }, width, c);
}

void ZepDisplay_Raylib::DrawChars(ZepFont& font, const NVec2f& pos, const NVec4f& col,
    const uint8_t* text_begin, const uint8_t* text_end) const
{
    if (!text_begin || !*text_begin)
        return;

    if (text_end == nullptr)
        text_end = text_begin + strlen((const char*)text_begin);

    Color c = {
        (unsigned char)(col.x * 255.0f),
        (unsigned char)(col.y * 255.0f),
        (unsigned char)(col.z * 255.0f),
        (unsigned char)(col.w * 255.0f)
    };

    int fontSize = font.GetPixelHeight();
    if (fontSize <= 0)
        fontSize = 16;

    // Round position to integer to avoid pixel alignment issues
    float x = roundf(pos.x);
    float y = roundf(pos.y);

    // Use spacing of 1.0 to match MeasureTextEx in GetTextSize
    DrawTextEx(m_defaultFont, (const char*)text_begin, { x, y }, (float)fontSize, 1.0f, c);
}

void ZepDisplay_Raylib::DrawRectFilled(const NRectf& rc, const NVec4f& col) const
{
    Color c = {
        (unsigned char)(col.x * 255.0f),
        (unsigned char)(col.y * 255.0f),
        (unsigned char)(col.z * 255.0f),
        (unsigned char)(col.w * 255.0f)
    };
    DrawRectangle((int)rc.Left(), (int)rc.Top(), (int)rc.Width(), (int)rc.Height(), c);
}

void ZepDisplay_Raylib::SetClipRect(const NRectf& rc)
{
    m_clipRect = rc;
    m_clipEnabled = true;
    // Raylib uses BeginScissorMode for clipping
    // Will be applied in BeginFrame/EndFrame
}

ZepFont& ZepDisplay_Raylib::GetFont(ZepTextType type)
{
    if (m_fonts[(int)type] == nullptr)
    {
        if (m_spDefaultFont == nullptr)
        {
            m_spDefaultFont = std::make_shared<ZepFont_Raylib>(*this, m_defaultFont, 16);
        }
        return *m_spDefaultFont;
    }
    return *m_fonts[(int)type];
}

bool ZepDisplay_Raylib::ShouldClose() const
{
    return WindowShouldClose();
}

void ZepDisplay_Raylib::BeginFrame()
{
    BeginDrawing();

    NVec4f bgColor(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
    ClearBackground(bgColor);

    if (m_clipEnabled)
    {
        BeginScissorMode((int)m_clipRect.Left(), (int)m_clipRect.Top(),
            (int)m_clipRect.Width(), (int)m_clipRect.Height());
    }
}

void ZepDisplay_Raylib::EndFrame()
{
    if (m_clipEnabled)
    {
        EndScissorMode();
        m_clipEnabled = false;
    }
    EndDrawing();
}

void ZepDisplay_Raylib::ClearBackground(const NVec4f& col)
{
    Color c = {
        (unsigned char)(col.x),
        (unsigned char)(col.y),
        (unsigned char)(col.z),
        (unsigned char)(col.w)
    };
    ::ClearBackground(c);
}

int ZepDisplay_Raylib::GetKeyPressed()
{
    return ::GetKeyPressed();
}

int ZepDisplay_Raylib::GetCharPressed()
{
    return ::GetCharPressed();
}

bool ZepDisplay_Raylib::IsKeyDown(int key)
{
    return ::IsKeyDown(key);
}

bool ZepDisplay_Raylib::IsMouseButtonDown(int button)
{
    return ::IsMouseButtonDown(button);
}

Vector2 ZepDisplay_Raylib::GetMousePosition()
{
    return ::GetMousePosition();
}

} // namespace Zep