#include "zep/commands_repl.h"
#include "zep/commands_tutor.h"
#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mode_repl.h"
#include "zep/mode_vim.h"
#include "zep/raylib/display_raylib.h"
#include "zep/repl_plugin_loader.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

// Include raylib directly for direct drawing in welcome screen
#include <raylib.h>

using namespace Zep;

// Forward declarations for version dialog functions
void ShowVersionDialog();
void ShowHelpDialog();
void ShowInfoDialog();

namespace
{
std::unordered_map<int, int> g_keyMap = {
    { 256, 1 }, // ESCAPE
    { 257, 0 }, // RETURN
    { 259, 2 }, // BACKSPACE
    { 258, 7 }, // TAB
    { 262, 4 }, // RIGHT
    { 263, 3 }, // LEFT
    { 265, 5 }, // UP
    { 264, 6 }, // DOWN
    { 261, 8 }, // DEL
    { 268, 9 }, // HOME
    { 269, 10 }, // END
    { 270, 12 }, // PAGEUP
    { 271, 11 }, // PAGEDOWN
    { 290, 13 }, // F1
    { 291, 14 }, // F2
    { 292, 15 }, // F3
    { 293, 16 }, // F4
    { 294, 17 }, // F5
    { 295, 18 }, // F6
    { 296, 19 }, // F7
    { 297, 20 }, // F8
    { 298, 21 }, // F9
    { 299, 22 }, // F10
    { 300, 23 }, // F11
    { 301, 24 }, // F12
};

int GetModifiers()
{
    int mod = 0;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        mod |= 1;
    if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))
        mod |= 2;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
        mod |= 4;
    return mod;
}

void DrawWelcomeScreen(ZepDisplay& display, ZepFont& font)
{
    // Use raylib directly for screen dimensions
    int screenW = GetRenderWidth();
    int screenH = GetRenderHeight();

    // Semi-transparent dark overlay
    Color overlay = { 0, 0, 0, 200 };
    DrawRectangle(0, 0, screenW, screenH, overlay);

    // Build version string from compile-time defines
    char versionStr[64];
    snprintf(versionStr, sizeof(versionStr), "version %d.%d.%d", ZEP_VERSION_MAJOR, ZEP_VERSION_MINOR, ZEP_VERSION_PATCH);

    // Centered text lines
    const char* title = "pZep - a VIM-like editor. 'nuff said.";
    const char* author = "by Dennis O. Esternon et al.";
    const char* tagline = "pZep is open source and freely distributable";
    const char* help1 = "type :q<Enter>       to exit";
    const char* help2 = "type :version<Enter> for version info";

    int fontSize = 24;
    auto titleSize = MeasureTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), title, fontSize, 1.0f);
    auto versionSize = MeasureTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), versionStr, fontSize, 1.0f);
    auto authorSize = MeasureTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), author, fontSize, 1.0f);
    int smallSize = 20;
    auto taglineSize = MeasureTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), tagline, smallSize, 1.0f);
    auto help1Size = MeasureTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), help1, smallSize, 1.0f);

    float centerX = screenW * 0.5f;
    float startY = screenH * 0.35f;
    float lineSpacing = fontSize * 1.6f;

    float x;

    x = centerX - titleSize.x * 0.5f;
    DrawTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), title, { x, startY }, fontSize, 1.0f, WHITE);

    x = centerX - versionSize.x * 0.5f;
    DrawTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), versionStr, { x, startY + lineSpacing }, fontSize, 1.0f, LIGHTGRAY);

    x = centerX - authorSize.x * 0.5f;
    DrawTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), author, { x, startY + lineSpacing * 2.0f }, fontSize, 1.0f, RAYWHITE);

    x = centerX - taglineSize.x * 0.5f;
    DrawTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), tagline, { x, startY + lineSpacing * 3.5f }, smallSize, 1.0f, YELLOW);

    x = centerX - help1Size.x * 0.5f;
    DrawTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), help1, { x, startY + lineSpacing * 5.0f }, smallSize, 1.0f, GREEN);
    x = centerX - help1Size.x * 0.5f;
    DrawTextEx(((ZepFont_Raylib*)&font)->GetRaylibFont(), help2, { x, startY + lineSpacing * 5.8f }, smallSize, 1.0f, GREEN);
}

} // namespace

int main(int argc, char* argv[])
{
    // Handle command-line flags before loading any files
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            ShowVersionDialog();
            return 0;
        }
        if (strcmp(argv[i], "--help") == 0)
        {
            ShowHelpDialog();
            return 0;
        }
        if (strcmp(argv[i], "--info") == 0)
        {
            ShowInfoDialog();
            return 0;
        }
    }

    const char* file = (argc > 1) ? argv[1] : "untitled";

    // Determine home directory and create ~/.pzep with default config if needed
    std::filesystem::path homeDir;
    const char* homeEnv = std::getenv("USERPROFILE");
    if (!homeEnv)
        homeEnv = std::getenv("HOME");
    if (homeEnv)
        homeDir = homeEnv;
    else
        homeDir = std::filesystem::current_path();

    std::filesystem::path pzepDir = homeDir / ".pzep";
    std::filesystem::create_directories(pzepDir);
    std::filesystem::path backupDir = pzepDir / "backup";
    std::filesystem::create_directories(backupDir);

    // Initialize default .pzeprc if not present
    std::filesystem::path pzeprcPath = pzepDir / ".pzeprc";
    if (!std::filesystem::exists(pzeprcPath))
    {
        std::ofstream out(pzeprcPath);
        out << "\" pzeprc - pZep configuration file\n";
        out << "\" Settings are similar to vimrc\n\n";
        out << "set number\n";
        out << "set nolist\n";
        out << "set wrap\n";
        out << "set noautoindent\n";
        out << "set expandtab\n";
        out << "set tabstop=4\n";
        out << "set shiftwidth=4\n";
    }

    ZepDisplay_Raylib display(1280, 800);
    ZepEditor editor(&display, pzepDir); // Use pzepDir as config root
    editor.SetGlobalMode(ZepMode_Vim::StaticName());

    // Load REPL plugins from the plugins/ directory (if any)
    // Plugin loading is disabled by default for security; enable via config
    // InitializeReplPluginLoader(&editor, "plugins");

#if defined(ZEP_ENABLE_LUA_REPL)
    RegisterLuaReplProvider(editor);
#endif
#if defined(ZEP_ENABLE_DUKTAPE_REPL)
    RegisterDuktapeReplProvider(editor);
#endif
#if defined(ZEP_ENABLE_QUICKJS_REPL)
    RegisterQuickJSEvalReplProvider(editor);
#endif

    // Load file from command line argument or create new "untitled" buffer
    ZepBuffer* buffer = editor.InitWithFileOrDir(file);

    // Debug: check if file was loaded
    if (buffer)
    {
        printf("Loaded buffer: %s\n", buffer->GetName().c_str());
    }
    else
    {
        printf("Failed to load: %s\n", file);
    }

    // CRITICAL: Set display region so Zep knows actual window size and can layout properly
    // Without this, status bar shows at (1,1) because regions are never computed
    editor.SetDisplayRegion(NVec2f(0.0f, 0.0f), NVec2f((float)display.GetScreenWidth(), (float)display.GetScreenHeight()));
    editor.GetConfig().autoHideCommandRegion = false; // Always show status bar at bottom
    editor.GetConfig().showMinimap = true;
    editor.GetConfig().showIndicatorRegion = true; // Show git indicators
    editor.GetConfig().shortTabNames = true; // Short tab names
    editor.GetConfig().tabToneColors = true; // Tab tone colors
    editor.GetConfig().cursorLineSolid = true; // Solid cursor line
    editor.GetConfig().searchGitRoot = true; // Search for git root

    // Load pzeprc configuration (system then user) to allow customization
    auto configPath = editor.GetFileSystem().GetConfigPath();
    editor.LoadPZepRC(configPath / "pzeprc");
    editor.LoadPZepRC(configPath / "_pzeprc");
    editor.LoadPZepRC(configPath / ".pzeprc");

    while (true)
    {
        // Check if ESC is being pressed - prevent window close
        if (IsKeyDown(KEY_ESCAPE))
        {
            // ESC is down - don't let ShouldClose return true
            // We'll check ShouldClose AFTER handling ESC
        }

        bool shouldClose = display.ShouldClose();
        // Only actually close if not caused by ESC (and not ESC being held)
        if (shouldClose && !IsKeyDown(KEY_ESCAPE))
        {
            // Check for unsaved changes before quitting
            ZepBuffer* pBuffer = editor.GetActiveBuffer();
            if (pBuffer && pBuffer->HasFileFlags(FileFlags::Dirty))
            {
                // Prevent closing without saving, mimic :q behavior
                editor.SetCommandText("E37: No write since last change (add ! to override)");
                // Do not break; stay in loop
            }
            else
            {
                break;
            }
        }

        // Check if window was resized and update Zep display region
        if (IsWindowResized())
        {
            editor.SetDisplayRegion(NVec2f(0.0f, 0.0f), NVec2f((float)GetScreenWidth(), (float)GetScreenHeight()));
        }

        // Note: Font resize disabled - causes blocky rendering.
        // Font stays crisp at default size only.
        // Ctrl+MouseWheel and Ctrl++/- resize removed for quality.

        // Handle font size with Ctrl+MouseWheel
        float mouseWheel = GetMouseWheelMove();
        if (mouseWheel != 0 && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            auto& font = display.GetFont(ZepTextType::Text);
            int newSize = font.GetPixelHeight() + (int)(mouseWheel * 2);
            if (newSize < 8)
                newSize = 8;
            if (newSize > 72)
                newSize = 72;
            font.SetPixelHeight(newSize);
        }

        // Handle Ctrl++ and Ctrl+- for font size
        if (IsKeyPressed(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            auto& font = display.GetFont(ZepTextType::Text);
            int newSize = font.GetPixelHeight() + 2;
            if (newSize <= 72)
            {
                font.SetPixelHeight(newSize);
            }
        }
        if (IsKeyPressed(KEY_MINUS) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            auto& font = display.GetFont(ZepTextType::Text);
            int newSize = font.GetPixelHeight() - 2;
            if (newSize >= 8)
            {
                font.SetPixelHeight(newSize);
            }
        }

        // Handle Ctrl++ and Ctrl+- for font size
        if (IsKeyPressed(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            auto& font = display.GetFont(ZepTextType::Text);
            int newSize = font.GetPixelHeight() + 2;
            if (newSize <= 72)
            {
                font.SetPixelHeight(newSize);
            }
        }
        if (IsKeyPressed(KEY_MINUS) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            auto& font = display.GetFont(ZepTextType::Text);
            int newSize = font.GetPixelHeight() - 2;
            if (newSize >= 8)
            {
                font.SetPixelHeight(newSize);
            }
        }

        display.BeginFrame();

        // Handle welcome screen dismissal (if enabled)
        if (editor.GetConfig().showWelcomeScreen)
        {
            // Dismiss on any keystroke
            int ch = GetCharPressed();
            int key = GetKeyPressed();
            if (ch > 0 || key > 0)
            {
                editor.GetConfig().showWelcomeScreen = false;
            }
        }

        // Handle Ctrl+Q to quit (alternative to :q)
        if (IsKeyPressed(KEY_Q) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            ZepBuffer* pBuffer = editor.GetActiveBuffer();
            if (pBuffer && pBuffer->HasFileFlags(FileFlags::Dirty))
            {
                editor.SetCommandText("No write since last change (add ! to override or save)");
            }
            else
            {
                break;
            }
        }

        // Let Ex command handling take care of :q via the command system
        // (no separate check here)

        // Skip ShouldClose - let only explicit :q or Ctrl+Q quit
        // Never close based on ShouldClose since ESC is disabled at raylib level

        ZepBuffer* buf = editor.GetActiveBuffer();
        if (buf)
        {
            ZepMode* mode = buf->GetMode();
            if (mode)
            {
                int mod = GetModifiers();

                int ch = GetCharPressed();
                if (ch > 0)
                {
                    mode->AddKeyPress(ch, mod);
                }

                int key = GetKeyPressed();
                if (key > 0)
                {
                    // Skip ASCII characters - those go through GetCharPressed()
                    if (key >= 32 && key <= 126)
                    {
                        // ASCII printable - ignore, handled by GetCharPressed()
                    }
                    else if (key > 0)
                    {
                        // ESC (key 256) should return to normal mode, not close window
                        if (key == 256 || IsKeyDown(KEY_ESCAPE))
                        {
                            mode->AddKeyPress(1, mod); // ESCAPE - return to normal mode
                        }
                        // F11 (300) - toggle fullscreen
                        else if (key == 300)
                        {
                            display.ToggleFullscreen();
                        }
                        // F12 (301) - toggle minimap
                        else if (key == 301)
                        {
                            editor.GetConfig().showMinimap = !editor.GetConfig().showMinimap;
                            editor.GetDisplay().SetLayoutDirty(true);
                            editor.SetCommandText(editor.GetConfig().showMinimap ? "minimap on" : "nominimap");
                        }
                        else
                        {
                            auto it = g_keyMap.find(key);
                            if (it != g_keyMap.end())
                            {
                                mode->AddKeyPress(it->second, mod);
                            }
                            else
                            {
                                // Default: map unmapped keys to their ExtKeys value if < 32
                                if (key < 32)
                                    mode->AddKeyPress(key, mod);
                            }
                        }
                    }
                }
            }
        }

        // Increment keystroke counter for swap file auto-save
        editor.IncrementKeystrokeCounter();

        auto mp = display.GetMousePosition();
        editor.OnMouseMove({ mp.x, mp.y });
        for (int b = 0; b < 3; b++)
        {
            if (display.IsMouseButtonDown(b))
                editor.OnMouseDown({ mp.x, mp.y }, (ZepMouseButton)b);
            else
                editor.OnMouseUp({ mp.x, mp.y }, (ZepMouseButton)b);
        }

        editor.Display();
        display.EndFrame();

        // If all tab windows have been closed (e.g., via :q), exit the main loop
        if (editor.GetTabWindows().empty())
        {
            break;
        }
    }

    return 0;
}
