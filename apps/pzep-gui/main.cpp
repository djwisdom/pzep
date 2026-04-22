#include "zep/editor.h"
#include "zep/mode_vim.h"
#include "zep/raylib/display_raylib.h"

#include <cstdio>
#include <string>
#include <unordered_map>

using namespace Zep;

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
} // namespace

int main(int argc, char* argv[])
{
    const char* file = (argc > 1) ? argv[1] : "untitled";

    ZepDisplay_Raylib display(1280, 800);
    ZepEditor editor(&display, std::filesystem::current_path());
    editor.SetGlobalMode(ZepMode_Vim::StaticName());

    // CRITICAL: Set display region so Zep knows actual window size and can layout properly
    // Without this, status bar shows at (1,1) because regions are never computed
    editor.SetDisplayRegion(NVec2f(0.0f, 0.0f), NVec2f((float)display.GetScreenWidth(), (float)display.GetScreenHeight()));
    editor.GetConfig().autoHideCommandRegion = false; // Always show status bar at bottom
    editor.GetConfig().showLineNumbers = true;
    // Disable relative line numbers - use absolute/normal numbering
    // by calling SetUseRelativeLineNumbers(false) on vim mode
    {
        // Get vim mode and disable relative lines
        auto* vimMode = dynamic_cast<ZepMode_Vim*>(editor.GetGlobalMode());
        if (vimMode)
        {
            vimMode->SetUseRelativeLineNumbers(false);
        }
    }

    ZepBuffer* buffer = editor.InitWithText(file, "");

    fprintf(stderr, "=== pZep-GUI v0.1.0 ===\n");
    fprintf(stderr, "Click in window, press 'i' to insert, ESC normal, :q quit\n");
    fprintf(stderr, "Display Initialized: %dx%d\n", 1280, 800);
    fprintf(stderr, "Font pixel height: %d\n", display.GetFont(ZepTextType::Text).GetPixelHeight());
    fflush(stderr);

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
            break;
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

        // Handle Ctrl+Q to quit (alternative to :q)
        if (IsKeyPressed(KEY_Q) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)))
        {
            break;
        }

        // Check after BeginFrame - but don't close if ESC was just pressed or in insert mode
        if (display.ShouldClose() && !IsKeyDown(KEY_ESCAPE))
        {
            // Only close if not ESC and not in insert mode
            Zep::EditorMode currentMode = Zep::EditorMode::None;
            if (auto* b = editor.GetActiveBuffer())
                if (auto* m = b->GetMode())
                    currentMode = m->GetEditorMode();
            if (currentMode != Zep::EditorMode::Insert)
            {
                fprintf(stderr, "DEBUG: ShouldClose true AFTER BeginFrame, closing\n");
                break;
            }
        }

        ZepBuffer* buf = editor.GetActiveBuffer();
        if (buf)
        {
            ZepMode* mode = buf->GetMode();
            if (mode)
            {
                static int lastMode = -1;
                int currMode = (int)mode->GetEditorMode();
                if (currMode != lastMode)
                {
                    fprintf(stderr, "MODE CHANGE: %d\n", currMode);
                    lastMode = currMode;
                    fflush(stderr);
                }

                int mod = GetModifiers();

                int ch = GetCharPressed();
                if (ch > 0)
                {
                    fprintf(stderr, "KEY CHAR: '%c' (%d)\n", (char)ch, ch);
                    fflush(stderr);
                    mode->AddKeyPress(ch, mod);
                }

                int key = GetKeyPressed();
                if (key > 0)
                {
                    fprintf(stderr, "DEBUG: GetKeyPressed=%d, ShouldClose=%d\n", key, display.ShouldClose() ? 1 : 0);
                    fflush(stderr);

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
                            // Don't check ShouldClose immediately - let Zep process the key first
                        }
                        // F11 (292) - toggle fullscreen
                        else if (key == 292)
                        {
                            ToggleFullscreen();
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

        auto mp = display.GetMousePosition();
        editor.OnMouseMove({ mp.x, mp.y });
        for (int b = 0; b < 3; b++)
        {
            if (display.IsMouseButtonDown(b))
                editor.OnMouseDown({ mp.x, mp.y }, (ZepMouseButton)b);
            else
                editor.OnMouseUp({ mp.x, mp.y }, (ZepMouseButton)b);
        }

        display.ClearBackground({ 0.1f, 0.1f, 0.1f, 1.0f });
        editor.Display();
        display.EndFrame();

        // Check ShouldClose AFTER EndFrame
        if (display.ShouldClose())
        {
            fprintf(stderr, "DEBUG: ShouldClose became true AFTER EndFrame\n");
            fflush(stderr);
            break;
        }
    }

    return 0;
}