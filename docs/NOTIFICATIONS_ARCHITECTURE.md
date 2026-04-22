# nZep Notifications - Design Architecture
## Under the Hood

*How it all works. For the curious and the contributors.*

---

## The Big Picture

```
┌─────────────────────────────────────────────────────────────────┐
│                     YOUR APP / CI/CD                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  NotificationBuilder ──► Notification ──► NotificationManager │
│         │                                        │             │
│         │                    ┌──────────────────────┼──────────┐  │
│         │                    │                      │          │  │
│         ▼                    ▼                      ▼          ▼  │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────┐  │
│  │   CI/CD     │    │  Webhooks  │    │  Manual Triggers    │  │
│  │   Hooks    │    │   Parser   │    │                     │  │
│  └─────────────┘    └─────────────┘    └─────────────────────┘  │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│                     RENDER LAYER                                 │
│                                                                 │
│  ┌────────────────────┐    ┌────────────────────┐           │
│  │ ImGuiNotification │    │ TerminalNotificati │           │
│  │    Renderer      │    │      Renderer       │           │
│  │   (Windows,     │    │   (Windows,       │           │
│  │    Linux,      │    │    Linux,        │           │
│  │    FreeBSD)    │    │    FreeBSD)     │           │
│  └────────────────────┘    └────────────────────┘           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

Three layers. Build once, render anywhere. That's the vibe.

---

## Layer 1: Core Data Model

### Notification Structure (`notifications.h`)

The notification is the atomic unit. Here's what's happening internally:

```cpp
struct Notification {
    // Required: tells you WHAT happened in one line
    std::string summary;
    
    // Required: system identifier for lookups
    std::string id;
    
    // Classification: type + severity (orthogonal dimensions)
    // Types are events. Severity is urgency.
    NotificationType type;
    NotificationSeverity severity;
    
    // Context: WHO/WHERE/WHEN
    // Project = which repo/project
    // Branch/commit = which code version
    // Author = who triggered it
    std::string project;
    std::string branch;
    std::string commit;
    std::string author;
    
    // Details: WHAT specifically
    // Target = build target, test suite, service name
    // First error = shortest useful error summary  
    // Error location = file:line or function name
    std::string target;
    std::string first_error;
    std::string error_location;
    
    // Tracing: for correlation
    std::string request_id;
    
    // Action: links to the story + what to do
    std::string link;
    std::string action;
    std::string action_link;
    
    // Editor integration: jump to exact location
    std::optional<std::string> file_path;  // Nullable - only if jumpable
    std::optional<int> file_line;
};
```

**Design decisions:**

- `summary` is the one-line answer to "should I interrupt?"
- Type + Severity are independent - a BuildFailure can be Critical or Low (depends on branch)
- `first_error` is NOT the full error - it's a summary ("undefined reference")
- Location is exact (`file:line`) enabling editor jump
- Optionals for editor integration - UI layer decides if clickable

---

## Layer 2: Builder Pattern

### Why the Builder?

The builder gives you two APIs:

1. **Shortcut builders** - 80% use case in 1 line
2. **Fluent builder** - full control when you need it

```cpp
// Shortcut - common case
BuildFailed("proj", "target", "error", "loc", "id", "url")

// Fluent - custom everything
NotificationBuilder(NotificationType::BuildFailure)
    .SetSummary(...)
    .SetProject(...)
    .SetBranch(...)
    .SetCommit(...)
    .SetAuthor(...)
    .SetTarget(...)
    .SetError(...)
    .SetErrorLocation(...)
    .SetRequestID(...)
    .SetLink(...)
    .SetAction(...)
    .SetFile(...)
    .Build()
```

This is the *builder pattern* - flexible creation without telescoping constructors.

---

## Layer 3: Notification Manager

### The Collection

```cpp
class NotificationManager {
    std::vector<Notification> notifications;
    
    // Circular buffer behavior - keeps last 100
    void Add(const Notification& n) {
        notifications.push_back(n);
        if (notifications.size() > 100) {
            notifications.erase(notifications.begin());
        }
    }
};
```

**Design decisions:**

- Circular buffer - notifications older than 100 are dropped
- Not persisted - this is runtime, not storage
- Simple vector - O(1) add, O(1) access
- Query methods return copies for thread safety

### Query Methods

```cpp
// Get critical only - for toast display
GetCritical();

// Filter by project - for multi-project repos
GetByProject(const std::string& project);

// Filter by severity  
GetBySeverity(NotificationSeverity severity);

// By type
GetByType(NotificationType type);
```

---

## Layer 4: Renderers

### Architecture by Renderer

#### ImGui Renderer (`notifications_imgui.h`)

```
Notification ──► ImGuiNotificationRenderer ──► Screen
                    │
                    ├── Toast (top-right popup)
                    │   - Position: fixed top-right
                    │   - Duration: seconds
                    │   - Auto-dismiss
                    │
                    ├── Panel (scrollable list)
                    │   - Position: left side
                    │   - Scrollable
                    │   - Clickable file:line
                    │
                    └── Status bar (bottom)
                        - Counts
                        - Platform
```

**Platform detection:**
```cpp
// Compile-time detection
#ifdef _WIN32    return "Windows";
#ifdef __linux__  return "Linux";
#ifdef __FreeBSD__ return "FreeBSD";
```

**Styling:**
- Severity colors built in
- Platform styles separate
- Customizable style struct

#### Terminal Renderer (`notifications_term.h`)

```
Notification ──► TerminalNotificationRenderer ──► Terminal
                    │
                    ├── Panel (box layout)
                    │   - Unicode box drawing
                    │   - ANSI colors
                    │
                    └── Toast (brief popup)
                        - 3 second display
                        - ANSI cursor save/restore
```

**Platform handling:**

- Windows: `conio.h` (`_kbhit`, `_getch`)
- POSIX: `termios` + ` select()` for non-blocking input
- ANSI detection: check TERM env + isatty()

---

## Data Flow

### Creation Flow

```
Your CI/CD ──► Create Notification ──► Add to Manager ──► Render
    │              │                          │
    │              │                          │
    │              └─► Validate              └─► Filter by severity
    │                                        │
    │                                        ├─► Toast (Critical only)
    │                                        ├─► Panel (all)
    │                                        └─► Suppress (Info, duplicates)
    │                                                    
    └─► Link to source
```

### Rendering Flow

```
Each Frame:
    │    
    ├─► Clear (terminal) / NewFrame (ImGui)
    │
    ├─► GetCritical():
    │       │
    │       └─► Toast() for each critical
    │             - Only Critical/High severity
    │             - Duration-based dismiss
    │
    ├─► GetAll():
    │       │
    │       └─► Panel():
    │             - Iterate notifications
    │             - Format by type
    │             - Color by severity
    │
    └─► Render
```

---

## Thread Safety

**Current state:** Single-threaded by design.

**Rationale:** Notifications typically arrive from:
- Main render loop
- Single webhook handler
- One CI/CD pipeline

**If you need thread safety:**
- Queue incoming notifications (lock-free queue recommended)
- Renderer reads from queue in main loop
- Or: make `GetCritical()` return atomic/copy

---

## Platform Considerations

### Windows
- Console: `conio.h` for input
- Colors: Basic console (not full ANSI)
- 8-bit color via console API

### Linux
- Terminal: Full ANSI support (most terminals)
- Colors: 256-color ANSI codes
- Input: `termios` + `select()`

### FreeBSD
- Same as Linux for most parts
- Terminal detection slightly different
- Same ANSI code handling

---

## Extension Points

### Adding a New Notification Type

1. Add to `NotificationType` enum
2. Add builder shortcut function
3. Add format function for each renderer
4. Add test case

That's it. No other changes needed.

### Adding a New Renderer

1. Create new file: `notifications_<platform>.h`
2. Implement:
   - `RenderPanel(manager)`
   - `RenderToast(notification)`
   - `GetPlatform()`
3. Reuse `Notification` struct and `NotificationManager`

---

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Create notification | O(1) | Stack allocation |
| Add to manager | O(1) amortized | Vector push |
| Get by severity | O(n) | Linear scan |
| Get by project | O(n) | Linear scan |
| Render to terminal | O(n) | n = notifications shown |
| Render to ImGui | O(n) | Same |

Memory: ~1KB per notification. 100 notifications = ~100KB.

---

## Design Philosophy

1. **Single source of truth** - One notification struct
2. **Renderer agnostic** - Core doesn't care about UI
3. **Extension not modification** - Add types without editing core
4. **Action-first** - Every notification has next step
5. **Context over noise** - One line answers the question
6. **Platform native** - Each renderer acts like the platform

---

## Contributing

When adding features:

1. **Tests first** - Add test in `notifications.test.cpp`
2. **Core unchanged** - Don't modify notification struct
3. **New builders** - Add shortcut functions
4. **New renderers** - New file, don't edit existing
5. **Document** - Update both manuals

---

## Dependencies

```
notifications.h       ──► (none) - Pure C++17
notifications_imgui.h ──► notifications.h + ImGui
notifications_term.h ──► notifications.h + <iostream>
```

No external dependencies. That's intentional.

---

## TL;DR

- Three layers: Data → Manager → Renderer
- One struct: Notification (core)
- Two renderer interfaces: ImGui, Terminal  
- Extension by addition, not modification
- Tests guarantee no regression

That's the architecture. Now go build something.