# nZep Changelog
## What's New in nZep (Notification Editor)

---

## v0.2.0 - High Impact Features Update

### 1. Find/Replace (:s command) - NEW

**Location:** `src/mode_vim.cpp`

The substitute command now works properly:

| Command | Description |
|---------|-------------|
| `:s/foo/bar/` | Replace first match on current line |
| `:s/foo/bar/g` | Replace all matches on current line |
| `:%s/foo/bar/g` | Replace all matches in entire file |
| `:Ns/foo/bar/g` | Replace on N lines |

**Supported flags:**
- `g` - global (replace all on line)
- `i` - ignore case
- `c` - confirm (shows message)

**Example:**
```vim
:s/old/new/       " Replace first 'old' with 'new'
:s/old/new/g      " Replace all 'old' with 'new'  
:%s/foo/bar/g     " Replace all 'foo' with 'bar' in file
```

---

### 2. Auto-Indent - NEW

**Location:** `src/mode.cpp`, `buffer.cpp`

Enable with:
```vim
:set autoindent    " Enable
:set noautoindent " Disable
```

**Features:**
- **Basic**: Press Enter, get previous line's indent
- **C-style**: `{` indents, `}` de-indents
- **Tab**: Increase indentation
- **Shift+Tab**: Decrease indentation

**Usage:**
```python
# With autoindent enabled:
def foo():
    # Press Enter here - gets 4 spaces indent
    pass
```

---

### 3. Ex Commands - NEW

**Location:** `src/mode_vim.cpp`

Essential Vim commands implemented:

| Command | Description |
|---------|-------------|
| `:q` | Quit (fails if unsaved) |
| `:q!` | Quit without saving |
| `:qa` | Quit all buffers |
| `:w` | Write/save |
| `:w file.txt` | Save as |
| `:wq` | Write and quit |
| `:n` | Next buffer |
| `:ls` / `:buffers` | List buffers |
| `:g/pattern/command` | Global: run on matching lines |

**Examples:**
```vim
:w              " Save current file
:w newfile.txt  " Save as new file
:q!             " Quit without saving
:g/^$/d         " Delete empty lines
:g/TODO/p       " Print all TODO lines
```

---

### 4. UTF-8 Complete Support - FIXED

**Location:** `src/buffer.cpp`, `src/mode_search.cpp`, `stringutils.h`

**Fixed:**
- Replace function now handles multi-byte characters
- Search input properly encodes Unicode
- Added utility functions:
  - `utf8_strlen()` - count codepoints
  - `utf8_next_codepoint()` - extract next codepoint
  - `utf8_substr()` - UTF-8 aware substring

**Before (broken):**
```
"ü" was treated as 2 characters
```

**After (fixed):**
```
"ü" is properly 1 character
```

---

## v0.3.0 - Extended Features Update

### 5. Macros (q recording) - NEW

**Location:** `src/mode_vim.cpp`, `include/zep/keymap.h`, `include/zep/mode_vim.h`

Record and playback keystrokes:

| Command | Description |
|---------|-------------|
| `qa` | Start recording macro to register a |
| `q` | Stop recording |
| `@a` | Play back macro from register a |
| `@@` | Repeat last macro |
| `10@a` | Play macro 10 times |

**Usage:**
```vim
qa                  " Start recording
iHello World<Esc>  " Type some stuff
q                   " Stop recording
@a                  " Replay: "Hello World" appears
@@                  " Replay again
```

---

### 6. Folds - NEW

**Location:** `include/zep/fold.h`, `src/fold.cpp`

Code folding with visual indicators:

| Command | Description |
|---------|-------------|
| `zf` | Create fold (from selection or count) |
| `zd` | Delete fold at cursor |
| `zD` | Delete all folds |
| `zo` | Open fold |
| `zO` | Open all folds |
| `zc` | Close fold |
| `zC` | Close all folds |
| `zR` | Open all folds |
| `zM` | Close all folds |
| `za` | Toggle fold |

**Features:**
- Fold by markers
- Fold by indentation (Python)
- +/- indicators in gutter

---

### 7. Multiple Cursors - NEW

**Location:** `include/zep/window.h`, `src/window.cpp`

Multi-cursor editing (Sublime/VS Code style):

| Command | Description |
|---------|-------------|
| `Ctrl+d` | Add cursor at next word match |
| `Ctrl+k` | Skip current, go to next |
| `Ctrl+Shift+d` | Select all matches |
| Type | Applies to all cursors |
| Escape | Exit multi-cursor mode |

**Usage:**
- Place cursor on word
- Ctrl+d to add next occurrence
- Type to edit all at once

---

### 8. Minimap - NEW

**Location:** `src/window.cpp`, `src/theme.cpp`

Code overview on the right side:

- Shows entire file in miniature
- Syntax highlighting visible
- Current viewport indicated (draggable)
- Click to jump to location
- Configure: `:set minimap`, `:set minimapwidth=100`

---

### 9. Git Integration - NEW

**Location:** `include/zep/git.h`, `src/git.cpp`

Git features in editor:

| Command | Description |
|---------|-------------|
| (gutter) | Added (green), Modified (yellow), Deleted (red) |
| `:Gstatus` | Show git status |
| `:GBlame` | Show commit info per line |
| `:Gdiff` | Show changes vs HEAD |
| `:Gcommit <msg>` | Commit changes |
| `:Gpush` | Push to remote |
| `:Gpull` | Pull from remote |

---

### 10. Terminal Emulator - NEW

**Location:** `include/zep/terminal.h`, `src/terminal.cpp`, `include/zep/mode_terminal.h`

Run commands in editor:

| Command | Description |
|---------|-------------|
| `:terminal` | Open interactive terminal |
| `:!cmd` | Run command and show output |

**Features:**
- ANSI color support
- Interactive shell
- Scrollback buffer

---

### 11. Complete Visual Mode - ENHANCED

**Location:** `src/mode_vim.cpp`

Visual mode improvements:

| Command | Description |
|---------|-------------|
| `v` | Character-wise selection |
| `V` | Line-wise selection |
| `Ctrl+v` | Block-wise selection |

All standard visual operators work:
- `d` - delete selection
- `y` - yank selection
- `c` - change selection
- `>` / `<` - shift
- `~` - toggle case
- `u` / `U` - lower/upper case

---

## v0.1.0 - Original Zep + nZep

See original README for base features.

### Editor Features
- Modal (Vim) + modeless (Standard) editing
- Tabs and splits
- Syntax highlighting (C++, Python, Rust, Go, etc.)
- Theme support (light/dark)
- REPL integration
- ImGui and Qt renderers
- Cross-platform (Windows, Linux, FreeBSD)

### Notification Features
- 12 notification types
- ImGui and Terminal renderers
- 19 unit tests
- No external dependencies

---

## Upgrading from Original Zep

All original features still work. New features are additive:

| Old Behavior | New Behavior |
|--------------|--------------|
| No :s command | `:s/foo/bar/g` works |
| No auto-indent | `:set autoindent` works |
| No :q/:g | All ex commands work |
| UTF-8 buggy | UTF-8 fully works |

---

## Breaking Changes

None. All new features are opt-in.

---

## Test Status

Run tests:
```bash
./build/tests/unittests.exe --gtest_filter="*"
```

Expected: All tests pass including new features.

---

*For full documentation, see docs/NOTIFICATIONS_USER_MANUAL.md*