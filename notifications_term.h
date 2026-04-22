#pragma once
// Terminal Implementation of Developer Notifications
// Works on: Windows, Linux, FreeBSD

#include "notifications.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace ZepNotifications
{

// Terminal colors (ANSI escape codes)
struct TerminalStyle
{
    // Reset
    const char* reset = "\033[0m";

    // Severity colors
    const char* critical = "\033[1;31m"; // Bold red
    const char* high = "\033[38;5;208m"; // Orange
    const char* medium = "\033[38;5;226m"; // Yellow
    const char* low = "\033[38;5;39m"; // Blue
    const char* info = "\033[90m"; // Gray
    const char* success = "\033[32m"; // Green

    // Styles
    const char* bold = "\033[1m";
    const char* dim = "\033[2m";
    const char* underline = "\033[4m";

    // Cursor
    const char* clear = "\033[2J";
    const char* home = "\033[H";
    const char* clear_eol = "\033[K";

    // Platform detection
    static const char* GetPlatformStyle()
    {
#ifdef _WIN32
        return "Windows";
#elif defined(__linux__)
        return "Linux";
#elif defined(__FreeBSD__)
        return "FreeBSD";
#else
        return "Unknown";
#endif
    }
};

class TerminalNotificationRenderer
{
public:
    TerminalStyle style;
    bool initialized = false;

    // Detect terminal capabilities
    static bool HasANSI()
    {
#ifdef _WIN32
        // Check if Windows 10+ console
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetStdHandle(STD_OUTPUT_HANDLE) == INVALID_HANDLE_VALUE)
            return false;
        if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
            return false;
        return true;
#else
        // Check TERM environment variable
        const char* term = getenv("TERM");
        if (term && std::string(term).find("256color") != std::string::npos)
            return true;
        if (term && std::string(term).find("color") != std::string::npos)
            return true;
        return isatty(STDOUT_FILENO);
#endif
    }

    // Get terminal size
    static void GetTermSize(int& width, int& height)
    {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
        {
            width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
        else
        {
            width = 80;
            height = 24;
        }
#else
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        {
            width = w.ws_col;
            height = w.ws_row;
        }
        else
        {
            width = 80;
            height = 24;
        }
#endif
    }

    // Get color by severity
    const char* GetSeverityColor(NotificationSeverity severity) const
    {
        switch (severity)
        {
        case NotificationSeverity::Critical:
            return style.critical;
        case NotificationSeverity::High:
            return style.high;
        case NotificationSeverity::Medium:
            return style.medium;
        case NotificationSeverity::Low:
            return style.low;
        case NotificationSeverity::Info:
            return style.info;
        }
        return style.info;
    }

    // Get icon by severity
    const char* GetSeverityIcon(NotificationSeverity severity) const
    {
        switch (severity)
        {
        case NotificationSeverity::Critical:
            return "✗";
        case NotificationSeverity::High:
            return "⚠";
        case NotificationSeverity::Medium:
            return "▸";
        case NotificationSeverity::Low:
            return "ℹ";
        case NotificationSeverity::Info:
            return "•";
        }
        return "•";
    }

    // Render single notification line
    void RenderLine(const Notification& n, int lineNum = 0)
    {
        const char* color = GetSeverityColor(n.severity);
        const char* icon = GetSeverityIcon(n.severity);

        std::cout << color << icon << " " << style.reset;

        // Format based on type
        std::string text = FormatOneLine(n);
        std::cout << text << "\n";

        // If critical, also render context
        if (n.severity == NotificationSeverity::Critical)
        {
            if (!n.error_location.empty())
            {
                std::cout << style.dim << "  → " << n.error_location;
            }
            if (!n.link.empty())
            {
                std::cout << " | " << n.link;
            }
            std::cout << style.reset << "\n";
        }
    }

    // Render full panel (ncurses-style)
    void RenderPanel(const NotificationManager& manager,
        int width, int height,
        int maxItems = 20)
    {
        // Header
        std::cout << style.clear << style.home;
        std::cout << style.bold << "╔══════════════════════════════════════════════╗\n";
        std::cout << "║ NOTIFICATIONS                           ║\n";
        std::cout << "╚══════════════════════════════════════════════╝"
                  << style.reset << "\n";

        // Filters
        std::cout << style.dim << "[A]ll [C]ritical [B]uild [T]est [R]untime | "
                  << "Platform: " << TerminalStyle::GetPlatformStyle() << style.reset << "\n";

        // Notifications
        int count = 0;
        for (const auto& n : manager.notifications)
        {
            if (count++ >= maxItems)
                break;
            std::cout << style.dim << count << ". " << style.reset;
            RenderLine(n);
        }

        // Fill remaining lines
        for (int i = count; i < maxItems; i++)
        {
            std::cout << "\n";
        }

        // Status bar
        std::cout << style.bold << "────────────────────────────────────────────\n";
        auto critical = manager.GetCritical();
        if (critical.size() > 0)
        {
            std::cout << style.critical << "⚠ " << critical.size()
                      << " CRITICAL notifications" << style.reset << " | ";
        }
        else
        {
            std::cout << style.success << "✓ OK" << style.reset << " | ";
        }
        std::cout << manager.Count() << " total | ";
        std::cout << TerminalStyle::GetPlatformStyle();
        std::cout << "\n";

        // Command hint
        std::cout << style.dim << "[Enter] open [x] dismiss [q] quit"
                  << style.reset << "\n";
    }

    // Render toast (non-blocking popup)
    void RenderToast(const Notification& n, float seconds = 3.0f)
    {
        if (n.severity != NotificationSeverity::Critical && n.severity != NotificationSeverity::High)
        {
            return;
        }

        // Save cursor position
        std::cout << "\033[s"; // ANSI save

        // Position at top-right
        std::cout << "\033[0;0H";

        // Draw box
        std::cout << style.bold << GetSeverityColor(n.severity);
        std::cout << "┌────────────────────────────────────────┐\n";
        std::cout << "│ " << GetSeverityIcon(n.severity) << " "
                  << SeverityToString(n.severity) << " "
                  << TypeToString(n.type) << " │\n";
        std::cout << "│ " << n.summary.substr(0, 36) << "\n";
        std::cout << "│ " << n.project << " | " << n.target << "   │\n";
        std::cout << "└────────────────────────────────────────┘"
                  << style.reset << "\n";

        std::cout << "\033[u"; // ANSI restore

        // Brief display
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(seconds * 1000)));
    }

    // Clear screen
    void Clear()
    {
        std::cout << style.clear << style.home;
    }

    // One-line format
    std::string FormatOneLine(const Notification& n) const
    {
        std::string out;

        // Type prefix
        out += "[" + TypeToString(n.type) + "] ";

        // Summary
        out += n.summary;

        // Context
        if (!n.error_location.empty())
        {
            out += " @" + n.error_location;
        }

        return out;
    }

    // Input helper (non-blocking on some platforms)
    static bool HasInput()
    {
#ifdef _WIN32
        return _kbhit() != 0;
#else
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval tv = { 0, 0 };
        return select(1, &readfds, nullptr, nullptr, &tv) > 0;
#endif
    }

    static char ReadChar()
    {
#ifdef _WIN32
        return _getch();
#else
        char c;
        read(STDIN_FILENO, &c, 1);
        return c;
#endif
    }
};

// Terminal app wrapper
class TerminalNotificationApp
{
public:
    TerminalNotificationRenderer renderer;
    NotificationManager manager;
    bool running = false;

    void Init()
    {
        if (!TerminalNotificationRenderer::HasANSI())
        {
            // Fall back to plain output
            std::cout << "Warning: ANSI not supported, using plain output\n";
        }
        running = true;

        // Add sample notifications
        manager.Add(BuildFailed(
            "core-lib", "all", "undefined reference",
            "src/util.cpp:128", "1234", "http://log")
                .Build());
        manager.Add(TestFailed(
            "auth-suite", "test_login: expected 200", "http://test")
                .Build());
    }

    void Run()
    {
        int width, height;

        while (running)
        {
            TerminalNotificationRenderer::GetTermSize(width, height);

            renderer.RenderPanel(manager, width, height);

            // Input handling
            if (TerminalNotificationRenderer::HasInput())
            {
                char c = TerminalNotificationRenderer::ReadChar();

                switch (c)
                {
                case 'q':
                case 'Q':
                    running = false;
                    break;
                case 'c':
                case 'C':
                    // Show critical only
                    renderer.Clear();
                    for (const auto& n : manager.GetCritical())
                    {
                        renderer.RenderLine(n);
                    }
                    std::cout << "\nPress any key...";
                    TerminalNotificationRenderer::ReadChar();
                    break;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

} // namespace ZepNotifications

// Platform-specific implementations
#ifdef _WIN32
// Windows uses conio.h via <windows.h>
#define KBHIT _kbhit
#define GETCH _getch
#else
// POSIX uses termios
static inline int kbhit()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif