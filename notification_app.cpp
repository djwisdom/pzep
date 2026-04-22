// Zep + ImGui Notification Center Integration
// Compile after building Zep library
// Link: Zep, ImGui, SDL2, OpenGL

#include "notifications.h"

#include <memory>
#include <string>
#include <vector>

// Forward declare SDL types (actual SDL2 headers included in build)
struct SDL_Window;
struct SDL_GLContext;

namespace ImGui
{
struct ImVec2;
struct ImVec4;
class ImGuiContext;
class ImFontAtlas;
} // namespace ImGui

// Zep Editor API
namespace Zep
{

class ZepEditor_ImGui;
class ZepBuffer;
class ZepTabWindow;
class ZepWindow;
class ZepDisplay_ImGui;

struct ZepFont_ImGui;

} // namespace Zep

// Using ZepNotifications::Notification instead of old NotificationItem

class NotificationApp
{
public:
    NotificationApp();
    ~NotificationApp();

    bool Init(int width, int height);
    void Render();
    void Shutdown();
    bool IsRunning() const;

    // Add notification to the feed
    void AddNotification(ZepNotifications::NotificationSeverity severity,
        const std::string& source,
        const std::string& message);

private:
    void RenderNotificationPanel(float width, float height);
    void RenderEditorPanel(float x, float y, float width, float height);
    void RenderStatusBar(float width);
    void RenderNotificationToast(const ZepNotifications::Notification& n);

    std::unique_ptr<Zep::ZepEditor_ImGui> spEditor;
    ZepNotifications::NotificationManager notificationManager;
    bool running = false;
};

// Implementation -------------------------------------------------------------

NotificationApp::NotificationApp() = default;
NotificationApp::~NotificationApp() = default;

bool NotificationApp::Init(int width, int height)
{
    (void)width;
    (void)height;

    // Create Zep editor instance
    // Config stored in ./zep_config directory
    std::string configPath = "./zep_config";
    spEditor = std::make_unique<Zep::ZepEditor_ImGui>(configPath);

    // Initialize with sample notification config
    spEditor->InitWithText("notifications.toml", R"(
# Notification Configuration
# Edit this file to customize your feeds

[feeds.github]
enabled = true
events = ["push", "pr", "workflow"]
channels = ["#dev"]

[feeds.jenkins]
url = "http://jenkins:8080"
jobs = ["main-build", "tests", "deploy"]
on_failure = "urgent"

[feeds.custom]
script = "python scripts/check_feeds.py"

[filters]
error = ["ERROR", "FAIL", "CRITICAL"]
warning = ["WARN", "deprecated"]
)");

    // Add sample developer notifications (serious, actionable)
    using namespace ZepNotifications;

    notificationManager.Add(BuildFailed(
        "core-lib", // project
        "all", // target
        "undefined reference", // error
        "src/util.cpp:128", // location
        "1234", // build ID
        "http://jenkins/build/1234" // log link
        )
            .Build());

    notificationManager.Add(TestFailed(
        "auth-suite",
        "test_login: expected 200 got 500",
        "http://jenkins/test/456")
            .Build());

    notificationManager.Add(RuntimeError(
        "auth-service",
        "NullPointerException",
        "AuthController.login()",
        "req-abc123",
        "http://trace/service/abc123")
            .Build());

    notificationManager.Add(DeployComplete(
        "staging",
        "v2.1.0",
        true, // success
        "http://deploy/staging/789")
            .Build());

    notificationManager.Add(SecurityAlert(
        "lib/utils.js",
        NotificationSeverity::High,
        "update lodash@>4.5.0",
        "http://gh/advisory/789")
            .Build());

    running = true;
    return true;
}

void NotificationApp::Render()
{
    // Main window dimensions (would come from SDL/ImGui in real app)
    float mainWidth = 1000.0f;
    float mainHeight = 700.0f;

    // Left: Notification panel (30% width)
    float notifWidth = mainWidth * 0.3f;
    RenderNotificationPanel(notifWidth, mainHeight - 50);

    // Right: Editor panel (remaining width)
    float editorX = notifWidth + 20;
    float editorWidth = mainWidth - notifWidth - 30;
    RenderEditorPanel(editorX, 10, editorWidth, mainHeight - 60);

    // Bottom: Status bar
    RenderStatusBar(mainWidth);
}

void NotificationApp::RenderNotificationPanel(float width, float height)
{
    using namespace ZepNotifications;

    // Pseudo-ImGui calls - replace with actual ImGui in your app
    (void)width;
    (void)height;

    // In real ImGui:
    // ImGui::SetNextWindowPos(ImVec2(10, 10));
    // ImGui::SetNextWindowSize(ImVec2(width, height));
    // ImGui::Begin("Notifications");

    for (const auto& n : notificationManager.notifications)
    {
        // Format each notification per the design spec
        std::string formatted;

        switch (n.type)
        {
        case NotificationType::BuildFailure:
            formatted = FormatBuildFailure(n);
            break;
        case NotificationType::TestFailure:
            formatted = FormatTestFailure(n);
            break;
        case NotificationType::RuntimeError:
            formatted = FormatRuntimeError(n);
            break;
        case NotificationType::Deployment:
            formatted = FormatDeployment(n);
            break;
        case NotificationType::SecurityAlert:
            formatted = FormatSecurityAlert(n);
            break;
        default:
            formatted = n.summary;
        }

        // Color by severity: Critical=Red, High=Orange, Medium=Yellow, Low=Blue
        // ImGui::TextColored(severityColor, formatted.c_str());
    }
    // ImGui::End();
}

void NotificationApp::RenderEditorPanel(float x, float y, float width, float height)
{
    // Zep editor rendering
    // In real ImGui integration:
    // ImGui::SetNextWindowPos(ImVec2(x, y));
    // ImGui::SetNextWindowSize(ImVec2(width, height));
    // ImGui::Begin("Editor");

    // The Zep editor window is rendered by Zep internally
    // when you call spEditor->Render() or similar

    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void NotificationApp::RenderStatusBar(float width)
{
    // Bottom status bar
    // ImGui::SetNextWindowPos(ImVec2(10, height - 40));
    // ImGui::SetNextWindowSize(ImVec2(width, 30));
    // ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoTitleBar);
    //
    // ImGui::Text("Zep Editor | Mode: %s | Buffer: notifications.toml",
    //     spEditor->GetActiveTabWindow()->GetActiveWindow()->GetMode()->GetName().c_str());
    //
    // ImGui::End();
    (void)width;
}

void NotificationApp::Shutdown()
{
    notificationManager.Clear();
    spEditor.reset();
}

bool NotificationApp::IsRunning() const
{
    return running;
}

void NotificationApp::AddNotification(ZepNotifications::NotificationSeverity severity,
    const std::string& source,
    const std::string& message)
{
    using namespace ZepNotifications;

    Notification n;
    n.summary = message;
    n.project = source;
    n.severity = severity;
    n.timestamp = std::chrono::system_clock::now();

    notificationManager.Add(n);
}

// Main entry point (pseudo-code for SDL2 + ImGui loop) --------------------------------

/*
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    // SDL2 + OpenGL + ImGui init (from your existing demo)
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    // ... setup window, GL context, ImGui ...

    NotificationApp app;
    app.Init(1024, 768);

    while (app.IsRunning()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) break;
        }

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render our app
        app.Render();

        ImGui::Render();
        // GL draw ...

        SDL_Delay(16);
    }

    app.Shutdown();
    // cleanup SDL, ImGui ...
    return 0;
}
*/

int main()
{
    // Quick test - just init and print status
    NotificationApp app;
    app.Init(1024, 768);

    // Print what's in the editor buffer
    auto* buffer = app.spEditor->GetActiveTabWindow()
                       ->GetActiveWindow()
                       ->GetBuffer();

    printf("Editor initialized with buffer: %s\n",
        buffer ? "OK" : "NULL");
    printf("Notifications loaded: %zu\n",
        app.notifications.size());

    app.Shutdown();
    return 0;
}