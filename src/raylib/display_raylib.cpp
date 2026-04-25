#include "zep/raylib/display_raylib.h"

#include <cstdio>
#include <cstring>
#include <set>
#include <string>

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

    size_t len = pEnd - pBegin;
    std::string buffer;
    buffer.assign((const char*)pBegin, len);

    float width = MeasureTextEx(m_font, buffer.c_str(), (float)m_pixelHeight, 1.0f).x;
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
    // Create window first
    InitWindow(width, height, "pZep-GUI - Vim-like Editor");

    // Disable ALL exit keys AFTER window creation - we handle closing ourselves via :q command
    SetExitKey(KEY_NULL); // KEY_NULL = 0, no key closes window
    // Enable resizable window
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    // Build comprehensive Unicode codepoint set for font atlas
    std::set<int> cpset;

    // Basic Latin (32-126) and Latin-1 Supplement (160-255)
    for (int c = 32; c <= 126; ++c)
        cpset.insert(c);
    for (int c = 160; c <= 255; ++c)
        cpset.insert(c);

    // Latin Extended-A (256-383)
    for (int c = 256; c <= 383; ++c)
        cpset.insert(c);
    // Latin Extended-B (384-591)
    for (int c = 384; c <= 591; ++c)
        cpset.insert(c);
    // IPA Extensions (592-687)
    for (int c = 592; c <= 687; ++c)
        cpset.insert(c);
    // Spacing Modifier Letters (688-767)
    for (int c = 688; c <= 767; ++c)
        cpset.insert(c);
    // Combining Diacritical Marks (768-879)
    for (int c = 768; c <= 879; ++c)
        cpset.insert(c);
    // Greek and Coptic (880-1023)
    for (int c = 880; c <= 1023; ++c)
        cpset.insert(c);
    // Cyrillic (1024-1279)
    for (int c = 1024; c <= 1279; ++c)
        cpset.insert(c);
    // Cyrillic Supplement (1280-1327)
    for (int c = 1280; c <= 1327; ++c)
        cpset.insert(c);
    // Georgian (1424-1516)
    for (int c = 1424; c <= 1516; ++c)
        cpset.insert(c);
    // Box Drawing (9472-9599) — for line art
    for (int c = 9472; c <= 9599; ++c)
        cpset.insert(c);
    // Block Elements (9600-9631)
    for (int c = 9600; c <= 9631; ++c)
        cpset.insert(c);
    // Geometric Shapes (9632-9727)
    for (int c = 9632; c <= 9727; ++c)
        cpset.insert(c);
    // Miscellaneous Symbols (9728-9983)
    for (int c = 9728; c <= 9983; ++c)
        cpset.insert(c);

    // Convert set to vector for LoadFontEx
    std::vector<int> codepoints(cpset.begin(), cpset.end());

    // Load font at larger size (72) to allow crisp rendering at all zoom levels up to max (72)
    m_defaultFont = LoadFontEx("C:/Windows/Fonts/CascadiaMono.ttf", 72, codepoints.data(), (int)codepoints.size());
    printf("INFO: FONT: CascadiaMono loaded: baseSize=%d, glyphCount=%d, codepoints=%zu\n", m_defaultFont.baseSize, m_defaultFont.glyphCount, codepoints.size());
    fflush(stdout);

    // If load failed, fall back to default raylib font
    if (m_defaultFont.glyphCount < 10)
    {
        if (m_defaultFont.texture.id != 0)
            UnloadFont(m_defaultFont);
        m_defaultFont = GetFontDefault();
    }

    // Create default font entry
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

std::string ZepDisplay_Raylib::GetClipboardText() const
{
    const char* text = ::GetClipboardText();
    if (text)
    {
        std::string result = text;
        // Note: UnloadClipboardText not available in some raylib versions; ignore
        return result;
    }
    return "";
}

void ZepDisplay_Raylib::SetClipboardText(const std::string& text)
{
    ::SetClipboardText(text.c_str());
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

    size_t len = text_end - text_begin;
    std::string buffer;
    buffer.assign((const char*)text_begin, len);

    Color c = {
        (unsigned char)(col.x * 255.0f),
        (unsigned char)(col.y * 255.0f),
        (unsigned char)(col.z * 255.0f),
        (unsigned char)(col.w * 255.0f)
    };

    int fontSize = font.GetPixelHeight();
    if (fontSize <= 0)
        fontSize = 16;

    float x = roundf(pos.x);
    float y = roundf(pos.y);

    // Get the underlying raylib font from the ZepFont implementation
    const ZepFont_Raylib* pRayFont = dynamic_cast<const ZepFont_Raylib*>(&font);
    ::Font raylibFont = pRayFont ? pRayFont->GetRaylibFont() : m_defaultFont;

    DrawTextEx(raylibFont, buffer.c_str(), { x, y }, (float)fontSize, 1.0f, c);
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
    // Override - don't let ESC close window, we handle closing ourselves via :q
    // But we can't check from const method, so just never return true from ESC
    // This is handled by checking IsKeyDown in main loop before calling ShouldClose
    return WindowShouldClose();
}

void ZepDisplay_Raylib::ToggleFullscreen()
{
    if (IsWindowFullscreen())
    {
        SetWindowState(FLAG_WINDOW_UNDECORATED); // Remove fullscreen flag
        SetWindowSize(m_lastWindowedWidth, m_lastWindowedHeight);
        // Center the window when exiting fullscreen
        int screenWidth = GetMonitorWidth(GetCurrentMonitor());
        int screenHeight = GetMonitorHeight(GetCurrentMonitor());
        SetWindowPosition((screenWidth - m_lastWindowedWidth) / 2, (screenHeight - m_lastWindowedHeight) / 2);
    }
    else
    {
        // Save current windowed size
        m_lastWindowedWidth = GetScreenWidth();
        m_lastWindowedHeight = GetScreenHeight();
        // Enter fullscreen on current monitor
        SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_FULLSCREEN_MODE);
        // Size to match the monitor
        int screenWidth = GetMonitorWidth(GetCurrentMonitor());
        int screenHeight = GetMonitorHeight(GetCurrentMonitor());
        SetWindowSize(screenWidth, screenHeight);
    }
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