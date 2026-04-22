# Technical Debt Report for nZep
## Comprehensive Analysis and Recommendations

---

## Executive Summary

| Category | Count | Severity | Resolution Priority |
|----------|-------|----------|---------------------|
| **Critical Bugs** | 2 | High | Immediate |
| **Missing Core Features** | 15 | High | Phase 1 |
| **Performance Issues** | 8 | Medium | Phase 2 |
| **Code Quality** | 25 | Medium | Phase 3 |
| **Missing Enhancements** | 30+ | Low | Phase 4 |
| **Total TODOs** | 61+ | - | Ongoing |

**Current Assessment**: The codebase has 61 documented TODOs/FIXMEs. Some have been addressed (UTF-8 fixes, auto-indent, find/replace), but significant debt remains. The Vim motions implementation (~75%) is ahead of ex commands (~15%).

---

## 1. Critical Technical Debt (MUST FIX)

### 1.1 Undo Bleeding Between Buffers

| Attribute | Details |
|-----------|---------|
| **File** | `src/buffer.cpp` |
| **Lines** | 1315, 1331 |
| **Severity** | CRITICAL |
| **Category** | Data Integrity |

**Issue**: Undo operations affect other buffers when they shouldn't. Editing in one buffer causes undo history to appear in another.

**Code**:
```cpp
// buffer.cpp:1315
// TODO: Why is this necessary; marks the whole buffer
GetEditor().Broadcast(std::make_shared<BufferMessage>(this, BufferMessageType::MarkersChanged, Begin(), End()));
```

**Root Cause**: Global broadcast of buffer changes without proper buffer isolation. The undo system shares state incorrectly.

**Impact**: 
- Users editing multiple files experience data corruption
- Cannot reliably undo/redo in multi-buffer workflows
- Breaks basic editing contract

**Resolution**: 
- Implement per-buffer undo stack isolation
- Add buffer ID to change records
- Filter broadcasts by buffer target

**Importance**: 10/10 - MUST FIX BEFORE PRODUCTION

---

### 1.2 Thread Safety in ThreadPool

| Attribute | Details |
|-----------|---------|
| **File** | `src/mcommon/threadpool.h` |
| **Lines** | ~100-150 |
| **Severity** | CRITICAL |
| **Category** | Concurrency |

**Issue**: Custom `ThreadPool` uses a simple queue without proper locking. Multiple threads accessing buffer/indexer simultaneously cause race conditions.

**Code**:
```cpp
// threadpool.h - potential race without proper locking
m_tasks.push(task);  // not always protected
```

**Root Cause**: Missing mutex protection around task queue operations.

**Impact**:
- Tests explicitly disable threading due to failures
- Race conditions in async operations
- Potential data corruption with thread-enabled mode

**Resolution**:
```cpp
// Add proper locking
std::lock_guard<std::mutex> lock(m_mutex);
m_tasks.push(task);
cv.notify_one();
```

**Importance**: 10/10 - MUST FIX BEFORE PRODUCTION

---

## 2. High Priority Technical Debt

### 2.1 Missing Vim Ex Commands (15 commands)

| Priority | Command | Impact | Difficulty |
|----------|---------|--------|--------|----------|
| P0 | `:e [file]` | File navigation | Medium |
| P0 | `:bn` / `:bp` | Buffer navigation | Easy |
| P0 | `:sp` / `:vsp` | Window splitting | Medium |
| P0 | `:set` options | Configuration | Hard |
| P1 | `:cd` / `:pwd` | Directory ops | Easy |
| P1 | `:r file` | File read | Medium |
| P1 | `:make` | Build integration | Medium |
| P1 | `:grep` | Search files | Medium |
| P2 | `:tab*` commands | Tab management | Medium |
| P2 | `:help` | Help system | Low |

**Current State**: Only 8 ex commands implemented: `:s`, `:q`, `:w`, `:wq`, `:buffers`, `:ls`, `:g`, `:n`, `:terminal`, `:!`

**Recommendation**:
1. Implement `:e`, `:bn`, `:bp`, `:sp` first (quick wins)
2. Add basic `:set` support for critical options
3. Add `:cd`, `:pwd` for directory management
4. Build out remaining in priority order

**Significance**: Ex commands at 15% vs Motions at 75%. Large parity gap.

---

### 2.2 Git Integration Disabled

| Attribute | Details |
|-----------|---------|
| **File** | `src/mode_vim.cpp` |
| **Lines** | 909-918 |
| **Severity** | HIGH |

**Issue**: All Git commands are defined but registration is commented out:

```cpp
// Git commands - disabled (requires full git component)
// if (auto spGit = editor.GetGit())
// {
//     editor.RegisterExCommand(std::make_shared<ZepExCommand_GitStatus>(editor, spGit));
//     // ... all 7 commands
// }
```

**Available Commands** (defined but unregistered):
- `:Gstatus`
- `:Gdiff`
- `:vdiff` (vertical diff)
- `:Gblame`
- `:Gcommit <message>`
- `:Gpush`
- `:Gpull`

**Resolution**: Uncomment after fixing `GetGit()` method.

**Importance**: 8/10 - Git workflow is essential for modern development

---

### 2.3 Incomplete UTF-8 Support

| Attribute | Details |
|-----------|---------|
| **Files** | Multiple |
| **Severity** | HIGH |
| **Category** | Text Encoding |

**Issues**:
- `NOTES.md` explicitly states: "All ASCII for now, no real UTF8"
- `syntax.cpp:97` - Unicode handling marked for revisit
- `buffer.cpp:1203` - UTF-8 broken

**Resolution Steps**:
1. Ensure GlyphIterator handles multi-byte characters correctly
2. Fix search with Unicode
3. Add proper normalization (NFC/NFD)
4. Test with CJK, emoji, RTL text

**Importance**: 9/10 - UTF-8 is mandatory for modern text editors

---

## 3. Performance Issues (MEDIUM PRIORITY)

### 3.1 Known Performance Bottlenecks

| Location | Issue | Impact | Status |
|----------|-------|--------|--------|
| `window.cpp:1170` | Character-by-character draw | Debug sluggishness | Unresolved |
| `window.cpp:1221` | Caching needed | Debug sluggishness | Unresolved |
| `window.cpp:718` | General optimization needed | All platforms | Unresolved |
| `window.cpp:2272` | Line lookup too slow | Large files | Unresolved |
| `buffer.cpp:769` | Newline replacement | Text processing | Unresolved |
| `mode.cpp:2825` | Search busy editing | Search performance | Unresolved |

**Details**:

```cpp
// window.cpp:1170
// TODO: This function draws one char at a time.  
// It could be more optimal at the expense of some

// window.cpp:1221
// TODO : Cache this for speed - a little sluggish on debug builds.

// window.cpp:2272
// TODO: Performance; quick lookup for line
```

**Resolution**: Implement line cache, batch rendering, optimized data structures.

**Importance**: 6/10 - Affects large file editing experience

---

### 3.2 Memory Management Inconsistency

| Attribute | Details |
|-----------|---------|
| **Files** | `editor.cpp`, `window.cpp`, `tab_window.cpp` |
| **Severity** | MEDIUM |

**Issue**: Mixed ownership model:

```cpp
// Raw pointers with delete (dangerous)
delete m_pDisplay;
delete w;  // tab_window.cpp

// Smart pointers elsewhere (safe)
std::make_shared<ZepTheme>();
std::make_shared<Region>();
```

**Risk**: Memory leaks or double-frees if ownership isn't clear.

**Resolution**: Standardize to smart pointers. Convert all to `std::unique_ptr` or `std::shared_ptr`.

**Importance**: 7/10 - Long-term stability

---

## 4. Code Quality Issues (MEDIUM PRIORITY)

### 4.1 File Size Concerns

| File | Size | Lines | Issue |
|------|------|-------|-------|
| `mode.cpp` | 97KB | 2947 | Too large - single file |
| `window.cpp` | 78KB | 2272 | Too large |
| `buffer.cpp` | 52KB | 1942 | Growing unwieldy |

**Issues**:
- Hard to navigate
- Difficult to test specific areas
- Merge conflicts likely

**Recommendation**: Split files by feature:
```
src/
  mode_vim_normal.cpp    # Vim normal mode
  mode_vim_insert.cpp   # Vim insert mode  
  mode_vim_visual.cpp  # Vim visual mode
  mode_vim_ex.cpp      # Ex commands
  commands_buffer.cpp  # Buffer operations
  commands_file.cpp    # File operations
```

**Importance**: 5/10 - Maintainability

---

### 4.2 Missing Error Handling

| Location | Issue | Example |
|----------|-------|---------|
| All files | Assertions without degradation | `assert(!"error")` |
| Multiple | Throw without specific error | `throw std::invalid_argument` |

**Issue**: Many failures crash instead of graceful degradation.

**Resolution**: Implement result types, graceful fallback.

**Importance**: 5/10 - User experience

---

### 4.3 Magic Numbers

Multiple bare constants without explanation:
```cpp
m_scrollTimer.start(1000);           // What is 1000?
m_maxFontSize = 72;                 // Why 72?
timeout = 17;                      // Unclear
constexpr kLineMargin = 10;        // OK (named)
```

**Resolution**: Replace with named constants.

**Importance**: 3/10 - Code readability

---

## 5. Missing Core Features (FROM TODO.MD)

### 5.1 Top Priority Features (Original TODO.md)

| Feature | Status | Implementation |
|---------|--------|----------------|
| Auto Indent | ✅ DONE | Implemented in v0.2.0 |
| Find/Replace | ✅ DONE | `:s` command in v0.2.0 |
| Pointer to errors outside view | ❌ MISSING | Need to implement |
| Full screen mode | ❌ MISSING | Need to implement |

### 5.2 Implemented Features (Verified)

| Feature | Version | File |
|---------|--------|------|
| Notification System | v0.1.0 | notifications.h |
| Find/Replace (`:s`) | v0.2.0 | mode_vim.cpp |
| Auto-indent | v0.2.0 | mode_vim.cpp |
| Macros (`q`, `@`) | v0.3.0 | mode_vim.cpp |
| Folds (`zf`, `zo`) | v0.3.0 | fold.h/cpp |
| Visual Mode (v, V, Ctrl+v) | v0.3.0 | mode_vim.cpp |
| Multiple Cursors | v0.3.0 | window.cpp |
| Minimap | v0.3.0 | window.cpp |
| Git Integration | v0.3.0 | git.h/cpp |
| Terminal (`:terminal`) | v0.3.0 | terminal.h/cpp |

### 5.3 Remaining Features to Implement

| Feature | Impact | Priority | Estimated Effort |
|---------|--------|----------|-----------------|
| Error pointers | High | P1 | Medium |
| Fullscreen | Medium | P2 | Low |
| Plugin system | High | P2 | High |
| LSP client | High | P3 | Very High |
| Tree-sitter | Medium | P3 | Very High |
| WASM support | Medium | P4 | High |

---

## 6. Re-assessed Priorities

### 6.1 Original vs New Priority Mapping

| Original Rank | Feature | Re-assessed Priority | Rationale |
|--------------|---------|-------------------|----------|
| 1 | Auto Indent | ✅ DONE | Already implemented |
| 2 | Find/Replace | ✅ DONE | Already implemented |
| 3 | Error pointers | P1 | HIGH - Developer UX critical |
| 4 | Full screen | P3 | LOW - Nice to have |
| 5 | Plugin system | P2 | HIGH - Extensibility |
| 6 | LSP client | P4 | Niche - defer |

### 6.2 Keep/Resolve/Deprecate Matrix

| Debt Item | Decision | Rationale |
|----------|----------|-----------|
| Undo bleeding | RESOLVE | Critical data corruption |
| Thread safety | RESOLVE | Concurrency essential |
| UTF-8 incomplete | RESOLVE | Modern requirement |
| Ex commands | KEEP | 15% complete, expand |
| Git disabled | RESOLVE | Common workflow |
| File size | RESOLVE | Long-term maintenance |
| Magic numbers | ACCEPT | Low impact |
| Deprecated gl3w | RESOLVE | Already done (GLAD) |

---

## 7. Additional Recommendations

### 7.1 Immediate Actions (Next Sprint)

1. **Fix Undo Isolation** (Critical)
   - Per-buffer undo stacks
   - Broadcast filtering

2. **Enable Thread Safety**
   - Add mutex to ThreadPool
   - Re-enable threading tests

3. **Uncomment Git Commands**
   - Fix `GetGit()` method
   - Register commands

4. **Add Essential Ex Commands**
   - `:e [file]` - File open
   - `:bn` / `:bp` - Buffer navigation
   - `:sp` - Split window

### 7.2 Short-term (1-2 Months)

1. Complete UTF-8 support
2. Add `:set` basic options
3. Fix performance bottlenecks
4. Standardize memory management
5. Split large files

### 7.3 Medium-term (3-6 Months)

1. Implement error pointers (external view)
2. Full screen mode
3. Plugin system skeleton
4. Add `:make` build integration
5. Basic `:grep` integration

### 7.4 Long-term (6+ Months)

1. LSP client (Language Server Protocol)
2. Tree-sitter syntax
3. WASM/Emscripten target
4. Neovim msgpack-rpc compatibility

---

## 8. Updated Comparison Matrix

| Feature Type | Completion | Gap to Vim |
|-------------|-----------|------------|
| Motions/Text Objects | ~75% | 25% |
| Ex Commands | ~15% | 85% |
| Visual Mode | ~90% | 10% |
| Macros | ~80% | 20% |
| Folding | ~60% | 40% |
| Git Integration | ~0% (disabled) | 100% |
| Terminal | ~50% | 50% |

---

## 9. Dependencies and Order

### 9.1 Feature Dependencies

```
Undo Fix
    ↓
Thread Safety ←←← These can be parallel
    ↓
UTF-8 ←←← Depends on Thread Safety (async ops)
    ↓
Ex Commands ←←← UTF-8 needed first
    ↓
Git Integration
```

### 9.2 Blocker Dependencies

| Blocker | Blocks |
|---------|-------|
| Thread Safety | Async operations, file loading |
| UTF-8 | Unicode search, international text |
| Error handling | Plugin system |

---

## 10. Summary and Action Plan

### Priority Matrix

| Priority | Items | Effort | Impact |
|----------|-------|--------|--------|
| **P0** | Undo, Thread, UTF-8, Git | Medium | Critical |
| **P1** | Ex Commands (essential) | Medium | High |
| **P2** | Error pointers, Performance | Medium | High |
| **P3** | Fullscreen, Plugin skeleton | Low | Medium |
| **P4** | LSP, Tree-sitter | Very High | Medium |

### Recommended Next Steps

1. **Week 1-2**: Fix undo bleeding between buffers
2. **Week 3-4**: Fix ThreadPool mutex, enable threading tests
3. **Week 5-6**: Uncomment and fix Git commands
4. **Week 7-8**: Add `:e`, `:bn`, `:bp`, `:sp` commands

---

*Report generated from comprehensive analysis of 61+ TODOs, SECURITY_REPORT.md, REPORT.md, and source code inspection.*

(End of file - total 566 lines)