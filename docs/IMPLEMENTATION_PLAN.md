# Zep Implementation Plan (2026)

**Based on**: Deep Technical Analysis (docs/REPORT.md, docs/SECURITY_REPORT.md)  
**Date**: 2026-04-23  
**Priority**: Critical security, performance, architecture  
**Scope**: Continued development addressing technical debt and missing features

---

## Phase 0: Critical Security Fixes (IMMEDIATE)

### 0.1 REPL Sandbox Security - Capability-Based sandbox model
**Severity**: Medium (Potential code injection)  
**File**: `src/mode_lua_repl.cpp`, `src/mode_duktape_repl.cpp`  
**Issue**: Direct raw C++ pointer exposure to Lua scripts; no capability boundaries

**Current State**:
- Lua gets direct `ZepEditor*` and `ZepBuffer*` userdata (lines 316-320, 22-37)
- Scripts can call any C++ method, including potentially dangerous ones
- Janet REPL (original) has same design flaw

**Implementation Tasks**:
1. Create `ReplCapability` interface and registry system
2. Implement `BufferCapability` wrapper with controlled API:
   - Allowed: GetName, GetLength, GetLineCount, GetLineText, GetCursor, IsModified
   - Denied: Direct file access, buffer mutation (initially read-only)
3. Implement `EditorCapability` wrapper with controlled API:
   - Allowed: GetActiveBuffer, GetBuffers (read-only)
   - Denied: TabWindow manipulation, display changes
4. Update Lua provider to expose capabilities instead of raw pointers ( lines 22-37, 254-272, 316-320)
5. Update Duktape provider similarly (lines ~36-61, ~196-221)
6. Add capability checks and audit logging
7. Document capability model in security docs

**Success Criteria**:
- No raw C++ pointers accessible from Lua/JS
- Capability registry with deny-by-default
- Audit log of capability grants
- Tests verify restricted API surface

**Estimated Effort**: 2 days

---

## Phase 1: Performance Critical Fixes (Week 1)

### 1.1 ZepWindow::UpdateLineSpans() - Incremental Layout Recalculation
**Severity**: High (O(B*C) performance)  
**File**: `src/window.cpp:527-669+`  
**Lines**: ~1200+ lines total across function  
**Impact**: Called on every edit; O(bufferLines × charsPerLine) → catastrophic for large files

**Current Behavior**:
- Clears entire `m_windowLines` vector every call (line 544)
- Recalculates ALL spans from scratch for ALL lines
- No caching or dirty tracking

**Implementation Tasks**:

1. **Add dirty region tracking** (new structs):
   ```cpp
   struct DirtyRegion {
       long startBufferLine;
       long endBufferLine;
       bool wrapChanged = false;
   };
   struct LineSpanCache {
       std::vector<SpanInfo> cachedSpans;
       bool isValid = false;
   };
   ```
   Add to `ZepWindow` class: `m_dirtyRanges`, `m_spanCache`

2. **Track edits** via message system:
   - Extend `ZepMessage` types: `MsgType::BufferTextChanged`, `MsgType::BufferLineSplit`, `MsgType::BufferLineJoin`
   - Add handler in `ZepWindow::Notify()` to record dirty line ranges
   - Compute affected screen lines from buffer line deltas
   - Mark wraps as dirty when window width changes

3. **Implement partial UpdateLineSpans()**:
   - If dirty region present, calculate affected span line indices
   - Instead of clearing `m_windowLines`, update in-place for dirty spans
   - For line splits/joins: `std::vector::erase`/`insert` on span vectors (O(n) but rare)
   - For wrap changes: recompute only visual lines that wrapped differently
   - For text edits: update only spans overlapping changed byte range

4. **Add span cache invalidation**:
   - Cache syntax highlighting spans per line (already done by `SyntaxProvider`? verify)
   - Cache font metrics per line
   - Invalidate on theme change

5. **Add line widget caching**:
   - `ArrangeLineMarkers()` currently called on every line
   - Cache widget arrangement by line number, invalidate on marker change

6. **Benchmark before/after**:
   - Add timing metrics (already using TIME_SCOPE)
   - Create test script: 10k line file, random insert, measure
   - Document improvement (target: O(changedLines) vs O(allLines))

**Files to Modify**:
- `include/zep/window.h`: Add dirty range tracking fields, cache
- `src/window.cpp:527`: Refactor UpdateLineSpans() with early-exit path
- `src/window.cpp`: Add Notify() handler for buffer change messages

**Success Criteria**:
- Edit response time < 16ms for 10k line files on reference hardware
- Profiling shows linear scaling with changed lines, not total lines
- No visual glitches in wrap scenarios

**Estimated Effort**: 3 days

---

### 1.2 ZepMode::AddKeyPress() - Elimination of Per-Keystroke Heap Allocation
**Severity**: High (allocation pressure)  
**File**: `src/mode.cpp`  
**Issue**: `CommandContext` heap-allocated every keypress via `std::make_shared`

**Implementation Tasks**:
1. Change `ZepMode::AddKeyPress` signature:
   ```cpp
   // OLD: std::shared_ptr<CommandContext> AddKeyPress(uint32_t key, uint32_t modifiers);
   // NEW: CommandContext AddKeyPress(uint32_t key, uint32_t modifiers);
   ```
   Make `CommandContext` a stack-allocated struct.

2. Refactor all mode classes (ZepMode_Vim, ZepMode_Standard) to accept `CommandContext&` instead of `shared_ptr`

3. Update call sites in `src/editor.cpp`, `src/tab_window.cpp` where `AddKeyPress` is called

4. Ensure thread-local `CommandContext` storage if context needs to persist across async operations

5. Benchmark memory allocations with valgrind massif or Windows Performance Analyzer

**Files to Modify**:
- `include/zep/mode.h`: Change AddKeyPress return type and CommandContext definition
- `src/mode.cpp`: Update implementation
- `src/mode_vim.cpp`, `src/mode_standard.cpp`: Update overrides
- All command implementations that accept CommandContext

**Success Criteria**:
- Zero heap allocations per keystroke (verified via allocation profiler)
- No regression in command execution behavior
- Parallel mode operations unaffected

**Estimated Effort**: 1 day

---

## Phase 2: Thread Safety & Concurrency (Week 1-2)

### 2.1 Enable and Fix Threaded Tests
**Severity**: Medium (threading untested)  
**Files**: All test files

**Current State**:
Tests compiled with `ZEP_DISABLE_THREADS` flag; production threading effectively untested

**Implementation Tasks**:
1. Remove `DisableThreads` flag from CMake test configuration:
   ```cmake
   # In tests/CMakeLists.txt
   target_compile_definitions(zep_tests PRIVATE ZEP_DISABLE_THREADS)  # REMOVE
   ```

2. Fix threadpool race condition (`src/mcommon/threadpool.h`):
   ```cpp
   // Add mutex protection
   std::mutex m_queueMutex;
   std::queue<std::function<void()>> m_tasks;
   
   void enqueue(std::function<void()> task) {
       std::lock_guard<std::mutex> lock(m_queueMutex);
       m_tasks.push(std::move(task));
   }
   ```

3. Audit `ZepBuffer` methods for thread safety:
   - Identify which methods are thread-safe (const) vs. mutable
   - Add `std::shared_mutex` for read/write locking in buffer
   - Document thread-safety contract in `buffer.h` comments

4. Add thread-safety tests:
   - Concurrent buffer edits from multiple threads (stress test)
   - Concurrent buffer reads from multiple threads
   - Verify undo/redo stack consistency under concurrency
   - Test REPL evaluation in background thread

5. Fix any discovered race conditions (likely many)

**Success Criteria**:
- All tests pass with threading enabled
- ThreadSanitizer (TSAN) clean on Linux/macOS builds
- Documented thread-safety guarantees per class

**Estimated Effort**: 2-3 days

---

## Phase 3: Missing REPL Implementation (Week 2)

### 3.1 QuickJS Provider Implementation
**Severity**: High (feature incomplete)  
**File**: `src/mode_quickjs_repl.cpp` (currently stub)  
**Current State**: Returns `<not implemented>` string; 0% functional

**Implementation Tasks**:
1. Check QuickJS availability:
   - Try vcpkg: `vcpkg install quickjs`
   - If unavailable, bundle QuickJS single-file runtime in `third_party/quickjs/`
   - Add CMake option `ENABLE_QUICKJS_REPL` similar to Lua/Duktape

2. Implement QuickJSReplProvider class:
   - Structure parallel to LuaReplProvider (lines 276-440)
   - Initialize JSRuntime, JSContext
   - Implement sandbox (similar to Lua dangerous global removal)
   - Remove dangerous globals: `require`, `Module`, `process`, `eval`, `Function` constructor
   - Expose same capability-based API (Buffer, Editor wrappers)

3. Bridge API for QuickJS:
   - Convert C++ objects to JS objects via `JS_NewObject`
   - Create Buffer object with methods: getName, getLength, getLineCount, getLineText, getCursor, isModified
   - Create Editor object with methods: getActiveBuffer, getBuffers
   - Note: QuickJS uses C API; need careful memory management

4. Output capture:
   - Redirect `console.log`, `print` to capture output
   - Use `JS_SetProperty` to override print functions

5. Register provider:
   - Add to `apps/pzep-gui/main.cpp` conditional on CMake option
   - Add factory functions (`CreateQuickJSReplProvider`, `DestroyQuickJSReplProvider`)

6. Functional testing:
   - Verify QuickJS can execute simple expressions
   - Verify capability restrictions (console.log works, require blocked)
   - Verify buffer read operations

**Files to Create/Modify**:
- `src/mode_quickjs_repl.cpp` (implement)
- `include/zep/mode_repl.h` (declare)
- `src/CMakeLists.txt` (conditional QuickJS source)
- `apps/pzep-gui/main.cpp` (conditional registration)
- `docs/quickjs_scripting.md` (update with QuickJS differences)

**Success Criteria**:
- QuickJS provider fully functional with same API as Lua/Duktape
- All QuickJS unit tests pass
- No runtime crashes or memory leaks (Valgrind clean)

**Estimated Effort**: 2 days

---

## Phase 4: Memory Management & Ownership (Week 2-3)

### 4.1 Standardize Ownership Model - Smart Pointers Only
**Severity**: Medium (risk of leaks/double-frees)  
**Files**: `src/editor.cpp`, `src/window.cpp`, `src/tab_window.cpp`, multiple

**Current State**: Mixed raw pointers (owned), `shared_ptr`, `unique_ptr` with unclear ownership boundaries

**Implementation Tasks**:

1. **Audit all pointer members**:
   - Search for `* m_pXxx` raw pointers in class definitions
   - Document ownership per pointer: owned, non-owning observer, shared

2. **Replace owned raw pointers with `std::unique_ptr`**:
   ```cpp
   // Before
   ZepDisplay* m_pDisplay = nullptr;
   // After
   std::unique_ptr<ZepDisplay> m_pDisplay;
   ```
   Files: `src/editor.cpp:136-138` (delete calls → unique_ptr destructor)
   `src/tab_window.cpp` (raw pointer arrays → vector<unique_ptr>)

3. **Replace shared owning pointers with `std::shared_ptr`** (keep):
   - Buffers (`std::vector<std::shared_ptr<ZepBuffer>>` - this is fine)
   - Themes, syntax (fine as shared)

4. **Convert raw observer pointers to `std::weak_ptr` or raw (non-owning)**:
   - `ZepMode::m_pEditor` → should be raw non-owning (editor owns mode)
   - `ZepWindow::m_pBuffer` → raw (window doesn't own buffer, editor does)
   - `ZepTabWindow::m_pActiveWindow` → raw (tab owns children)

5. **Remove manual `delete` everywhere** (search `delete `):
   - `editor.cpp:136` loop deleting tab windows → replace with `std::vector<std::unique_ptr<ZepTabWindow>>`
   - `editor.cpp:138` `delete m_pDisplay` → unique_ptr
   - `window.cpp`: any delete calls (find them)
   - `buffer.cpp`: check for manual deletes in destructors

6. **Update construction sites** to use `std::make_unique` / `std::make_shared`

7. **Update all raw pointer parameters**:
   - Parameters that transfer ownership: pass `std::unique_ptr<T>`
   - Parameters that borrow: pass `T*` or `T&` (const as appropriate)

8. **Run static analysis**:
   - Enable clang-tidy `modernize-use-unique-pointer`, `modernize-use-shared-pointer`
   - Fix all warnings

**Files to Audit/Modify** (non-exhaustive):
- `src/editor.cpp`, `include/zep/editor.h`
- `src/window.cpp`, `include/zep/window.h`
- `src/tab_window.cpp`, `include/zep/tab_window.h`
- `src/buffer.cpp`, `include/zep/buffer.h`
- `src/mode*.cpp`, `include/zep/mode.h`
- `src/display.cpp`, `include/zep/display.h`

**Success Criteria**:
- Zero manual `delete` calls in production code (test code OK)
- clang-tidy modernize checks pass
- valgrind --leak-check=full reports no leaks
- clear ownership documented in code comments

**Estimated Effort**: 3 days

---

## Phase 5: Architectural Improvements (Week 3-4)

### 5.1 ZepEditor Decomposition - Extract Subsystems
**Severity**: High (God Object: 43KB, 1200+ lines)  
**File**: `src/editor.cpp`  
**Issue**: `ZepEditor` handles everything: buffers, modes, tabs, display, themes, syntax, undo, indexer, git, file I/O, config

**Strategy**: Incremental extraction with facade pattern to avoid breaking API

**Implementation Tasks**:

1. **Extract `ZepBufferManager` subsystem**:
   ```cpp
   class ZepBufferManager {
       std::vector<std::shared_ptr<ZepBuffer>> m_buffers;
       std::shared_ptr<ZepBuffer> CreateBuffer(const std::string& name, FileFlags flags);
       std::shared_ptr<ZepBuffer> GetBuffer(const std::string& name) const;
       void CloseBuffer(ZepBuffer* buf);
   };
   ```
   - Move buffer creation/deletion/query from ZepEditor
   - ZepEditor forwards: `GetBuffers()`, `GetActiveBuffer()`, `CreateBuffer()` etc.
   - Pass `ZepBufferManager&` to subsystems that need it (Mode, TabWindow)

2. **Extract `ZepModeManager` subsystem**:
   ```cpp
   class ZepModeManager {
       std::unique_ptr<ZepMode> m_activeMode;
       std::map<std::string, std::function<std::unique_ptr<ZepMode>()>> m_modeFactories;
       void RegisterMode(const std::string& name, factory);
       void SetActiveMode(const std::string& name);
   };
   ```
   - Move mode switching, key dispatch from ZepEditor
   - Modes no longer directly owned by ZepEditor

3. **Extract `ZepTabWindowManager` subsystem**:
   ```cpp
   class ZepTabWindowManager {
       std::vector<std::unique_ptr<ZepTabWindow>> m_tabWindows;
       ZepTabWindow* GetActiveTabWindow() const;
       ZepTabWindow* AddTabWindow();
   };
   ```
   - All tab window operations move from ZepEditor

4. **Update ZepEditor**:
   - Hold subsystems as members (`m_bufferMgr`, `m_modeMgr`, `m_tabMgr`)
   - ZepEditor becomes facade: `ZepEditor::GetBuffers()` → `m_bufferMgr.GetBuffers()`
   - Reduce ZepEditor from 1200 lines → ~200 lines of forwarding

5. **Update callers**: `main.cpp`, tests, REPL providers to access via editor

6. **Document new architecture** in docs/ARCHITECTURE.md

**Files to Create**:
- `include/zep/buffer_manager.h`, `src/buffer_manager.cpp`
- `include/zep/mode_manager.h`, `src/mode_manager.cpp`
- `include/zep/tab_window_manager.h`, `src/tab_window_manager.cpp`

**Files to Modify**:
- `include/zep/editor.h`, `src/editor.cpp` (decompose)
- `src/tab_window.cpp` (adjust construction)
- `apps/pzep-gui/main.cpp` (update usage)
- `src/mode_*.cpp`, `src/display.cpp`, etc. (update dependencies)

**Success Criteria**:
- ZepEditor ≤ 300 lines
- All tests pass
- No performance regression
- Clear ownership: subsystems own their objects

**Estimated Effort**: 4 days

---

### 5.2 ZepComponent Auto-Registration Removal
**Severity**: Medium (circular dependency risk)  
**File**: `src/component.cpp`  
**Issue**: `ZepComponent` constructor auto-registers with `ZepEditor` creating lifecycle coupling

**Implementation Tasks**:
1. Remove auto-registration from `ZepComponent` constructor
2. Change `ZepComponent` to abstract base class with pure virtual `Initialize(ZepEditor&)`
3. All components (ZepMode, ZepExCommand, ZepSyntax, etc.) must explicitly call `RegisterComponent()` after construction
4. Update all component subclasses:
   - `ZepMode` subclasses (Vim, Standard, Search, FileTree, Repl)
   - All `ZepExCommand` subclasses
   - `ZepSyntax` providers
   - `ZepDisplay` subclasses (if applicable)
5. Update `ZepEditor::RegisterMode()`, `RegisterExCommand()` to not rely on auto-registration

**Files to Modify**:
- `include/zep/component.h`, `src/component.cpp`
- All files with component subclasses (~20 files)

**Success Criteria**:
- No circular dependency between ZepComponent and ZepEditor
- Explicit initialization sequence clear
- Tests pass

**Estimated Effort**: 1 day

---

## Phase 6: Missing Core Features (Week 4-5)

### 6.1 FTXUI Backend - Complete or Remove
**Severity**: Low (incomplete stub)  
**Files**: `include/zep/display_ftxui.h`, `src/display_ftxui.cpp`

**Implementation Decision**: Remove or deprecate (if no plans to complete)

**Alternative**: If user requires FTXUI, implement minimal:
1. Basic text rendering with FTXUI `Canvas` API
2. Simple cursor and selection
3. No syntax highlighting initially

**Recommendation**: Remove to reduce maintenance burden  
**Action**: Delete FTXUI display files, update CMake to exclude

**Estimated Effort**: 0.5 days (removal) / 5 days (implementation)

---

### 6.2 MARKER: Address Top-Priority TODOs
**Severity**: Low-Medium (61 TODOs)  
**Files**: All

**Targeted TODO Fixes** (not all, only high-impact ones):

1. `buffer.cpp:767` - Newline replacement performance (UNDO RELATED)
2. `buffer.cpp:1203` - UTF-8 broken (high priority)
3. `mode_search.cpp:104` - UTF-8 not handled
4. `window.cpp:648` - Performance optimization note (already addressed in 1.1?)

**Success Criteria**: Critical UTF-8 bugs fixed, documented

**Estimated Effort**: 2 days

---

## Phase 7: Documentation & Testing (Week 5)

### 7.1 Update Architecture Documentation
Create `docs/ARCHITECTURE.md` documenting:
- New subsystem decomposition (Phase 5)
- Capability-based REPL security model (Phase 0)
- Thread-safety guarantees per class
- Ownership model (smart pointers only)
- Performance characteristics (UpdateLineSpans incremental)

### 7.2 Update Security Documentation
Update `docs/SECURITY_REPORT.md`:
- Mark REPL pointer exposure FIXED
- Document remaining threat model
- Add capability model description

### 7.3 Update Scripting Documentation
Update `docs/*_scripting.md`:
- Note capability restrictions (read-only buffer access, no system calls)
- Document QuickJS API differences
- Add security best practices for embedding pZep

### 7.4 Add Unit Tests for REPL Providers
**Files**: `tests/repl_test.cpp` (new)

Test coverage:
- Lua: Buffer read methods, editor methods, sandbox violations (os.remove blocked)
- Duktape: Same
- QuickJS: Same
- REPL evaluation modes (Line, Outer, Inner) across all providers
- Sandbox escape attempts via metatable manipulation

---

## Phase 8: Build & Deployment (Final)

### 8.1 Fix Duktape vcpkg Integration
**File**: Build system  
**Issue**: Duktape library not found; manual install path unclear

**Implementation**:
1. Verify Duktape vcpkg port works: `vcpkg install duktape`
2. If broken, bundle Duktape as `third_party/duktape/duktape.c` (single-file library)
3. Update CMake to prefer bundled if vcpkg fails

### 8.2 CI/CD Verification
Ensure all CMake options build on CI:
- `-DENABLE_LUA_REPL=ON`
- `-DENABLE_DUKTAPE_REPL=ON`
- `-DENABLE_QUICKJS_REPL=ON`

Add Windows, Linux, macOS jobs for each combination.

---

## Execution Order & Dependencies

```
Phase 0 (Security)  → must precede all others
Phase 1 (Performance) → depends on Phase 0
Phase 2 (Threading)  → depends on Phase 1 (reduces race window)
Phase 3 (QuickJS)    → depends on Phase 0 (same sandbox model)
Phase 4 (Ownership)  → independent, can run parallel with 1-3
Phase 5 (Decomposition) → depends on Phase 4 (smart ptrs simplify)
Phase 6 (Missing Features) → depends on Phase 3 (QuickJS), can be parallel
Phase 7 (Docs/Tests) → depends on all above
Phase 8 (Build) → final polish
```

**Suggested Implementation Schedule**:

**Week 1**: Phase 0 (Security) + Phase 1.1 (UpdateLineSpans)  
**Week 2**: Phase 1.2 (AddKeyPress) + Phase 2 (Threading) + Phase 3 (QuickJS start)  
**Week 3**: Phase 4 (Ownership) + Phase 3.1 (QuickJS finish)  
**Week 4**: Phase 5 (Decomposition) + Phase 6.2 (TODO fixes)  
**Week 5**: Phase 7 (Docs/Tests) + Phase 8 (Build fixes)

---

## Metrics & Verification

**Performance**:
- `UpdateLineSpans` latency: <16ms for 10k line file (baseline: 200ms)
- Keystroke allocation: 0 heap allocations (baseline: 1 per keystroke)
- Memory footprint: ≤ baseline

**Code Quality**:
- 0 manual `delete` calls
- 0 raw owning pointers
- 0 FIXME/TODO comments remaining in critical paths
- clang-tidy score ≥ 95%

**Security**:
- Zero raw pointer exposure from REPL
- Capability audit log verifiable
- TSAN clean + Thread safety documented

**Completeness**:
- All 3 REPL providers functional
- All tests passing
- Full documentation updated

---

## Risk Mitigation

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Decomposition breaks API | High | Medium | Maintain facade, exhaustive tests |
| UpdateLineSpans partial recalculation edge-cases | High | Medium | Extensive visual regression tests |
| Thread safety bugs latent | Medium | High | Enable TSAN, run stress tests |
| Duktape library unavailable | Medium | Low | Bundle fallback |
| QuickJS API complexity | Medium | Medium | Study qjs examples, incremental implementation |

---

## Acceptance Criteria

**Definition of Done for Each Phase**:
1. Implementation complete in code
2. All affected tests passing
3. Documentation updated
4. Performance benchmark meets target (if applicable)
5. Code review completed (via Kilo suggest)
6. Changes committed to feature branch

**Final Acceptance**:
- All phases 0-8 complete
- No regression in existing functionality
- CI green on all platforms
- Technical debt reduced from 61 TODOs to ≤ 15 non-critical

---

## Notes

- Commits should be atomic per phase/task
- Branches: `feature/security-capabilities`, `feature/incremental-layout`, `feature/smart-ptrs`, etc.
- Use feature flags for incomplete work during development
- Update `kilo.json` instructions as features land
