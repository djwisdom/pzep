# nZep Notifications - Developer Manual
## Hook It Into Your Workflow

*This is where we get into the weeds. Let's go.*

---

## The Quick Integration (TL;DR)

```cpp
#include "notifications.h"

using namespace ZepNotifications;

// Create a notification
auto n = BuildFailed(
    "my-project",      // project
    "all",          // target  
    "undefined ref", // error
    "src/foo.cpp:42", // location
    "1234",        // build ID
    "http://log"    // link
).Build();

// Add to manager
manager.Add(n);
```

Three files to care about:
- `notifications.h` - Core types & builders
- `notifications_imgui.h` - ImGui renderer  
- `notifications_term.h` - Terminal renderer

That's it. Now let's break it down.

---

## The Core Types

### Notification Type Enum

```cpp
enum class NotificationType {
    BuildFailure,      // CI/CD build blowed up
    TestFailure,      // Tests is broke
    MergeCIStatus,   // PR status changed
    CodeReviewRequest, // Someone wants your eyes
    RuntimeError,   // Production on fire
    Deployment,    // Deploy done/failed
    SecurityAlert, // Something security-critical
    PerformanceRegression, // Things slow
    FlakyTest,    // Unreliable test
    DependencyVuln, // Update needed
    LongRunningTask, // That big task finished
    WorkspaceEvent // Git stuff
};
```

### Severity Levels

```cpp
enum class NotificationSeverity {
    Critical,  // DROP EVERYTHING - production is dying
    High,      // Should handle soon  
    Medium,    // Worth attention
    Low,       // FYI only
    Info       // Aggregated noise
};
```

---

## The Notification Structure

Every notification carries:

```cpp
struct Notification {
    // Core - the what
    std::string summary;           // One-line summary
    std::string id;            // ID for lookups
    
    // Classification
    NotificationType type;
    NotificationSeverity severity;
    
    // Timing
    std::chrono::system_clock::time_point timestamp;
    
    // Context - who/where/when
    std::string project;      // Which project/repo
    std::string branch;      // Which branch
    std::string commit;    // Which commit
    std::string author;   // Who triggered
    
    // Details
    std::string target;        // e.g., build target
    std::string first_error;  // Short error message
    std::string error_location; // file:line or function
    std::string request_id;    // Request trace ID
    
    // Action
    std::string link;         // Full details URL
    std::string action;       // Action text ("Open log")
    std::string action_link; // Action URL
    
    // Editor integration - CLICK TO JUMP
    std::optional<std::string> file_path;
    std::optional<int> file_line;
};
```

---

## The Builder API (Fluent)

### Shortcut Builders

Five-minute integration:

```cpp
// Build failure
manager.Add(BuildFailed(
    "core-lib", "all", "undefined reference", 
    "src/util.cpp:128", "1234", "http://jenkins/1234"
).Build());

// Test failure
manager.Add(TestFailed(
    "auth-suite", "test_login: expected 200 got 500", 
    "http://jenkins/test/456"
).Build());

// Runtime error (prod/staging)
manager.Add(RuntimeError(
    "auth-service", "NullPointerException",
    "AuthController.login()", "req-abc123",
    "http://trace/service/abc123"
).Build());

// Deploy result
manager.Add(DeployComplete(
    "staging", "v2.1.0", true, "http://deploy/789"
).Build());

// Security alert
manager.Add(SecurityAlert(
    "lib/utils.js", NotificationSeverity::High,
    "update lodash@>4.5.0", "http://gh/advisory/789"
).Build());
```

### Fluent Builder (Full Control)

For maximum flex:

```cpp
Notification n = NotificationBuilder(NotificationType::RuntimeError)
    .SetSummary("Auth service down")
    .SetProject("auth-service")
    .SetBranch("main")
    .SetCommit("abc123")
    .SetAuthor("deploy-bot")
    .SetTarget("auth-api")
    .SetError("NullPointerException")
    .SetErrorLocation("AuthController.login()")
    .SetRequestID("req-xyz789")
    .SetLink("http://trace/xyz789")
    .SetAction("View Trace")
    .SetFile("src/auth.cpp", 42)
    .SetSeverity(NotificationSeverity::Critical)
    .Build();
```

---

## The Notification Manager

```cpp
NotificationManager manager;

// Add notifications
manager.Add(notification);

// Query
auto critical = manager.GetCritical();
auto byProject = manager.GetByProject("auth-service");
auto bySeverity = manager.GetBySeverity(NotificationSeverity::High);

// Stats
size_t count = manager.Count();

// Clear all
manager.Clear();

// Circular buffer - keeps last 100, drops oldest
```

---

## ImGui Renderer Integration

```cpp
#include "notifications_imgui.h"

ImGuiNotificationRenderer renderer;

// In your render loop:
renderer.RenderPanel(manager, x, y, width, height);

// For critical toasts:
for (const auto& n : manager.GetCritical()) {
    renderer.RenderToast(n, 5.0f);  // 5 second toast
}
```

### Customizing ImGui Styles

```cpp
ImGuiNotificationStyle& style = renderer.style;

// Tweak colors
style.critical = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
style.success = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);

// Handle clicks - in your app:
if (ImGui::IsItemClicked()) {
    // spEditor->OpenFileAt(*n.file_path, *n.file_line);
}
```

---

## Terminal Renderer Integration

```cpp
#include "notifications_term.h"

TerminalNotificationRenderer renderer;
TerminalNotificationApp app;

// Quick start (handles setup, input)
app.Init();
app.Run();  // q to quit

// Manual control:
int width, height;
renderer.GetTermSize(width, height);
renderer.RenderPanel(manager, width, height);

// Non-blocking input:
if (renderer.HasInput()) {
    char c = renderer.ReadChar();
    // handle c
}
```

### Customizing Terminal Colors

```cpp
TerminalStyle& style = renderer.style;

// ANSI codes - tweak away
style.critical = "\033[1;31m";  // Bold red
style.high = "\033[38;5;208m";   // Orange  
style.success = "\033[32m";        // Green
```

---

## Platform Detection

Both renderers auto-detect:

```cpp
#ifdef _WIN32
    // Windows
#elif defined(__linux__)
    // Linux  
#elif defined(__FreeBSD__)
    // FreeBSD
#endif

// Or via helper:
const char* platform = ImGuiNotificationRenderer::GetPlatform();  // "Windows", "Linux", "FreeBSD"
```

---

## CI/CD Integration

### Jenkins Example

```groovy
post {
    failure {
        script {
            def err = currentBuild.result ?: 'UNDEFINED'
            def loc = getFirstError()
            def logUrl = env.BUILD_URL
            // POST to your notification endpoint
            sh "curl -X POST ${NOTIF_URL} -d 'project=${JOB_NAME}&target=${env.BUILD_NUMBER}&error=${err}&location=${loc}&link=${logUrl}'"
        }
    }
}
```

### GitHub Actions

```yaml
- name: Notify on failure
  if: failure()
  run: |
    curl -X POST ${{ secrets.NOTIF_URL }} \
      -d "project=${{ github.repository }}" \
      -d "target=${{ github.workflow }}" \
      -d "error=${{ steps.test.outputs.error }}" \
      -d "link=${{ github.run_id }}"
```

---

## Webhooks (JSON Payload)

If you're building a webhook receiver:

```json
{
  "type": "BuildFailure",
  "severity": "Critical",
  "project": "core-lib",
  "target": "all",
  "error": "undefined reference",
  "location": "src/util.cpp:128",
  "id": "1234",
  "link": "http://jenkins/build/1234"
}
```

Parse this, pass to `NotificationBuilder`, add to manager.

---

## Extending - Adding Custom Types

Want a new notification type? Here's how:

1. Add to enum:
```cpp
enum class NotificationType {
    // ... existing
    CustomType,  // NEW
};
```

2. Add format:
```cpp
inline std::string FormatCustomType(const Notification& n) {
    return "[CUSTOM] " + n.summary;
}
```

3. Add builder:
```cpp
inline NotificationBuilder CustomNotification(...) {
    return NotificationBuilder(NotificationType::CustomType)
        // ... set fields
        .Build();
}
```

---

## Tests! (Please)

Run the tests:

```bash
cd build/tests/Debug
./unittests.exe --gtest_filter="Notification*"
```

19 tests covering builders, formatting, manager logic. All passing.

---

## Quick Reference Card

| Need | Do This |
|------|---------|
| Build fail | `BuildFailed(proj, tgt, err, loc, id, url)` |
| Test die | `TestFailed(suite, err, url)` |
| Runtime | `RuntimeError(svc, exc, loc, req, url)` |
| Deploy | `DeployComplete(env, ver, success, url)` |
| Security | `SecurityAlert(file, severity, fix, url)` |
| Query critical | `manager.GetCritical()` |
| Query project | `manager.GetByProject("name")` |
| ImGui panel | `renderer.RenderPanel(manager, x, y, w, h)` |
| Terminal | `renderer.RenderPanel(manager, w, h)` |

---

## Issues? Questions?

The notification system is open source. Check the GitHub, file an issue, or just read the code. It's not that complicated - promise.

Now go ship something.