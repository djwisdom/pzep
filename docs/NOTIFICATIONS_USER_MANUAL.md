# nZep Notifications - User Manual
## Because Your Time Shouldn't Be Wasted Context-Switching

*Created for developers who actually care about getting things done.*

---

## Wait, What Is This Thing?

Look, let me be straight with you - notifications are broken. They're either a flood of useless noise or a deafening silence when something's actually on fire. The nZep Notification System is different - it's built for developers who need **actionable information**, not "hey someone commented on your PR" spam.

Think of it as a command center for your dev workflow. Build fails? You'll know *exactly* what's wrong and *where*. Tests tanking? Here's the first failing test. Deploy borked? Boom - here's the error, the file, and a link to the full story. All in one line that answers: "Should I interrupt what I'm doing?"

That's the vibe we're going for here.

---

## Getting Started (Quick)

### The Notification Types

Here's the deal - there are 12 notification types, but you really care about these core ones:

| What Happened | What You See |
|--------------|------------|
| Build blows up | "Build failed — **core-lib** (target: all) — error: undefined reference at **src/util.cpp:128** — Open log [ID#1234]" |
| Tests die | "Tests broken — **auth-suite** — 3 fails — test_login: expected 200 got 500 — Re-run" |
| Runtime crash | "Prod ERROR — **auth-service** — NullPointerException @ **AuthController.login()** — request=abc123 — Open trace" |
| Deploy done | "Deployment — **staging** — **v2.1.0** — success — View Deploy" |
| Security issue | "CRITICAL: Security — **lib/utils.js** — severity: HIGH — remediation: update lodash@>4.5.0" |

See that pattern? Project name, what went wrong, *where* it went wrong (file:line), and an action. That's intentional.

---

## Understanding the Display

### Terminal View (Simple, Clean)

When you're in the terminal, you'll see something like this:

```
╔══════════════════════════════════════════════╗
║ NOTIFICATIONS                           ║
╚══════════════════════════════════════════════╝
[A]ll [C]ritical [B]uild [T]est [R]untime | Platform: Linux
1. ✗ [BUILD] core-lib — undefined reference @ src/util.cpp:128
2. ✗ [TESTS] auth-suite — test_login failed
3. ✓ [DEPLOY] staging — v2.1.0 — success
────────────────────────────────────────────
✓ OK | 3 total | Linux
[Enter] open [x] dismiss [q] quit
```

Press **C** to see only critical items.
Press **B** for builds only.
Press **Q** to peace out.

### ImGui View (Graphical)

In the ImGui version, you've got:

1. **Toast popups** - Critical stuff shows up top-right as a toast. Can't miss it.
2. **Notification panel** - Scrollable list on the left side
3. **Clickable file:lines** - Click on `src/util.cpp:128` and it jumps there in your editor
4. **Severity badges** - Red for critical, orange for high, yellow for medium, blue for low

The colors actually mean something. Red means *drop what you're doing*.

---

## Controls

### Terminal Controls

| Key | What It Does |
|-----|-------------|
| `q` or `Q` | Quit - go home |
| `c` or `C` | Show Critical only |
| `b` or `B` | Show Build failures |
| `t` or `T` | Show Test failures |
| `r` or `R` | Show Runtime errors |
| `a` or `A` | Show All |
| `Enter` | Open selected notification |
| `x` | Dismiss selected |

### ImGui Controls

Click the file:line in any notification to jump straight there in Zep. Easy.

---

## Filtering & Notifications You Actually Want

Here's the key - configure what lights you up:

**Critical (always toast):**
- Build failures on main/develop branches
- Test failures (any branch)
- Security vulnerabilities
- Production runtime errors

**High (panel highlight):**
- Deploy failures
- Performance regressions

**Medium/Low (just list):**
- Code review requests
- Flaky tests
- Dependency updates

You can filter by project, severity, type - whatever keeps you in the zone.

---

## The Design Philosophy (Why This Works)

A few principles here:

1. **One line answers "Should I interrupt?"** - If it's not actionable in one line, it's not a notification. It's a document.

2. **Context, not noise** - "Build failed" is useless. "Build failed — core-lib — undefined reference at src/util.cpp:128" tells you the project, what happened, and where.

3. **Action immediately clear** - Every notification has an action. "Open log", "Re-run tests", "View trace". No wondering what to do next.

4. **Link to full story** - Sometimes you need more. The ID and link are always there.

5. **Respect developer flow** - Critical alerts get toast. Everything else waits in the panel. You're in control.

---

## Wrapping Up

This isn't just another notification system. It's built for developers who treat their attention as a scarce resource - because it is.

The whole point? **Reduce context switching. Decide fast. Get back to coding.**

That is all.

---

*Questions? Issues? Actually, just ping us on GitHub. Now go build something cool.*