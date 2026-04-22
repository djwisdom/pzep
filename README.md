# nZep
## The Notification Editor for Developers Who Actually Give a Damn

*Yeah, we built an editor. And then we built the notification system it deserved.*

---

## The Vibe

Look, notifications are broken. They're either a flood of useless noise or a deafening silence when something's actually on fire. And editors? Either you're stuck with massive IDEs that take forever to load, or you're in the terminal wondering why your "mini editor" doesn't support half the features you need.

**nZep** changes that. It's two things:
1. **Zep** - A mini editor you can actually embed in your stuff
2. **Notifications** - Actionable alerts that respect your flow

Think of it as your command center. Build fails? You'll know *exactly* what's wrong and *where*. Right in the editor. Tests tanking? Here's the first failing test. Deploy borked? Here's the error, the file, and a link to the full story. All in one line that answers: "Should I interrupt what I'm doing?"

That's the vibe.

---

## What You Actually Get

### The Editor (Zep)

A proper embeddable code editor that works in ImGui, Qt, or your custom renderer. Vim mode if you want it. Normal mode if you don't. No fluff.

```
┌────────────────────────────────────────────┐
│  Zep Editor                          │
│                                    │
│  // Your code here                │
│  def main():                     │
│      print("hello world")         │
│                                    │
│  [Normal] [Vim] Mode             │
└────────────────────────────────────────────┘
```

- **Vim mode** - Most commands you actually use day-to-day
- **Standard mode** - Notepad-style, no learning curve  
- **Splits & tabs** - Work how you want
- **Syntax highlighting** - C++, Python, Rust, Go, JavaScript, and more
- **REPL integration** - Run code directly (Janet Lang example)
- **Theme support** - Light/dark, make it yours
- **No dependencies** - Core library is dependency-free
- **Cross-platform** - Windows, Linux, FreeBSD

### The Notifications (nZep)

Twelve notification types designed for developers who value their attention:

| When This Happens | You See This |
|----------------|------------|
| Build blows up | "Build failed — **core-lib** (target: all) — error: undefined at **src/util.cpp:128**" |
| Tests die | "Tests broken — **auth-suite** — test_login: expected 200 got 500" |
| Prod crashes | "Prod ERROR — **auth-service** — NullPointerException @ **AuthController.login()**" |
| Deploy done | "Deployment — **staging** — **v2.1.0** — success" |
| Security issue | "CRITICAL: Security — **lib/utils.js** — severity: HIGH" |

One line. Answers "Should I interrupt now?" in literally one line.

---

## Quick Start

### Building the Editor Demo

```bash
# Dependencies (vcpkg)
vcpkg install sdl2 imgui glad freetype

# Build
mkdir build && cd build
cmake .. -DBUILD_IMGUI=ON -DBUILD_DEMOS=ON
cmake --build .
```

Run: `./demos/demo_imgui/ZepDemo`

### Building Notifications

```bash
# Already included - just build with tests
mkdir build && cd build  
cmake .. -DBUILD_TESTS=ON
cmake --build . --target unittests
./tests/unittests --gtest_filter="Notification*"
```

### Running the Simple Test (No Dependencies!)

```bash
# Windows
cl /EHsc /std:c++17 /I. notification_demo/terminal_test.cpp /Fe:test.exe
./test.exe

# Linux/FreeBSD  
g++ -std=c++17 -I. notification_demo/terminal_test.cpp -o test
./test
```

---

## The Notification Architecture (For Real)

```
┌─────────────────────────────────────────────────────┐
│                 YOUR APP / CI/CD                 │
├─────────────────────────────────────────────────────┤
│  NotificationBuilder ──► Notification ──► Manager  │
│                    │                    │             │
│                    ├─► Toast (Critical only)          │
│                    ├─► Panel (all)               │
│                    └─► Suppress (noise)           │
├─────────────────────────────────────────────────────┤
│              RENDER LAYER                       │
│  ┌──────────────────┐  ┌──────────────────┐   │
│  │   ImGui Renderer  │  │ Terminal Renderer│   │
│  │  (Windows,      │  │  (Windows,       │   │
│  │   Linux,        │  │   Linux,         │   │
│  │   FreeBSD)      │  │   FreeBSD)      │   │
│  └──────────────────┘  └──────────────────┘   │
└─────────────────────────────────────────────────────┘
```

Three layers. Build once, render anywhere.

---

## Code Examples

### Creating a Notification (One Line)

```cpp
#include "notifications.h"

using namespace ZepNotifications;

// Build failure - done
manager.Add(BuildFailed(
    "core-lib",  // project
    "all",     // target
    "undefined reference",  // error
    "src/util.cpp:128",   // location
    "1234",             // build ID
    "http://jenkins/1234"  // log link
).Build());

// Test failure - done
manager.Add(TestFailed(
    "auth-suite",
    "test_login: expected 200 got 500",
    "http://jenkins/test/456"
).Build());

// Runtime error - done
manager.Add(RuntimeError(
    "auth-service",
    "NullPointerException", 
    "AuthController.login()",
    "req-abc123",
    "http://trace/abc123"
).Build());
```

### Full Control (Fluent API)

```cpp
Notification n = NotificationBuilder(NotificationType::RuntimeError)
    .SetSummary("Auth service down")
    .SetProject("auth-service")
    .SetBranch("main")
    .SetCommit("abc123")
    .SetError("NullPointerException")
    .SetErrorLocation("AuthController.login()")
    .SetFile("src/auth.cpp", 42)  // Click to jump in editor
    .SetSeverity(NotificationSeverity::Critical)
    .SetLink("http://trace/abc123")
    .SetAction("View Trace")
    .Build();
```

### Rendering in ImGui

```cpp
#include "notifications_imgui.h"

ImGuiNotificationRenderer renderer;

// In your render loop:
renderer.RenderPanel(manager, x, y, width, height);

// Toast for critical:
for (const auto& n : manager.GetCritical()) {
    renderer.RenderToast(n, 5.0f);  // 5 second toast
}
```

### Rendering in Terminal

```cpp
#include "notifications_term.h"

TerminalNotificationApp app;
app.Init();
app.Run();  // q to quit

// Or manual control:
TerminalNotificationRenderer renderer;
renderer.RenderPanel(manager, width, height);
```

---

## Notification Design Principles

1. **One line answers "Should I interrupt?"** - If it's not actionable in one line, it's not a notification
2. **Context, not noise** - "Build failed" is useless. "Build failed — core-lib — undefined at src/util.cpp:128" tells you everything
3. **Action immediately clear** - "Open log", "Re-run tests", "View trace" - no wondering what to do
4. **Link to full story** - Sometimes you need more. The ID and link are always there
5. **Respect developer flow** - Critical alerts get toast. Everything else waits in the panel
6. **Platform native** - Each renderer acts like the platform (Win/Lin/FreeBSD)

---

## What's Working

### Editor (Zep)
- Modal (Vim) + modeless editing
- ImGui and Qt renderers
- Tabs and splits
- Syntax highlighting (multiple languages)
- Theme support
- REPL integration
- 200+ unit tests
- Cross-platform (Windows, Linux, FreeBSD)

### Vim Mode (nZep extensions)
- **Ex Commands**: `:s` find/replace, `:w`, `:q`, `:wq`, `:buffers`, `:g`
- **Buffer Commands**: `:e` file, `:bn`/:`:bp`/:`:b` navigation, `:sp` split
- **:set Options**: number, wrap, autoindent, tabstop, expandtab, and more
- **Git Integration**: `:Gstatus`, `:Gdiff`, `:vdiff`, `:Gblame`, `:Gcommit`, `:Gpush`, `:Gpull`
- **Macros**: `q` recording, `@` playback
- **Folds**: `zf`, `zd`, `zo`, `zc`
- **Visual Mode**: `v`, `V`, `Ctrl+v` block
- **Multiple Cursors**: `Ctrl+d` add cursor
- **Terminal**: `:terminal`, `:!cmd`
- **Error Pointers**: Scrollbar indicators for off-screen errors

### Notifications (nZep)
- 12 notification types (Build, Test, Runtime, Deploy, Security, and more)
- ImGui renderer with toasts
- Terminal renderer with ANSI colors
- 19 unit tests
- Platform detection (Windows, Linux, FreeBSD)
- No external dependencies

---

## What's Not Done (Let's Be Honest)

- **Vim mode**: Missing paragraph motions, horizontal scroll, filter operator
- **Editor**: No LSP client (that's a whole other thing)
- **Notifications**: No persistence (runtime only)

But here's the thing - it's open source. Make it yours.

---

## Platform Support

| Platform | Editor | Notifications | Tests |
|----------|--------|-------------|-------|
| Windows | ✓ | ✓ | ✓ |
| Linux | ✓ | ✓ | ✓ (build script) |
| FreeBSD | ✓ | ✓ | ✓ (build script) |

---

## Links & Resources

- [Original Zep by @cmaughan](https://github.com/Rezonality/zep)
- [Architecture Doc](docs/NOTIFICATIONS_ARCHITECTURE.md)
- [User Manual](docs/NOTIFICATIONS_USER_MANUAL.md)  
- [Developer Manual](docs/NOTIFICATIONS_DEVELOPER_MANUAL.md)
- [Security Report](SECURITY_REPORT.md)
- [Full Analysis](REPORT.md)
- [Vim Compatibility Report](docs/VIM_COMPATIBILITY_REPORT.md)
- [Vim Commands Report](docs/VIM_COMMANDS_REPORT.md)
- [Technical Debt Report](docs/TECHNICAL_DEBT_REPORT.md)

---

## The Point

Notifications should respect your attention. An editor should be embeddable. Both should work on your platform without 47 dependencies.

That's nZep.

*Now go build something cool.*

---

[![Builds](https://github.com/Rezonality/zep/workflows/Builds/badge.svg)](https://github.com/Rezonality/zep/actions?query=workflow%3ABuilds)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/Resonality/zep/blob/master/LICENSE)
[![All Contributors](https://img.shields.io/badge/all_contributors-9-orange.svg?style=flat-square)](#contributors-)