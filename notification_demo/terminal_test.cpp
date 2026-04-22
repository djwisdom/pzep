// Simple ASCII Terminal Notification Test
// No ANSI, no colors - just pure ASCII
// Works on: Windows, Linux, FreeBSD

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "../notifications.h"

using namespace ZepNotifications;

void PrintBanner()
{
    std::cout << "\n";
    std::cout << "+====================================+\n";
    std::cout << "|     nZep TERMINAL TEST             |\n";
    std::cout << "+====================================+\n";
    std::cout << "\n";
}

void PrintNotification(const Notification& n)
{
    std::string icon = " ";
    if (n.severity == NotificationSeverity::Critical)
        icon = "X";
    else if (n.severity == NotificationSeverity::High)
        icon = "!";
    else
        icon = ">";

    std::cout << icon << " ";

    // Format based on type
    switch (n.type)
    {
    case NotificationType::BuildFailure:
        std::cout << "BUILD: " << n.project;
        if (!n.target.empty())
            std::cout << " (" << n.target << ")";
        if (!n.first_error.empty())
            std::cout << " - " << n.first_error;
        if (!n.error_location.empty())
            std::cout << " at " << n.error_location;
        break;

    case NotificationType::TestFailure:
        std::cout << "TEST: " << n.target;
        if (!n.first_error.empty())
            std::cout << " - " << n.first_error;
        break;

    case NotificationType::RuntimeError:
        std::cout << "RUNTIME: " << n.project;
        if (!n.first_error.empty())
            std::cout << " - " << n.first_error;
        if (!n.error_location.empty())
            std::cout << " at " << n.error_location;
        break;

    case NotificationType::Deployment:
        std::cout << "DEPLOY: " << n.project;
        if (!n.target.empty())
            std::cout << " - " << n.target;
        if (!n.first_error.empty())
            std::cout << " - " << n.first_error;
        break;

    case NotificationType::SecurityAlert:
        std::cout << "SECURITY: " << n.project;
        if (!n.first_error.empty())
            std::cout << " - " << n.first_error;
        break;

    default:
        std::cout << n.summary;
    }

    std::cout << "\n";

    // Show location + link
    if (!n.error_location.empty())
    {
        std::cout << "  -> Location: " << n.error_location << "\n";
    }
    if (!n.link.empty())
    {
        std::cout << "  -> Link: " << n.link << "\n";
    }
    if (n.file_path && n.file_line)
    {
        std::cout << "  -> EDITOR: " << *n.file_path << ":" << *n.file_line << " (click to open)\n";
    }

    std::cout << "\n";
}

void RunDemo()
{
    NotificationManager mgr;

    std::cout << "[1] Adding sample notifications...\n\n";

    // Add notifications WITH file:line for editor integration
    auto n1 = BuildFailed("core-lib", "all", "undefined reference",
        "src/util.cpp:128", "1234", "http://jenkins/build/1234")
                  .SetFile("src/util.cpp", 128)
                  .Build();
    mgr.Add(n1);

    auto n2 = TestFailed("auth-suite", "test_login: expected 200 got 500",
        "http://jenkins/test/456")
                  .Build();
    mgr.Add(n2);

    auto n3 = RuntimeError("auth-service", "NullPointerException",
        "AuthController.login()", "req-abc123", "http://trace/abc123")
                  .SetFile("src/auth.cpp", 42)
                  .Build();
    mgr.Add(n3);

    auto n4 = DeployComplete("staging", "v2.1.0", true,
        "http://deploy/staging/789")
                  .Build();
    mgr.Add(n4);

    auto n5 = SecurityAlert("lib/utils.js", NotificationSeverity::High,
        "update lodash@>4.5.0", "http://gh/advisory/789")
                  .Build();
    mgr.Add(n5);

    std::cout << "Notifications: " << mgr.Count() << "\n";
    std::cout << "Critical: " << mgr.GetCritical().size() << "\n\n";

    // Print all notifications
    std::cout << "+====================================+\n";
    std::cout << "|         ALL NOTIFICATIONS          |\n";
    std::cout << "+====================================+\n\n";

    int i = 1;
    for (const auto& n : mgr.notifications)
    {
        std::cout << "[" << i << "] ";
        PrintNotification(n);
        i++;
    }

    std::cout << "+====================================+\n";
    std::cout << "|           SUMMARY                 |\n";
    std::cout << "+====================================+\n\n";

    auto critical = mgr.GetCritical();
    if (critical.size() > 0)
    {
        std::cout << "ACTION REQUIRED: " << critical.size() << " critical issues\n";
        for (const auto& n : critical)
        {
            std::cout << "  - " << n.project << ": " << n.first_error << "\n";
        }
    }
    else
    {
        std::cout << "STATUS: All clear\n";
    }

    std::cout << "\n";
    std::cout << "Platform: ";
#ifdef _WIN32
    std::cout << "Windows\n";
#elif defined(__linux__)
    std::cout << "Linux\n";
#elif defined(__FreeBSD__)
    std::cout << "FreeBSD\n";
#else
    std::cout << "Unknown\n";
#endif
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    PrintBanner();

    std::cout << "Choose test:\n";
    std::cout << "  [1] Quick demo (show notifications)\n";
    std::cout << "  [2] Show editor integration\n";
    std::cout << "\n> ";

    int choice = '1';
    std::cin >> choice;
    std::cin.ignore();

    std::cout << "\n";

    switch (choice)
    {
    case 1:
        RunDemo();
        break;

    case 2:
        std::cout << "+====================================+\n";
        std::cout << "|       EDITOR INTEGRATION           |\n";
        std::cout << "+====================================+\n\n";

        std::cout << "To use the Zep editor:\n\n";
        std::cout << "1. Build with ImGui (requires vcpkg):\n";
        std::cout << "   mkdir build && cd build\n";
        std::cout << "   cmake .. -DBUILD_IMGUI=ON -DBUILD_DEMOS=ON\n";
        std::cout << "   cmake --build .\n\n";
        std::cout << "2. Run the demo:\n";
        std::cout << "   ./demos/demo_imgui/ZepDemo\n\n";

        std::cout << "Alternative - VS Code extension:\n";
        std::cout << "   Use VS Code with zep-mode or vim extension\n\n";

        std::cout << "For the ImGui + Notification app:\n";
        std::cout << "   See notification_demo/main.cpp\n";
        std::cout << "   (requires Zep library built)\n";
        break;

    default:
        RunDemo();
        break;
    }

    std::cout << "\n=== DONE ===\n";

    return 0;
}