#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <raylib.h>

#include "zep/editor.h"
#include "zep/mcommon/math/math.h"
#include "zep/mode_standard.h"
#include "zep/mode_vim.h"
#include "zep/raylib/display_raylib.h"
#include "zep/theme.h"

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 800;

void PrintWelcome()
{
    std::cout << "=== pZep-GUI v0.1.0 ===" << std::endl;
    std::cout << "A Vim-like editor powered by Raylib" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  ESC       - Return to normal mode" << std::endl;
    std::cout << "  i         - Enter insert mode" << std::endl;
    std::cout << "  :         - Enter ex command" << std::endl;
    std::cout << "  h/j/k/l   - Navigation (normal mode)" << std::endl;
    std::cout << std::endl;
    std::cout << "Ex commands:" << std::endl;
    std::cout << "  :w        - Save file" << std::endl;
    std::cout << "  :q        - Quit" << std::endl;
    std::cout << "  :e <file> - Open file" << std::endl;
    std::cout << "  :bn       - Next buffer" << std::endl;
    std::cout << "  :bp       - Previous buffer" << std::endl;
    std::cout << "  :set      - Toggle options" << std::endl;
}

long MapKey(int raylibKey)
{
    switch (raylibKey)
    {
    case KEY_NULL:
        return 0;
    case KEY_A:
        return 'a';
    case KEY_B:
        return 'b';
    case KEY_C:
        return 'c';
    case KEY_D:
        return 'd';
    case KEY_E:
        return 'e';
    case KEY_F:
        return 'f';
    case KEY_G:
        return 'g';
    case KEY_H:
        return 'h';
    case KEY_I:
        return 'i';
    case KEY_J:
        return 'j';
    case KEY_K:
        return 'k';
    case KEY_L:
        return 'l';
    case KEY_M:
        return 'm';
    case KEY_N:
        return 'n';
    case KEY_O:
        return 'o';
    case KEY_P:
        return 'p';
    case KEY_Q:
        return 'q';
    case KEY_R:
        return 'r';
    case KEY_S:
        return 's';
    case KEY_T:
        return 't';
    case KEY_U:
        return 'u';
    case KEY_V:
        return 'v';
    case KEY_W:
        return 'w';
    case KEY_X:
        return 'x';
    case KEY_Y:
        return 'y';
    case KEY_Z:
        return 'z';
    case KEY_APOSTROPHE:
        return '\'';
    case KEY_COMMA:
        return ',';
    case KEY_MINUS:
        return '-';
    case KEY_PERIOD:
        return '.';
    case KEY_SLASH:
        return '/';
    case KEY_ZERO:
        return '0';
    case KEY_ONE:
        return '1';
    case KEY_TWO:
        return '2';
    case KEY_THREE:
        return '3';
    case KEY_FOUR:
        return '4';
    case KEY_FIVE:
        return '5';
    case KEY_SIX:
        return '6';
    case KEY_SEVEN:
        return '7';
    case KEY_EIGHT:
        return '8';
    case KEY_NINE:
        return '9';
    case KEY_SEMICOLON:
        return ';';
    case KEY_EQUAL:
        return '=';
    case KEY_LEFT_BRACKET:
        return '[';
    case KEY_BACKSLASH:
        return '\\';
    case KEY_RIGHT_BRACKET:
        return ']';
    case KEY_GRAVE:
        return '`';
    case KEY_SPACE:
        return ' ';
    case KEY_ESCAPE:
        return Zep::ExtKeys::ESCAPE;
    case KEY_ENTER:
        return Zep::ExtKeys::RETURN;
    case KEY_TAB:
        return Zep::ExtKeys::TAB;
    case KEY_BACKSPACE:
        return Zep::ExtKeys::BACKSPACE;
    case KEY_UP:
        return Zep::ExtKeys::UP;
    case KEY_DOWN:
        return Zep::ExtKeys::DOWN;
    case KEY_LEFT:
        return Zep::ExtKeys::LEFT;
    case KEY_RIGHT:
        return Zep::ExtKeys::RIGHT;
    case KEY_F1:
        return Zep::ExtKeys::F1;
    case KEY_F2:
        return Zep::ExtKeys::F2;
    case KEY_F3:
        return Zep::ExtKeys::F3;
    case KEY_F4:
        return Zep::ExtKeys::F4;
    case KEY_F5:
        return Zep::ExtKeys::F5;
    case KEY_F6:
        return Zep::ExtKeys::F6;
    case KEY_F7:
        return Zep::ExtKeys::F7;
    case KEY_F8:
        return Zep::ExtKeys::F8;
    case KEY_F9:
        return Zep::ExtKeys::F9;
    case KEY_F10:
        return Zep::ExtKeys::F10;
    case KEY_F11:
        return Zep::ExtKeys::F11;
    case KEY_F12:
        return Zep::ExtKeys::F12;
    case KEY_HOME:
        return Zep::ExtKeys::HOME;
    case KEY_END:
        return Zep::ExtKeys::END;
    case KEY_PAGE_UP:
        return Zep::ExtKeys::PAGEUP;
    case KEY_PAGE_DOWN:
        return Zep::ExtKeys::PAGEDOWN;
    case KEY_INSERT:
        return 'i';
    case KEY_DELETE:
        return Zep::ExtKeys::DEL;
    case KEY_KP_0:
        return '0';
    case KEY_KP_1:
        return '1';
    case KEY_KP_2:
        return '2';
    case KEY_KP_3:
        return '3';
    case KEY_KP_4:
        return '4';
    case KEY_KP_5:
        return '5';
    case KEY_KP_6:
        return '6';
    case KEY_KP_7:
        return '7';
    case KEY_KP_8:
        return '8';
    case KEY_KP_9:
        return '9';
    case KEY_KP_DECIMAL:
        return '.';
    case KEY_KP_DIVIDE:
        return '/';
    default:
        return 0;
    }
}

int main(int argc, char* argv[])
{
    fprintf(stderr, "DEBUG: main start\n");
    fflush(stderr);

    try
    {
        fprintf(stderr, "DEBUG: after try\n");
        fflush(stderr);

        PrintWelcome();
        fprintf(stderr, "DEBUG: after PrintWelcome\n");
        fflush(stderr);

        // Create Raylib display
        fprintf(stderr, "DEBUG: creating display\n");
        fflush(stderr);
        Zep::ZepDisplay_Raylib display(SCREEN_WIDTH, SCREEN_HEIGHT);
        fprintf(stderr, "DEBUG: after display\n");
        fflush(stderr);

        // Create the Zep editor
        fprintf(stderr, "DEBUG: creating editor\n");
        fflush(stderr);
        Zep::ZepEditor editor(&display, ".");
        fprintf(stderr, "DEBUG: after editor\n");
        fflush(stderr);

        // Register Vim mode
        printf("Registering Vim mode...\n");
        fflush(stdout);
        editor.RegisterGlobalMode(std::make_shared<Zep::ZepMode_Vim>(editor));
        printf("Registered Vim mode\n");
        fflush(stdout);

        // Register Standard mode as well
        editor.RegisterGlobalMode(std::make_shared<Zep::ZepMode_Standard>(editor));

        // Initialize buffer FIRST, before setting global mode
        printf("About to InitWithText...\n");
        fflush(stdout);
        auto pBuf = editor.InitWithText("untitled", "");
        printf("After InitWithText, buffer: %p\n", pBuf);
        fflush(stdout);

        printf("  Buffers: %zu\n", editor.GetBuffers().size());
        fflush(stdout);

        printf("About to UpdateWindowState...\n");
        fflush(stdout);
        editor.UpdateWindowState();
        printf("After UpdateWindowState\n");
        fflush(stdout);

        // Set the display region to match the window size
        printf("About to SetDisplayRegion...\n");
        fflush(stdout);
        editor.SetDisplayRegion(Zep::NVec2f(0.0f, 0.0f), Zep::NVec2f(SCREEN_WIDTH, SCREEN_HEIGHT));
        printf("After SetDisplayRegion\n");
        fflush(stdout);

        // THEN set global mode - now there's a window
        printf("About to SetGlobalMode(Vim)...\n");
        fflush(stdout);
        editor.SetGlobalMode("Vim");
        printf("After SetGlobalMode(Vim)\n");
        fflush(stdout);

        // Debug: check current mode
        auto* pMode = editor.GetGlobalMode();
        printf("Global mode pointer: %p\n", pMode);
        if (pMode)
        {
            printf("Global mode name: %s\n", pMode->Name());
        }
        fflush(stdout);

        // Begin the mode on the active window
        printf("About to GetActiveWindow...\n");
        fflush(stdout);
        Zep::ZepWindow* pWin = editor.GetActiveWindow();
        printf("Got window: %p\n", pWin);
        fflush(stdout);

        if (pWin)
        {
            printf("About to Begin...\n");
            fflush(stdout);
            editor.GetGlobalMode()->Begin(pWin);
            printf("After Begin\n");
            fflush(stdout);
        }
        else
        {
            printf("ERROR: Still no window!\n");
            fflush(stdout);
        }

        printf("Main loop starting...\n");
        fflush(stdout);
        std::cout << "Editor initialized. Window should appear." << std::endl;
        std::cout << "Press ESC then q to quit, or :q in the editor." << std::endl;
        std::cout << "If window doesn't appear, there may be a display rendering issue." << std::endl;
        std::cout.flush();

        // Main loop - run until window is closed
        int frameCount = 0;
        while (!display.ShouldClose())
        {
            display.BeginFrame();

            // Handle input via ZepEditor API
            auto pBuffer = editor.GetActiveBuffer();
            if (pBuffer)
            {
                auto pMode = pBuffer->GetMode();

                // Check for character input (text)
                int codepoint = display.GetCharPressed();
                while (codepoint != 0)
                {
                    pMode->AddKeyPress(codepoint, 0);
                    codepoint = display.GetCharPressed();
                }

                // Check for special key input
                int key = display.GetKeyPressed();
                while (key != 0)
                {
                    long zepKey = MapKey(key);
                    // MapKey returns 0 for RETURN (ExtKeys::RETURN = 0), so we need to handle that
                    if (zepKey != 0 || key == KEY_ENTER)
                    {
                        pMode->AddKeyPress(zepKey, 0);
                    }
                    key = display.GetKeyPressed();
                }
            }

            // Render the editor
            editor.Display();

            display.EndFrame();
            frameCount++;
        }

        std::cout << "Rendered " << frameCount << " frames, exiting." << std::endl;
        std::cout << "Goodbye!" << std::endl;
    }
    catch (const std::exception& e)
    {
        printf("EXCEPTION: %s\n", e.what());
    }
    catch (...)
    {
        printf("UNKNOWN EXCEPTION\n");
    }
    return 0;
}