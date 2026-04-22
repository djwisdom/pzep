#pragma once
// ImGui Implementation of Developer Notifications
// Works on: Windows, Linux, FreeBSD (via SDL2 + OpenGL)

#include "notifications.h"
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#ifdef __FreeBSD__
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace ZepNotifications
{

// ImGui colors for notifications
struct ImGuiNotificationStyle
{
    ImVec4 critical = ImVec4(0.9f, 0.2f, 0.2f, 1.0f); // Red
    ImVec4 high = ImVec4(0.9f, 0.5f, 0.2f, 1.0f); // Orange
    ImVec4 medium = ImVec4(0.9f, 0.8f, 0.2f, 1.0f); // Yellow
    ImVec4 low = ImVec4(0.3f, 0.5f, 0.9f, 1.0f); // Blue
    ImVec4 info = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    ImVec4 success = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); // Green

    ImVec4 background = ImVec4(0.1f, 0.1f, 0.1f, 0.95f);
    ImVec4 border = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    ImVec4 text = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    ImVec4 text_dim = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
};

class ImGuiNotificationRenderer
{
public:
    ImGuiNotificationStyle style;
    bool initialized = false;

    // Platform detection
    static const char* GetPlatform()
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

    // Get screen size (platform-specific)
    static void GetScreenSize(int& width, int& height)
    {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#elif defined(__linux__) || defined(__FreeBSD__)
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        width = w.ws_col;
        height = w.ws_row;
#else
        width = 80;
        height = 24;
#endif
    }

    ImVec4 GetSeverityColor(NotificationSeverity severity) const
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

    // Render notification toast (popup for critical items)
    void RenderToast(const Notification& n, float duration_sec = 5.0f)
    {
        // Only toast for critical/high severity
        if (n.severity != NotificationSeverity::Critical && n.severity != NotificationSeverity::High)
        {
            return;
        }

        // ImGui toast window - top right corner
        float toastWidth = 400.0f;
        float toastHeight = 80.0f;

        ImGui::SetNextWindowPos(ImVec2(
            ImGui::GetIO().DisplaySize.x - toastWidth - 20.0f,
            20.0f));
        ImGui::SetNextWindowSize(ImVec2(toastWidth, toastHeight));

        ImGui::Begin("Notification##toast", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImVec4 color = GetSeverityColor(n.severity);

        // Severity badge
        ImGui::TextColored(color, "[%s]", SeverityToString(n.severity).c_str());
        ImGui::SameLine();

        // Type badge
        ImGui::TextColored(style.text_dim, "[%s]", TypeToString(n.type).c_str());

        // Summary
        ImGui::TextWrapped(n.summary.c_str());

        // Context line
        if (!n.project.empty() || !n.target.empty())
        {
            ImGui::Separator();
            ImGui::TextColored(style.text_dim, "%s | %s",
                n.project.c_str(), n.target.c_str());
        }

        // Action button
        if (!n.action.empty())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton(n.action.c_str()))
            {
                // Open link/action - would integrate with system
                // OpenURL(n.link);
            }
        }

        ImGui::End();
    }

    // Render notification list panel
    void RenderPanel(const NotificationManager& manager,
        float x, float y, float width, float height,
        int maxItems = 50)
    {
        ImGui::SetNextWindowPos(ImVec2(x, y));
        ImGui::SetNextWindowSize(ImVec2(width, height));

        ImGui::Begin("Notifications##panel", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        // Filter buttons
        if (ImGui::Button("All"))
        {
            // Show all
        }
        ImGui::SameLine();
        if (ImGui::Button("Critical"))
        {
            // Show critical only
        }
        ImGui::SameLine();
        if (ImGui::Button("Builds"))
        {
            // Show builds
        }
        ImGui::SameLine();
        if (ImGui::Button("Tests"))
        {
            // Show tests
        }

        ImGui::Separator();

        // Notification list (scrollable)
        ImGui::BeginChild("notifications_list", ImVec2(0, height - 100));

        int count = 0;
        for (const auto& n : manager.notifications)
        {
            if (count++ >= maxItems)
                break;

            ImVec4 color = GetSeverityColor(n.severity);

            // Severity indicator
            ImGui::TextColored(color, "●");
            ImGui::SameLine();

            // Summary
            std::string text = FormatOneLine(n);
            ImGui::TextWrapped(text.c_str());

            // Location (clickable file:line)
            if (n.file_path && n.file_line)
            {
                ImGui::SameLine();
                ImGui::TextColored(style.text_dim, " @ %s:%d",
                    n.file_path->c_str(), *n.file_line);

                if (ImGui::IsItemClicked())
                {
                    // Open file at line in Zep editor
                    // spEditor->OpenFileAt(*n.file_path, *n.file_line);
                }
            }

            // Timestamp
            ImGui::SameLine(style.text_dim, style.textDim);
            ImGui::TextColored(style.text_dim, " | %s", GetTimestamp(n).c_str());
        }

        ImGui::EndChild();

        // Status bar
        ImGui::Separator();
        ImGui::TextColored(style.text_dim,
            "%zu notifications | Platform: %s | %s",
            manager.Count(), GetPlatform(),
            manager.GetCritical().size() > 0 ? "⚠ CRITICAL" : "OK");

        ImGui::End();
    }

    // Format one-line version for list
    std::string FormatOneLine(const Notification& n) const
    {
        switch (n.type)
        {
        case NotificationType::BuildFailure:
            return FormatBuildFailure(n);
        case NotificationType::TestFailure:
            return FormatTestFailure(n);
        case NotificationType::RuntimeError:
            return FormatRuntimeError(n);
        case NotificationType::Deployment:
            return FormatDeployment(n);
        case NotificationType::SecurityAlert:
            return FormatSecurityAlert(n);
        default:
            return n.summary;
        }
    }

    // Get timestamp string
    std::string GetTimestamp(const Notification& n) const
    {
        auto now = std::chrono::system_clock::now();
        auto diff = now - n.timestamp;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();

        if (seconds < 60)
            return "just now";
        if (seconds < 3600)
            return std::to_string(seconds / 60) + "m ago";
        if (seconds < 86400)
            return std::to_string(seconds / 3600) + "h ago";
        return std::to_string(seconds / 86400) + "d ago";
    }
};

// Helper to init (call once at startup)
inline void ImGuiNotifications_Init()
{
    // ImGui::GetIO().Fonts->AddFontFromFileTTF(...);
}

} // namespace ZepNotifications

// ImGui function forward declarations (implement in your app)
namespace ImGui
{
struct ImVec2
{
    float x, y;
    ImVec2(float _x = 0, float _y = 0)
        : x(_x)
        , y(_y)
    {
    }
};
struct ImVec4
{
    float x, y, z, w;
    ImVec4(float _x = 0, float _y = 0, float _z = 0, float _w = 0)
        : x(_x)
        , y(_y)
        , z(_z)
        , w(_w)
    {
    }
};
struct ImGuiIO
{
    ImVec2 DisplaySize;
    ImFontAtlas* Fonts;
    // Simplified - full ImGuiIO is much larger
};
ImGuiIO& GetIO();
void NewFrame();
void Render();
bool Begin(const char* name, bool* p_open = nullptr, int flags = 0);
void End();
void BeginChild(const char* str_id, ImVec2 size = ImVec2(0, 0), bool border = false, int flags = 0);
void EndChild();
void Text(const char* fmt, ...);
void TextWrapped(const char* fmt, ...);
void TextColored(ImVec4 col, const char* fmt, ...);
void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);
void Separator();
bool Button(const char* label, ImVec2 size = ImVec2(0, 0));
bool SmallButton(const char* label);
bool IsItemClicked(int mouse_button = 0);
void SetNextWindowPos(ImVec2 pos, int cond = 0, ImVec2 pivot = ImVec2(0, 0));
void SetNextWindowSize(ImVec2 size, int cond = 0);
ImVec4 GetStyleColorVec4(int idx);
struct ImFontAtlas;
struct ImFont;
} // namespace ImGui

// Full ImGui implementation would include imgui.h
// This is a header for integration