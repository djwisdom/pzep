#include "zep/raylib/display_raylib.h"

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
#include <wingdi.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

namespace
{
struct EnumData
{
    std::vector<std::string>* pNames = nullptr;
    std::unordered_set<std::string>* pSeen = nullptr;
};

int CALLBACK EnumFontFamExProc(const LOGFONTW* lpelfe, const TEXTMETRICW* /*lpntme*/, DWORD /*FontType*/, LPARAM lParam)
{
    EnumData* data = reinterpret_cast<EnumData*>(lParam);
    if (!data || !data->pNames || !data->pSeen)
        return 1;
    std::wstring wname(lpelfe->lfFaceName);
    std::string name(wname.begin(), wname.end());
    // Fixed pitch check
    if (lpelfe->lfPitchAndFamily & FIXED_PITCH)
    {
        if (data->pSeen->insert(name).second)
        {
            data->pNames->push_back(name);
        }
    }
    return 1;
}
} // namespace

ZepDisplay_Raylib::ZepDisplay_Raylib(int width, int height)
    : ZepDisplay()
    , m_width(width)
    , m_height(height)
    , m_clipEnabled(false)
{
    // ... existing window init ...

    // Build comprehensive Unicode codepoint set for font atlas
    std::set<int> cpset;

    // ... existing ranges ...

    // Convert set to vector for LoadFontEx
    m_fontCodepoints = std::vector<int>(cpset.begin(), cpset.end());

    // Load default font at larger size (72) to allow crisp rendering at all zoom levels up to max (72)
    m_defaultFont = LoadFontEx("C:/Windows/Fonts/CascadiaMono.ttf", 72, m_fontCodepoints.data(), (int)m_fontCodepoints.size());
    printf("INFO: FONT: CascadiaMono loaded: baseSize=%d, glyphCount=%d, codepoints=%zu\n", m_defaultFont.baseSize, m_defaultFont.glyphCount, m_fontCodepoints.size());
    fflush(stdout);

    // Record current font name
    m_currentFontName = "Cascadia Mono";

    // Enumerate available monospace fonts on the system
    EnumerateMonospaceFonts();

    // Ensure the default loaded font is in the list of available fonts
    if (std::find(m_monospaceFonts.begin(), m_monospaceFonts.end(), m_currentFontName) == m_monospaceFonts.end())
    {
        m_monospaceFonts.push_back(m_currentFontName);
        std::sort(m_monospaceFonts.begin(), m_monospaceFonts.end());
        if (m_fontPathMap.find(m_currentFontName) == m_fontPathMap.end())
        {
            m_fontPathMap[m_currentFontName] = "C:/Windows/Fonts/CascadiaMono.ttf";
        }
    }

    // If load failed, fall back to default raylib font
    if (m_defaultFont.glyphCount < 10)
    {
        if (m_defaultFont.texture.id != 0)
            UnloadFont(m_defaultFont);
        m_defaultFont = GetFontDefault();
    }

    // Use bilinear filtering for smoother text (crisper than point sampling)
    SetTextureFilter(m_defaultFont.texture, TEXTURE_FILTER_BILINEAR);

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

// Font enumeration and management
void ZepDisplay_Raylib::EnumerateMonospaceFonts()
{
#ifdef _WIN32
    std::vector<std::string> faceNames;
    std::unordered_set<std::string> seen;

    EnumData data;
    data.pNames = &faceNames;
    data.pSeen = &seen;

    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        LOGFONTW lf = { 0 };
        lf.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExW(hdc, &lf, EnumFontFamExProc, (LPARAM)&data, 0);
        ReleaseDC(NULL, hdc);
    }

    // Build registry map of font family name -> file path
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD i = 0;
        WCHAR valueName[512];
        BYTE valueData[512];
        while (true)
        {
            DWORD nameSize = sizeof(valueName) / sizeof(WCHAR);
            DWORD dataSize = sizeof(valueData);
            LONG ret = RegEnumValueW(hKey, i, valueName, &nameSize, NULL, NULL, valueData, &dataSize);
            if (ret != ERROR_SUCCESS)
                break;
            std::wstring wName(valueName);
            std::string nameStr(wName.begin(), wName.end());
            size_t parenPos = nameStr.find('(');
            if (parenPos != std::string::npos)
            {
                nameStr = nameStr.substr(0, parenPos);
                while (!nameStr.empty() && isspace(static_cast<unsigned char>(nameStr.back())))
                    nameStr.pop_back();
            }
            // Convert valueData (REG_SZ) to wstring then string
            std::wstring wFile(reinterpret_cast<wchar_t*>(valueData));
            std::string fileName(wFile.begin(), wFile.end());
            std::string fullPath = "C:\\Windows\\Fonts\\" + fileName;
            if (m_fontPathMap.find(nameStr) == m_fontPathMap.end())
            {
                m_fontPathMap[nameStr] = fullPath;
            }
            i++;
        }
        RegCloseKey(hKey);
    }

    // Intersect: only fonts that are both fixed-pitch and have a file path
    for (const auto& name : faceNames)
    {
        if (m_fontPathMap.find(name) != m_fontPathMap.end())
        {
            m_monospaceFonts.push_back(name);
        }
    }

    std::sort(m_monospaceFonts.begin(), m_monospaceFonts.end());
#endif // _WIN32
}

std::vector<std::string> ZepDisplay_Raylib::GetAvailableMonospaceFonts() const
{
    return m_monospaceFonts;
}

std::string ZepDisplay_Raylib::GetCurrentFontName() const
{
    return m_currentFontName;
}

bool ZepDisplay_Raylib::SetFontByName(const std::string& fontName)
{
#ifdef _WIN32
    std::string path;
    std::string actualName;

    auto it = m_fontPathMap.find(fontName);
    if (it != m_fontPathMap.end())
    {
        actualName = it->first;
        path = it->second;
    }
    else
    {
        // Case-insensitive search
        std::string lowerTarget = fontName;
        std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::tolower);
        bool found = false;
        for (const auto& kv : m_fontPathMap)
        {
            std::string key = kv.first;
            std::string keyLower = key;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
            if (keyLower == lowerTarget)
            {
                actualName = kv.first;
                path = kv.second;
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    // Load the font using stored codepoints
    ::Font newFont = LoadFontEx(path.c_str(), 72, m_fontCodepoints.data(), (int)m_fontCodepoints.size());
    if (newFont.glyphCount < 10)
        return false;

    // Unload old font if it's not the default raylib font
    if (m_defaultFont.texture.id != 0 && m_defaultFont.texture.id != GetFontDefault().texture.id)
    {
        UnloadFont(m_defaultFont);
    }
    m_defaultFont = newFont;

    // Preserve current pixel height (or use current text font height)
    int currentHeight = 16;
    if (m_fonts[(int)ZepTextType::Text])
    {
        currentHeight = m_fonts[(int)ZepTextType::Text]->GetPixelHeight();
    }

    auto newSpFont = std::make_shared<ZepFont_Raylib>(*this, m_defaultFont, currentHeight);
    SetFont(ZepTextType::Text, newSpFont);
    m_spDefaultFont = newSpFont;

    SetLayoutDirty(true);
    m_currentFontName = actualName;
    return true;
#else
    return false;
#endif
}

} // namespace Zep