# pZep Native Integrations & Advanced Features Implementation Plan

**Version:** 1.0  
**Target:** Post-0.5.x stabilization  
**Scope:** Recent files, system tray, URI handlers, plugin system, LSP, IME, native dialogs, performance tooling  
**Platform:** Windows primary; macOS/Linux follow-up (where applicable)

---

## 1. Recent Files Menu

**Goal:** Maintain a list of recently opened files (MRU) accessible from the menu bar, with configurable limit and persistence.

### Current State
- Files opened via command line, `:e`, or double-click are not tracked centrally.
- No menu bar exists in the current GUI (pzep-gui has no native menu bar; Windows GUI apps typically use a menubar or context menu).

### Implementation Plan

#### Phase 1 – MRU Storage & API
- Add `ZepEditor::GetRecentFiles()` and `ZepEditor::AddRecentFile(const fs::path&)`.
- Store MRU list in config directory (`~/.pzep/recent_files.json` or TOML).
- Config: `"max_recent_files": 20` (default). LRU eviction.
- Add `ClearRecentFiles()` command.

#### Phase 2 – Menu Integration
- Extend `pzep-gui` main loop to create a native Windows menubar (via raylib's `GuiMenuBar` if available, or custom region drawn at top).
- Add **File → Recent Files** submenu showing up to N entries, with `&1` keyboard accelerators if ≤9 items.
- On selection, call `editor.OpenFile(path)` (reuse existing buffer load logic).
- Gray out submenu when empty.

#### Phase 3 – Persistence & UI Polish
- Load MRU on startup, save on exit (and optionally on each add).
- Show "No recent files" placeholder.
- Support clearing MRU via `:clearrecent` or menu item.
- Consider file existence check on load (if missing, remove from list).

**Dependencies:** Config system (`~/.pzep/.pzeprc` already exists), raylib UI primitives.  
**Effort:** ~2–3 days.

---

## 2. System Tray Icon

**Goal:** Background operation, quick access to common actions (show/hide, open file, quit) via tray icon and context menu.

### Current State
- No tray integration; pzep-gui is a standard windowed app.

### Implementation Plan (Windows-focused)

#### Phase 1 – Tray Icon Basics
- Use Win32 API directly (since pzep-gui is already WIN32 subsystem):  
  - `NOTIFYICONDATA` struct, `Shell_NotifyIcon(NIM_ADD, ...)`, `NIM_MODIFY`, `NIM_DELETE`.
- Load an icon resource (embed `.ico` or use raylib's `LoadImage` → convert to `HICON`).
- Add minimal tray icon in `ZepDisplay_Raylib::Init()` or main.cpp during startup.
- Handle `WM_TRAYICON` message (register window message `"WM_TRAYICON"`; add to `WndProc`).
- On left-click: toggle window show/hide (minimize to tray / restore).
- On right-click: show context menu (popup) with items: **Open**, **Show/Hide**, **Quit**.

#### Phase 2 – Context Menu Actions
- **Open** → `GetOpenFileName` common dialog (reuse existing dialog code if any) → `editor.OpenFile`.
- **Show/Hide** → `ShowWindow(hwnd, SW_SHOW|SW_HIDE)` toggle, set `m_trayVisible` flag.
- **Quit** → `PostQuitMessage(0)` after saving config.

#### Phase 3 – Lifetime & Cleanup
- Remove tray icon on app exit (`NIM_DELETE`).
- Handle `WM_DESTROY` gracefully even when minimized to tray.
- Optionally: single-instance enforcement via named mutex (if second instance, restore first and exit).

**Dependencies:** Win32 message loop access. Currently `pzep-gui` uses raylib's `SetWindowState`/`IsWindowMinimized`, but to receive `WM_TRAYICON` we need to subclass or replace raylib's window proc on Windows.
- **Approach A (preferred):** Subclass raylib's toplevel window via `SetWindowLongPtr` after display init; add `WM_TRAYICON` case to custom proc; forward other messages to raylib's original proc.
- **Approach B:** Replace raylib's Win32 backend entirely (too invasive). Not recommended.

**Effort:** ~3–4 days (Win32 interop testing).

---

## 3. URI Handlers

**Goal:** Register `pzep://` (or `zep://`) URI scheme; allow opening files/positions from external apps (e.g., terminal `pzep://open?file=...&line=10`).

### Implementation Plan

#### Phase 1 – URI Scheme Registration (Windows)
- Add installer or first-run registration:
  - Write to `HKEY_CLASSES_ROOT\pzep` and `HKEY_CURRENT_USER\Software\Classes\pzep` (per-user preferred).
  - Set `URL Protocol` = "" (empty string value) to register as protocol.
  - Set `shell\open\command` default value to `"<path>\pzep-gui.exe" "--uri" "%1"`.
- Provide `pzep-gui --register-uri` to register, and `--unregister-uri` to clean.

#### Phase 2 – URI Parsing & Handling
- Add `ZepEditor::HandleURI(const std::string& uri)` in `src/editor.cpp`.
- Parse query parameters: `file` (required), `line` (optional), `col` (optional), `buffer` (optional buffer name).
- Decode percent-encoding (use simple decoder).
- Open file (create buffer), then move cursor to `(line, col)` after load.
- Edge cases: file not found, line out of range.

#### Phase 3 – CLI Integration
- Parse `--uri <uri>` in main.cpp before normal startup; optionally `--background` to not show window (useful for background launchers).
- `pzep-gui --uri "pzep://open?file=C:/path/file.txt&line=5&col=2"`

**Dependencies:** Windows registry API (`RegCreateKeyEx`, `RegSetValueEx`).  
**Effort:** ~2 days.

---

## 4. Plugin/Extension System & Public API

**Goal:** Load third‑party modules (DLLs on Windows, `.so`/`.dylib` elsewhere) that can extend editor functionality (commands, modes, syntax highlighters).

### Design

#### Core Plugin Interface
```cpp
// include/zep/plugin.h
namespace Zep {
struct ZepPlugin {
    virtual ~ZepPlugin() = default;
    virtual void Init(ZepEditor& editor) = 0;            // Called after load
    virtual void Shutdown(ZepEditor& editor) = 0;        // Called before unload
    virtual const char* Name() const = 0;
    virtual const char* Version() const = 0;
};
} // namespace Zep
```

#### Plugin Loader
- Add `ZepPluginLoader` class in `src/plugin_loader.cpp` / `.h`.
- Discover plugins from `~/.pzep/plugins/` (subdirs or direct `.dll` files).
- Load each via `LoadLibrary` (Windows) / `dlopen` (POSIX).
- Resolve `ZepPlugin* CreatePlugin()` factory function.
- Call `Init(editor)`; keep `shared_ptr<ZepPlugin>` in registry.
- Provide `RegisterCommand`, `RegisterMode`, `RegisterSyntax` helper functions.
- Unload on exit (or hot‑reload optional).

#### Registration APIs
- `ZepEditor::RegisterExCommand(std::unique_ptr<ZepExCommand>)`
- `ZepEditor::RegisterMode(std::unique_ptr<ZepMode>)`
- `ZepEditor::RegisterSyntax(const std::string& ext, std::function<SyntaxStatePtr(const std::string&)> factory)`
- `ZepEditor::RegisterKeyBinding(EditorMode mode, const std::string& key, CommandId id)`

#### Lifecycle & Safety
- Plugins run in same process; a crashable plugin brings down editor. (Future: sandbox via separate process + IPC).
- Versioning: API version in `ZEP_PLUGIN_API_VERSION` macro; reject mismatched.
- Dependency: plugins link against `Zep.lib` (import library) and raylib if needed.

#### Manifest & Discovery
- Optional `plugin.json`/`plugin.toml` per plugin: `{ "name": "...", "version": "1.0", "entry": "mylib.dll" }`.
- Or implicit: all `.dll` in `plugins/` dir loaded.

#### Example Plugin (C++)
```cpp
extern "C" ZEP_API ZepPlugin* CreatePlugin() {
    return new MyPlugin();
}
```

**Dependencies:** Dynamic loading, cross‑platform abstraction layer (Windows `LoadLibrary` vs `dlopen`).  
**Effort:** ~1 week (loader + registration API + example + docs).

---

## 5. Language Server Protocol (LSP) Client

**Goal:** Integrate language intelligence ( diagnostics, completion, goto‑definition, hover, document symbols).

### Architecture

#### Phase 1 – LSP Core
- Add `src/lsp/` directory: `lsp_client.cpp`, `lsp_jsonrpc.cpp`, `lsp_types.cpp`.
- Implement JSON‑RPC 2.0 over stdio (simplest) or TCP (for remote servers).
- Use `nlohmann::json` (add as submodule) or `cpptoml`‑style JSON if available.
- `ZepLspClient` class: connect to server per workspace (or per file), send/receive messages, manage request/response IDs.

#### Phase 2 – Editor Integration
- Detect language from file extension / shebang → map to LSP server command (via config `lsp_servers` mapping).
- Launch server as child process (`CreateProcess`/`fork+exec`) with stdio pipes.
- Send `initialize`, `initialized`, `textDocument/didOpen` on buffer load.
- On edit, send `textDocument/didChange` (incremental or full).
- Handle server responses:
  - `textDocument/publishDiagnostics` → display squiggles / populate Problems panel.
  - `completion` → show completion popup (hook into existing UI).
  - `textDocument/definition` → jump to location.
  - `hover` → tooltip.
  - `textDocument/documentSymbol` → populate outline/symbol list.

#### Phase 3 – UI & UX
- Add **LSP panel** (dockable region) for diagnostics (like VS Code Problems).
- Inline diagnostics: underline errors/warnings; show hover on mouse.
- Completion popup: triggered by `<C-Space>` or auto after typing; use raylib list box.
- Goto definition: `<C-]>` or via context menu.
- Settings: `lsp.enabled = true`, `lsp.log = "~/.pzep/lsp.log"`.
- Per‑language server config: `lsp.server.cpp = "clangd"`, `lsp.server.python = "pylsp"`.

**Dependencies:** JSON parser, child process management, asynchronous message pump integration with raylib event loop.  
**Effort:** ~2–3 weeks (robust LSP is complex).

---

## 6. IME & Complex Script Shaping

**Goal:** Support input method editors (CJK, Indic, Thai) and complex text shaping (Arabic, Devanagari, etc.).

### Current Limitation
- raylib's text input (`GetCharPressed`, `GetKeyPressed`) delivers pre‑composed Unicode code points for simple scripts but no IME composition or glyph shaping.
- No `Imm*` API usage on Windows; no HarfBuzz integration.

### Implementation Options

#### Option A – Minimal Windows IME Support
- On Windows, enable IME via `ImmAssociateContext(hwnd, himc)` and handle `WM_IME_STARTCOMPOSITION`, `WM_IME_COMPOSITION`, `WM_IME_ENDCOMPOSITION`.
- Use `ImmGetCompositionString` to retrieve composed string.
- In raylib, patch its Win32 event loop to forward IME messages to pzep; provide composition string overlay at cursor.
- Still need text shaping: for Arabic/Indic, use **Harfbuzz** + **Fribidi** (optional submodule).
  - Convert UTF‑8 → HarfBuzz buffer → shape → glyph indices.
  - Use raylib's `Font` with custom glyph loading (need to populate font atlas with shaped glyphs; raylib's `LoadFontEx` supports codepoints but not complex clusters).

#### Option B – Switch to Higher‑level Text Input
- Move editing surface to a native widget (e.g., Windows RichEdit control) hosted via raylib's `SetWindowState`? Not feasible; raylib owns window.
- Replace raylib display backend entirely (massive effort).

#### Recommendation
Start with **Option A** for IME on Windows only; defer complex shaping to later. Provide a `--disable-ime` flag.

**Effort (IME only):** ~1 week.  
**Effort (IME + shaping):** ~2–3 weeks + HarfBuzz dependency.

---

## 7. Native Dialogs & System Integration

**Goal:** Use OS-native file open/save, print, and about dialogs; system‑level features (recent files, file associations).

### 7.1 Native File Dialogs
- Replace any custom file browser (none currently; pzep uses `:e` path typing) with **native common dialogs**.
- Windows: `GetOpenFileName`, `GetSaveFileName` (`OPENFILENAME` struct).
- Add `ZepDisplay::OpenFileDialog(const char* filter)` and `SaveFileDialog()`.
- Use in `commands_file.cpp` for `:e`, `:w`, `:r`, `:saveas`.
- Filters: `"Text Files\0*.txt;*.md;*.cpp;*.h;*.py\0All Files\0*.*\0"`.

### 7.2 File Associations
- Register `.pzp` (or `.zep`) extension as pzep project files.
- Create file type association in Windows Registry:
  - `HKEY_CLASSES_ROOT\.pzp` → `pzepfile`
  - `HKEY_CLASSES_ROOT\pzepfile\shell\open\command` → `"<path>\pzep-gui.exe" "%1"`
- Installer should handle this; also provide `--associate` CLI.

### 7.3 Native About/Info Dialog
- Already using `MessageBoxA`; could use `TaskDialogIndirect` for richer UI.

### 7.4 System Menus & Accelerators
- Add standard **Edit** menu (Undo/Redo/Cut/Copy/Paste/Select All) with system‑standard shortcuts; use raylib's `GuiTextBox`? Instead implement top menubar region that forwards to editor commands.
- Window system menu (icon in top‑left) → add "About", "Settings", "Exit".

**Dependencies:** Win32 API calls encapsulated in `src/native_win.cpp` (or platform‑specific files).  
**Effort:** ~3–4 days.

---

## 8. Performance Tooling

**Goal:** Built‑in profiling, diagnostics, and benchmarking for editor operations; large‑file testing infrastructure.

### 8.1 Internal Profiling Hooks
- Add `ZepProfiler` singleton; scoped timers (`ZepProfileScope(__FUNCTION__)`).
- Track per‑operation times: `Display()`, `UpdateLayout()`, `Undo/Redo`, `SyntaxParse`, `LSP request`, file load.
- UI: Toggle with `--profiler` or `:profile start/stop/dump`.
- Output: plain text or JSON (`~/.pzep/profile_<timestamp>.json`).
- Use `std::chrono` (high‑resolution clock). Minimal overhead when disabled (compiler‑time flag).

### 8.2 Memory & Layout Diagnostics
- `:stats` command: show buffer size, line count, layout cache hits/misses, glyph cache memory, visible line count, undo stack size.
- `:layout` debug overlay (draw bounding boxes for regions).

### 8.3 Large File Tests & Benchmarks
- Add `tests/benchmarks/` directory with script to open 10 MB, 100 MB, 1 GB files; measure scroll latency, search responsiveness.
- Automated harness (Python or CMake custom command) recording metrics to CSV.
- Establish performance budgets (e.g., 60 FPS on 10K lines, <100 ms layout on 100K lines).

### 8.4 Frame Time & GPU Stats
- Expose raylib's internal metrics: `GetFPS()`, frame time, draw calls count.
- Display in status line when `debug.show_fps = true` (config option).

**Effort:** ~4–5 days.

---

## Cross‑Cutting Concerns

### Build & CMake
- Introduce `BUILD_PLUGINS`, `ENABLE_LSP`, `ENABLE_IME`, `ENABLE_SYSTEM_TRAY` options in top‑level `CMakeLists.txt`.
- Conditional dependencies: `find_package(nlohmann_json)` for LSP; `find_package(HarfBuzz)` for shaping.
- Platform guards: `if(WIN32)`, `if(APPLE)`, `if(UNIX)`.

### Configuration
- Extend `~/.pzep/.pzeprc` with sections:
```toml
[ui]
max_recent_files = 20
show_tray_icon = true

[plugin]
dirs = [ "~/.pzep/plugins" ]
enable = true

[lsp]
enable = true
log_file = "~/.pzep/lsp.log"
servers = { cpp = "clangd", python = "pylsp", javascript = "typescript-language-server" }

[ime]
enable = true  # Windows only

[performance]
enable_profiler = false
show_fps = false
```

### Error Handling & Logging
- Centralized logger already exists; ensure plugin and LSP errors are captured.
- Plugin errors → message box / command line notification, not crash.

### Testing Strategy
- Unit tests: `tests/unit/` for MRU eviction, URI parsing, JSON‑RPC framing.
- Integration tests: `tests/integration/` that launch pzep-gui with test data (headless via `--headless` future flag).
- Manual QA checklist per OS.

---

## Effort Summary (Person‑Days)

| Feature | Phase 1 | Phase 2 | Phase 3 | Total (days) |
|---------|---------|---------|---------|--------------|
| Recent Files | 1 | 1 | 1 | 3 |
| System Tray | 1 | 1 | 1 | 3 |
| URI Handlers | 1 | 1 | 0 | 2 |
| Plugin System | 2 | 2 | 1 (docs) | 5 |
| LSP Client | 5 | 5 | 3 (UI) | 13 |
| IME/Shaping | 3 (IME) | 2 (shaping) | 1 (config) | 6 |
| Native Dialogs | 1 | 1 | 1 | 3 |
| Performance Tooling | 2 | 1 | 1 | 4 |
| **Sum** | | | | **39 days** (~8 weeks) |

*Note*: Parallelization possible (different engineers work on independent features).

---

## Risks & Mitigations

- **Raylib limitations:** Tray/URI registration require native API calls; raylib doesn't block but we must manage platform code ourselves.
- **LSP complexity:** Large surface area; start with subset (diagnostics + completion) and iterate.
- **IME/shaping:** Platform‑specific; Windows first, defer macOS/Linux to later phase.
- **Plugin ABI stability:** Use opaque pointers and semantic versioning; provide deprecation cycle.
- **Performance:** LSP and syntax parsing can be heavy; run on background threads, batch updates.

---

## Conclusion

This plan transforms pzep from a prototype into a production‑grade editor with native OS integrations, extensibility, and language intelligence. Implementation should follow phased milestones with frequent releases, collecting user feedback on each integration before moving to the next.
