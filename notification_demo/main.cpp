// Notification Editor Demo
// Shows: Zep Editor + Notifications Panel in ImGui
// Platform: Windows, Linux, FreeBSD

#include <GL/gl.h>
#include <SDL.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include "zep/imgui/display_imgui.h"
#include "zep/imgui/editor_imgui.h"
#include "zep/mode_standard.h"
#include "zep/mode_vim.h"

#include "../notifications.h"

using namespace Zep;
using namespace ZepNotifications;

// ============================================================================
// APP STATE
// ============================================================================

struct App
{
    std::unique_ptr<ZepEditor_ImGui> spEditor;
    NotificationManager notifManager;
    ImGuiNotificationRenderer notifRenderer;
    bool running = true;
};

void InitNotifications(NotificationManager& mgr)
{
    // Sample notifications for demo
    mgr.Add(BuildFailed(
        "core-lib", "all", "undefined reference",
        "src/util.cpp:128", "1234", "http://jenkins/build/1234")
            .Build());

    mgr.Add(TestFailed(
        "auth-suite", "test_login: expected 200 got 500",
        "http://jenkins/test/456")
            .Build());

    mgr.Add(RuntimeError(
        "auth-service", "NullPointerException",
        "AuthController.login()", "req-abc123",
        "http://trace/service/abc123")
            .Build());

    mgr.Add(DeployComplete(
        "staging", "v2.1.0", true,
        "http://deploy/staging/789")
            .Build());

    mgr.Add(SecurityAlert(
        "lib/utils.js", NotificationSeverity::High,
        "update lodash@>4.5.0",
        "http://gh/advisory/789")
            .Build());
}

// ============================================================================
// SDL + ImGui SETUP
// ============================================================================

SDL_Window* CreateWindow(int width, int height)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    return SDL_CreateWindow(
        "Zep Notification Center",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
}

void InitImGui(SDL_Window* window, SDL_GLContext glContext)
{
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Style setup
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.ChildRounding = 4.0f;
}

void ShutdownImGui(SDL_GLContext ctx, SDL_Window* window)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// ============================================================================
// RENDER LOOP
// ============================================================================

void Render(App& app, int width, int height)
{
    // Clear
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Layout: 70% editor, 30% notifications
    int editorW = (int)(width * 0.7);
    int notifX = editorW;
    int notifW = width - notifX;

    // === LEFT: Zep Editor ===
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)editorW, (float)height));
    ImGui::Begin("Editor", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Render Zep editor
    auto* pTab = app.spEditor->GetActiveTabWindow();
    if (pTab)
    {
        for (auto* win : pTab->GetWindows())
        {
            if (win->IsActive())
            {
                auto* buf = win->GetBuffer();
                if (buf)
                {
                    // Show buffer content
                    const auto& text = buf->GetWorkingBuffer();
                    ImGui::TextUnformatted(text.begin(), text.end());
                }
            }
        }
    }
    ImGui::End();

    // === RIGHT: Notifications Panel ===
    ImGui::SetNextWindowPos(ImVec2((float)notifX, 0));
    ImGui::SetNextWindowSize(ImVec2((float)notifW, (float)height));
    ImGui::Begin("Notifications", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    // Filter buttons
    if (ImGui::Button("All"))
    {
    }
    ImGui::SameLine();
    if (ImGui::Button("Critical"))
    {
    }
    ImGui::SameLine();
    if (ImGui::Button("Builds"))
    {
    }
    ImGui::SameLine();
    if (ImGui::Button("Tests"))
    {
    }

    ImGui::Separator();

    // Notification list
    for (const auto& n : app.notifManager.notifications)
    {
        ImVec4 color;
        switch (n.severity)
        {
        case NotificationSeverity::Critical:
            color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            break;
        case NotificationSeverity::High:
            color = ImVec4(0.9f, 0.5f, 0.2f, 1.0f);
            break;
        case NotificationSeverity::Medium:
            color = ImVec4(0.9f, 0.8f, 0.2f, 1.0f);
            break;
        case NotificationSeverity::Low:
            color = ImVec4(0.3f, 0.5f, 0.9f, 1.0f);
            break;
        default:
            color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        }

        // Severity icon
        const char* icon = (n.severity == NotificationSeverity::Critical) ? "✗" : (n.severity == NotificationSeverity::High) ? "⚠"
                                                                                                                             : "▸";

        ImGui::TextColored(color, "%s", icon);
        ImGui::SameLine();

        // Format notification
        std::string text;
        switch (n.type)
        {
        case NotificationType::BuildFailure:
            text = FormatBuildFailure(n);
            break;
        case NotificationType::TestFailure:
            text = FormatTestFailure(n);
            break;
        case NotificationType::RuntimeError:
            text = FormatRuntimeError(n);
            break;
        case NotificationType::Deployment:
            text = FormatDeployment(n);
            break;
        case NotificationType::SecurityAlert:
            text = FormatSecurityAlert(n);
            break;
        default:
            text = n.summary;
        }

        ImGui::TextWrapped(text.c_str());

        // Clickable file:line
        if (n.file_path && n.file_line)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " @ %s:%d",
                n.file_path->c_str(), *n.file_line);

            if (ImGui::IsItemClicked())
            {
                // TODO: Open in Zep
                // app.spEditor->OpenFileAt(*n.file_path, *n.file_line);
            }
        }
    }

    ImGui::End();

    // === BOTTOM: Status Bar ===
    ImGui::SetNextWindowPos(ImVec2(0, (float)(height - 30)));
    ImGui::SetNextWindowSize(ImVec2((float)width, 30));
    ImGui::Begin("Status", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    auto critical = app.notifManager.GetCritical();
    if (critical.size() > 0)
    {
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
            "⚠ %zu CRITICAL", critical.size());
    }
    else
    {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "✓ OK");
    }
    ImGui::SameLine();
    ImGui::Text(" | %zu notifications | %s",
        app.notifManager.Count(),
        ImGuiNotificationRenderer::GetPlatform());
    ImGui::SameLine();
    ImGui::Text(" | Editor: %s",
        app.spEditor->GetActiveTabWindow()->GetActiveWindow()->GetMode()->GetName().c_str());

    ImGui::End();

    // Render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    const int WIDTH = 1200;
    const int HEIGHT = 800;

    // SDL + OpenGL
    SDL_Window* window = CreateWindow(WIDTH, HEIGHT);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);
    InitImGui(window, glContext);

    // App state
    App app;

    // Init Zep editor
    app.spEditor = std::make_unique<ZepEditor_ImGui>("./zep_config");
    app.spEditor->InitWithText("notifications.toml", R"(
# Notification Configuration
# Edit this file to customize your feeds

[feeds.github]
enabled = true
events = ["push", "pr", "workflow"]

[feeds.jenkins]
url = "http://jenkins:8080"
jobs = ["main-build"]

[filters]
error = ["ERROR", "FAIL", "CRITICAL"]
)");

    // Init notifications
    InitNotifications(app.notifManager);

    // Main loop
    while (app.running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                app.running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                app.running = false;
            }
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        Render(app, w, h);

        SDL_GL_SwapWindow(window);
        SDL_Delay(16); // ~60fps
    }

    ShutdownImGui(glContext, window);
    return 0;
}