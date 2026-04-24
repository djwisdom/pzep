# pZep - Vim-like Code Editor with Interactive Tutorial & REPL

A lightweight, standalone code editor with full Vim keybindings, an interactive tutorial system (`:tutor`), and multi-language REPL integration.

## 🚀 Quick Start

```bash
# Build
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DENABLE_LUA_REPL=ON -DENABLE_DUKTAPE_REPL=ON ..
cmake --build . --config Release

# Run (Windows GUI)
./Release/pzep-gui.exe [filename]
```

**Keybindings:**
- `i` — Enter insert mode
- `ESC` — Return to normal mode
- `:q` — Quit current window; `:q!` — Force quit
- `:w` — Save buffer
- `:tutor` — Launch interactive tutorial
- `:lua` / `:duktape` / `:quickjs` — Open REPL buffer (if enabled)
- `Ctrl+Q` — Quit application (with dirty buffer check)

## 🎓 Interactive Tutorial (`:tutor`)

The built-in tutorial guides you through pZep's core features with hands-on demos:

1. **Basic Navigation & Modes** — Normal, Insert, Visual
2. **Window Splits** — `:split` / `:vsplit`, navigating panes
3. **Tab Management** — `:tabnew`, `gt` / `gT`
4. **REPL Integration** — Try Lua, Duktape, or QuickJS inline
5. **Buffers & Files** — Managing multiple files
6. **Advanced Commands** — Search, replace, macros

Each lesson opens a dedicated tutorial buffer with explanatory text and embedded demo commands. Press the suggested keys to try features live.

## 🖥️ REPL Integration

pZep supports multiple scripting language REPLs that run in dedicated editor buffers:

| REPL | CMake Flag | Example Usage |
|------|------------|---------------|
| **Lua** | `-DENABLE_LUA_REPL=ON` | `:lua` → `1+1` → Enter → `2` |
| **Duktape (JS)** | `-DENABLE_DUKTAPE_REPL=ON` | `:duktape` → `Math.sqrt(16)` → Enter |
| **QuickJS** | `-DENABLE_QUICKJS_REPL=ON` | `:quickjs` → `const x = 42; x` |

**REPL Keybindings (inside REPL buffer):**
- `Enter` — Evaluate current line or selected expression
- `Ctrl+Enter` — Evaluate outer expression
- `Shift+Enter` — Evaluate and insert result

## 🏗️ Architecture Overview

### Core Components

- **`ZepEditor`** — Main editor controller, manages tab windows, buffers, and global state
- **`ZepTabWindow`** — Container for editor panes (splits) and file tabs
- **`ZepWindow`** — Single editor pane with text area, line numbers, minimap, and scrollbars
- **`ZepBuffer`** — Text buffer with undo/redo, syntax highlighting, and file I/O
- **`ZepMode`** — Input modes (Normal, Insert, Visual, Command, Ex)
- **`ZepDisplay`** — Rendering and input abstraction (Raylib backend)

### Layout (Four-Corner Design)

```
┌─────────────────────────────────────────────┐
│ Tab Region (top)                           │ ← File tabs
├───────────────────────────────────────────┤
│ Tab Content Region (center)                │
│ ┌─────┬────┬──────────────┬───────────┐   │
│ │Line │Git │     Text     │ Scrollbar/│   │
│ │ nums│    │              │ Minimap   │   │
│ └─────┴────┴──────────────┴───────────┘   │
├───────────────────────────────────────────┤
│ Command Region (bottom)                   │
│ Vim | NORMAL | 100% | file.txt | 12:5   │ ← Status bar
└─────────────────────────────────────────────┘
```

### Vim Mode Reference

| Mode | Key | Description |
|------|-----|-------------|
| **Normal** | `ESC` | Navigation, commands (`:w`, `:q`)
| **Insert** | `i`, `a`, `o` | Text editing
| **Visual** | `v` | Character-wise selection
| **Command** | `:` | Ex commands and search (`/pattern`)

## 🔨 Building

### Windows (MSVC)

```cmd
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 \
  -DENABLE_LUA_REPL=ON \
  -DENABLE_DUKTAPE_REPL=ON \
  -DENABLE_QUICKJS_REPL=ON \
  ..
cmake --build . --config Release
```

The main library (`Zep.lib`) builds by default. The GUI app (`pzep-gui`) and REPL plugins are built as separate targets when their respective options are enabled.

### Linux / macOS (GCC/Clang)

```bash
mkdir build && cd build
cmake -DENABLE_LUA_REPL=ON -DENABLE_DUKTAPE_REPL=ON ..
make -j$(nproc)
```

**Dependencies:** Lua, Duktape development headers (or use bundled sources).

## 📁 Repository Structure

```
pzep/
├── apps/
│   └── pzep-gui/         # pZep-GUI application (Raylib)
├── include/zep/          # Public headers
│   ├── mode_tutorial.h   # Tutorial mode
│   ├── commands_repl.h   # REPL command wrappers
│   ├── repl_plugin.h     # Plugin ABI for external REPLs
├── src/
│   ├── mode_tutorial.cpp # Tutorial implementation
│   ├── commands_repl.cpp # REPL command registration
│   ├── mode_lua_repl.cpp # Lua REPL provider
│   ├── mode_duktape_repl.cpp # Duktape REPL
│   ├── mode_quickjs_repl.cpp # QuickJS REPL
│   ├── repl_plugin_loader.cpp # Dynamic plugin loading
│   ├── mode_vim.cpp      # Vim keybindings & ex commands
│   ├── window.cpp        # 4-corner layout, line numbers, minimap
│   └── ...               # Core editor (buffer, syntax, etc.)
├── plugins/              # Plugin build configuration
├── tests/                # Unit tests
└── README.md
```

## ✨ Recent Features (v0.5.x)

- **`:tutor` command** — Guided interactive tutorial
- **Multi-language REPLs** — Lua, Duktape (JavaScript), QuickJS
- **Dynamic plugin system** — Load external REPL providers at runtime
- **Vim-style tilde indicators** — `~` for lines beyond EOF in line numbers
- **Filler line rendering** — Correct viewport filling and minimap alignment
- **Ex command `!` support** — `:q!`, `:w!` work without space
- **Windows GUI** — No console window, native Win32 subsystem

## 🐛 Known Issues & Workarounds

- **Cursor line/column display** — May show `-1` in rare edge cases after certain edits. A defensive clamp ensures the UI remains usable. (Tracking issue: investigate stale `m_windowLines` after buffer modifications.)
- **REPL plugin loading** — QuickJS plugin may require external library on Windows. Use bundled sources via CMake options.

## 📄 License

Fork of Zep. Originally based on nzep (Notification Editor). See repository for full license details.
