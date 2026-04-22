# Lua Scripting in pZep

pZep supports optional Lua scripting integration, allowing you to extend the editor with custom automation, macros, and dynamic behavior. Lua is a lightweight, embeddable scripting language designed for extensibility and widely used in game engines and embedded systems.

## Architecture

Lua integration in pZep follows a **sandboxed provider model**:

```
┌─────────────────────────────────────────────┐
│  pZep Editor (Core C++ Engine)              │
├─────────────────────────────────────────────┤
│  IZepReplProvider Interface                 │
├─────────────────────────────────────────────┤
│  LuaReplProvider (Sandbox)                  │
│  ├── lua_State* (isolated runtime)          │
│  ├── Restricted global environment          │
│  ├── Editor-bridge API exposure            │
│  └── Resource limits (time/memory)         │
└─────────────────────────────────────────────┘
```

**Key Components:**
- **`IZepReplProvider`** – Abstract interface all providers implement
- **`LuaReplProvider`** – Lua-specific implementation (`src/mode_lua_repl.cpp`)
- **Editor Bridge API** – Safe, restricted functions exposed to Lua scripts
- **Sandbox** – Isolated Lua state with no direct OS/filesystem access

## Quick Start

### Enable Lua Support

Lua support is included by default. No additional configuration needed.

### Basic Lua Commands

Open the command region in pZep (typically at the bottom) and type:

```lua
-- Simple arithmetic
> 2 + 2 * 10
24

-- String concatenation
> "Hello, " .. "pZep!"
Hello, pZep!

-- Table creation and access
> my_table = {a=1, b=2, c=3}
> my_table.a
1

-- Iterate over table
> for k,v in pairs(my_table) do print(k,v) end
a	1
b	2
c	3
```

### Lua as a Calculator

Lua can be used as an inline calculator within pZep:

```lua
-- Unit conversion: Celsius to Fahrenheit
> function c_to_f(c) return (c * 9/5) + 32 end
> c_to_f(100)
212

-- Statistics: mean of numbers
> function mean(...)
>>   local sum = 0
>>   local count = select('#', ...)
>>   for i = 1, count do sum = sum + select(i, ...) end
>>   return sum / count
>> end
> mean(10, 20, 30, 40, 50)
30

-- Binary/hex conversion helpers
> function to_bin(n)
>>   local result = ""
>>   while n > 0 do
>>     result = (n % 2) .. result
>>     n = math.floor(n / 2)
>>   end
>>   return result
>> end
> to_bin(42)
101010
```

## Editor Automation

Lua scripts can interact with the pZep editor through the **Editor Bridge API**. This provides powerful automation capabilities while maintaining security boundaries.

### Working with Buffers

```lua
-- Get the active buffer (current file)
local buf = editor:GetActiveBuffer()

-- Get buffer information
print("Buffer name:", buf:GetName())
print("Buffer length:", buf:GetLength())
print("Is modified:", buf:IsModified())

-- Get current cursor position
local cursor = buf:GetCursor()
print("Line:", cursor.line, "Column:", cursor.column)

-- Read text from buffer
local line_text = buf:GetLineText(cursor.line)
print("Current line:", line_text)

-- Insert text at cursor
buf:Insert(cursor.line, cursor.column, "-- Added by Lua script\n")

-- Save the buffer
buf:Save()
print("Buffer saved")
```

### Practical Automation Examples

#### Example 1: Comment/Uncomment Toggle

```lua
-- Toggle comment on current line (C-style comments)
function toggle_comment()
    local buf = editor:GetActiveBuffer()
    local cursor = buf:GetCursor()
    local line = buf:GetLineText(cursor.line)

    -- Check if line starts with -- (Lua-style comment)
    if line:match("^%s*%-%-") then
        -- Uncomment: remove leading --
        local uncommented = line:gsub("^%s*%-%-%s?", "")
        buf:ReplaceLine(cursor.line, uncommented)
        print("Uncommented line")
    else
        -- Comment: add -- at start
        local commented = "-- " .. line
        buf:ReplaceLine(cursor.line, commented)
        print("Commented line")
    end
end

-- Usage: :lua toggle_comment()
-- Or bind to a key in your keymap
```

#### Example 2: Insert Timestamp

```lua
-- Insert ISO 8601 timestamp at cursor
function insert_timestamp()
    local buf = editor:GetActiveBuffer()
    local cursor = buf:GetCursor()

    -- Get current time
    local now = os.date("!%Y-%m-%dT%H:%M:%SZ")  -- UTC time

    -- Insert formatted timestamp
    local text = "-- " .. now .. " - "
    buf:Insert(cursor.line, cursor.column, text)
end

-- Usage: Call from command or keybinding
-- Result: -- 2026-04-23T05:44:20Z -
```

#### Example 3: Word Count Statistics

```lua
-- Count words, lines, and characters in current buffer
function buffer_stats()
    local buf = editor:GetActiveBuffer()
    local num_lines = buf:GetNumLines()

    local total_chars = 0
    local total_words = 0

    for i = 0, num_lines - 1 do
        local line = buf:GetLineText(i)
        total_chars = total_chars + #line

        -- Count words (non-whitespace sequences)
        local words_in_line = 0
        for _ in line:gmatch("%S+") do
            words_in_line = words_in_line + 1
        end
        total_words = total_words + words_in_line
    end

    print("Buffer Statistics:")
    print(string.format("  Lines: %d", num_lines))
    print(string.format("  Words: %d", total_words))
    print(string.format("  Characters: %d", total_chars))
    print(string.format("  Avg words/line: %.1f", total_words / num_lines))
end

-- Call with: :lua buffer_stats()
```

#### Example 4: Duplicate Line with Modification

```lua
-- Duplicate current line and indent it
function duplicate_line_indented()
    local buf = editor:GetActiveBuffer()
    local cursor = buf:GetCursor()
    local current_line = buf:GetLineText(cursor.line)

    -- Insert duplicate below with extra indent
    local indent = current_line:match("^(%s*)")
    local new_line = indent .. "    " .. current_line:gsub("^%s*", "")

    buf:InsertLine(cursor.line + 1, new_line)
    print("Duplicated line with indent")
end
```

## Configuration & Keymaps

### Bind Lua Functions to Keys

Add to your `zep.cfg` or use the `:map` command:

```vim
" Map F5 to run buffer stats
:map <F5> :lua buffer_stats()<CR>

" Map F6 to insert timestamp
:map <F6> :lua insert_timestamp()<CR>

" Map Ctrl+Alt+C to toggle comment
:map <C-A-C> :lua toggle_comment()<CR>
```

### Persistent Configuration

Store reusable Lua functions in a startup script:

```lua
-- ~/.config/pzep/lua/init.lua  (or zep.cfg [lua] section)

-- Utility: trim whitespace
function trim(s)
    return s:match("^%s*(.*%S)") or ""
end

-- Utility: file extension
function ext(filename)
    return filename:match(".*%.([^%.]+)$") or ""
end

-- Auto-format current line (strip trailing whitespace)
function trim_line()
    local buf = editor:GetActiveBuffer()
    local cursor = buf:GetCursor()
    local line = buf:GetLineText(cursor.line)
    buf:ReplaceLine(cursor.line, trim(line))
end

-- Initialize: register functions globally
_G.toggle_comment = toggle_comment
_G.insert_timestamp = insert_timestamp
_G.buffer_stats = buffer_stats
_G.trim_line = trim_line

print("Lua environment loaded")
```

### Load Custom Scripts

```lua
-- Load a Lua script file from disk (sandboxed, limited paths)
local script = editor:LoadScript("scripts/utils.lua")
if script then
    print("Custom script loaded")
end
```

## Security Model

Lua scripting in pZep is **sandboxed by default** with strict boundaries:

### What Lua CAN Do

- **Read/write editor buffers** – only through editor bridge API
- **Manipulate text** – insert, delete, replace within open files
- **Query editor state** – cursor position, buffer info, mode status
- **Call editor commands** – via `editor:ExecuteCommand()`
- **Perform calculations** – full Lua standard library (math, string, table)
- **Maintain script state** – global variables persist for session

### What Lua CANNOT Do

- **No filesystem access** – Cannot read/write arbitrary files
- **No network access** – Cannot make HTTP requests or open sockets
- **No OS command execution** – Cannot spawn processes or shell commands
- **No environment access** – Cannot read environment variables
- **No debug library** – Debug hooks disabled
- **No package loading** – `require()` disabled (except sandboxed modules)

### Protected Global Environment

```lua
-- These globals are NOT available (removed for security):
-- os.execute  → removed
-- io.open     → removed
-- os.getenv   → removed
-- loadfile    → removed
-- dofile      → removed
-- package     → restricted

-- What IS available:
math   = math      -- math functions (sin, cos, sqrt, etc.)
string = string    -- string manipulation
table  = table     -- table utilities
tonumber, tostring, type, pairs, ipairs, print  -- safe
```

### Resource Limits

Configure in `zep.cfg`:

```ini
[lua]
; Maximum execution time per script (milliseconds)
max_execution_time_ms = 1000

; Memory limit in kilobytes
memory_limit_kb = 10240

; Enable/disable Lua scripting at runtime
enabled = true
```

### Safe Error Handling

```lua
-- Wrapper for safe script execution
function safe_execute(func, ...)
    local success, result = xpcall(func, function(err)
        -- Capture error with traceback
        return debug.traceback(err, 2)
    end, ...)

    if success then
        return true, result
    else
        print("ERROR:", result)
        return false, nil
    end
end

-- Usage
local ok, val = safe_execute(function()
    -- Your script code here
    return buffer_stats()
end)

if not ok then
    print("Script failed – check logs")
end
```

## Advanced Editor Integration

### Working with Multiple Buffers

```lua
-- List all open buffers
function list_buffers()
    local buffers = editor:GetBuffers()
    print("Open buffers:")
    for i, buf in ipairs(buffers) do
        local status = buf:IsModified() and "[*]" or "   "
        print(string.format("  %d. %s %s", i, status, buf:GetName()))
    end
end

-- Switch to buffer by name or index
function switch_buffer(target)
    local buffers = editor:GetBuffers()

    -- Try by index
    if type(target) == "number" and buffers[target] then
        editor:SetActiveBuffer(buffers[target])
        print("Switched to buffer:", buffers[target]:GetName())
        return
    end

    -- Try by name (partial match)
    for _, buf in ipairs(buffers) do
        if buf:GetName():find(target, 1, true) then
            editor:SetActiveBuffer(buf)
            print("Switched to buffer:", buf:GetName())
            return
        end
    end

    print("Buffer not found:", target)
end

-- Usage:
-- :lua list_buffers()
-- :lua switch_buffer("main.cpp")
```

### Syntax-Aware Operations

```lua
-- Insert license header at top of file
function insert_license()
    local buf = editor:GetActiveBuffer()
    local license = [[
-- Copyright (c) 2026 Your Company
-- Licensed under MIT License
-- See LICENSE for details.

]]

    -- Prepend to beginning of file
    buf:Insert(0, 0, license)
    print("License header inserted")
end

-- Wrap selected text region (Visual mode)
-- Assumes a visual selection is active
function wrap_selection(prefix, suffix)
    local buf = editor:GetActiveBuffer()
    local sel = editor:GetSelection()

    if sel:IsEmpty() then
        print("No selection")
        return
    end

    local start_line, start_col = sel.start.line, sel.start.column
    local end_line, end_col = sel.end.line, sel.end.column

    -- Get selected text
    local selected = buf:GetTextRegion(start_line, start_col, end_line, end_col)

    -- Wrap and replace
    local wrapped = prefix .. selected .. suffix
    buf:ReplaceRegion(start_line, start_col, end_line, end_col, wrapped)
    print("Selection wrapped")
end

-- Usage from visual mode:
-- :lua wrap_selection("/* ", " */")
```

### Search and Replace

```lua
-- Find and replace across entire buffer
function find_replace(find_str, replace_str, case_sensitive)
    case_sensitive = case_sensitive or false

    local buf = editor:GetActiveBuffer()
    local num_lines = buf:GetNumLines()
    local count = 0

    for i = 0, num_lines - 1 do
        local line = buf:GetLineText(i)
        local new_line

        if case_sensitive then
            new_line = line:gsub(find_str, replace_str)
        else
            new_line = line:gsub(find_str:lower(), replace_str:lower())
        end

        if new_line ~= line then
            buf:ReplaceLine(i, new_line)
            count = count + 1
        end
    end

    print(string.format("Replaced %d occurrences", count))
end

-- Usage:
-- :lua find_replace("TODO", "FIXME", true)
```

### Macro Recording Playback

```lua
-- Simple macro storage
local macros = {}

-- Record current actions into a macro
function start_macro(name)
    if _G.current_macro then
        print("Already recording macro:", _G.current_macro)
        return
    end

    _G.current_macro = name
    _G.macro_actions = {}
    print("Started recording macro:", name)
end

-- Record an action (called from keypresses)
function record_action(action)
    if _G.macro_actions then
        table.insert(_G.macro_actions, action)
    end
end

-- Stop recording and save macro
function stop_macro()
    if not _G.current_macro then
        print("No macro recording")
        return
    end

    macros[_G.current_macro] = _G.macro_actions
    print("Saved macro '" .. _G.current_macro .. "' with " .. #_G.macro_actions .. " actions")
    _G.current_macro = nil
    _G.macro_actions = nil
end

-- Playback a recorded macro
function play_macro(name)
    local actions = macros[name]
    if not actions then
        print("Macro not found:", name)
        return
    end

    local buf = editor:GetActiveBuffer()
    for _, action in ipairs(actions) do
        -- Replay the action (simplified)
        if action.type == "insert" then
            buf:Insert(action.line, action.col, action.text)
        elseif action.type == "delete" then
            -- Delete logic
        end
    end

    print("Played macro:", name)
end
```

## Enterprise Use Cases

### 1. Code Review Automation

```lua
-- Check for common code review issues
function code_review_check()
    local buf = editor:GetActiveBuffer()
    local issues = {}

    for i = 0, buf:GetNumLines() - 1 do
        local line = buf:GetLineText(i)

        -- Check for debug prints
        if line:match("print%s*%(") and not line:match("--") then
            table.insert(issues, string.format("Line %d: Possible debug print", i + 1))
        end

        -- Check for TODO without ticket
        if line:match("TODO") and not line:match("TODO%[%w+%-%d+%]") then
            table.insert(issues, string.format("Line %d: TODO without ticket reference", i + 1))
        end

        -- Check for hardcoded credentials pattern
        if line:match("[Pp]assword%s*=%s*['\"][^'\"]+['\"]") then
            table.insert(issues, string.format("Line %d: Hardcoded password detected!", i + 1))
        end

        -- Check for long lines (> 120 chars)
        if #line > 120 then
            table.insert(issues, string.format("Line %d: Exceeds 120 characters (%d)", i + 1, #line))
        end
    end

    if #issues > 0 then
        print("Code Review Issues Found:")
        for _, issue in ipairs(issues) do
            print("  - " .. issue)
        end
    else
        print("No issues found – good job!")
    end
end

-- Integrate into pre-commit validation
```

### 2. Config File Generator

```lua
-- Generate .env file from template with environment substitution
function generate_env_file(template_path, output_path)
    local function interpolate(str, env)
        return str:gsub([[%$%{([^}]+)}]], function(key)
            return env[key] or ""
        end)
    end

    -- Mock environment (in real use, pass actual env vars)
    local mock_env = {
        API_URL = "https://api.example.com",
        DB_HOST = "localhost",
        DB_PORT = "5432",
        NODE_ENV = "production"
    }

    -- Read template (if editor can access)
    local content = editor:LoadExternalFile(template_path)
    if not content then
        print("Template not found:", template_path)
        return
    end

    local result = interpolate(content, mock_env)

    -- Write output (via editor buffer)
    local buf = editor:CreateBuffer("generated.env")
    buf:SetText(result)
    buf:SaveAs(output_path)

    print("Generated:", output_path)
end
```

### 3. Log Analysis Helper

```lua
-- Parse and summarize log files
function analyze_log()
    local buf = editor:GetActiveBuffer()
    local num_lines = buf:GetNumLines()

    local stats = {
        errors = 0,
        warnings = 0,
        info = 0,
        http_requests = 0,
        unique_ips = {}
    }

    local patterns = {
        ERROR = "ERROR",
        WARNING = "WARN",
        INFO = "INFO",
        HTTP = 'HTTP/%d.%d"%s (%d+)',
        IP = '(%d+%.%d+%.%d+%.%d+)'
    }

    for i = 0, num_lines - 1 do
        local line = buf:GetLineText(i)

        if line:find("ERROR") then stats.errors = stats.errors + 1 end
        if line:find("WARN") then stats.warnings = stats.warnings + 1 end
        if line:find("INFO") then stats.info = stats.info + 1 end

        -- Extract IPs
        for ip in line:gmatch("(%d+%.%d+%.%d+%.%d+)") do
            stats.unique_ips[ip] = true
        end
    end

    print("Log Analysis Summary:")
    print(string.format("  Errors:   %d", stats.errors))
    print(string.format("  Warnings: %d", stats.warnings))
    print(string.format("  Info:     %d", stats.info))
    print(string.format("  Unique IPs: %d", #stats.unique_ips))
end
```

### 4. JSON/YAML Data Extraction

```lua
-- Simple JSON value extractor (without full JSON parser)
-- For simple key-value extraction from structured text
function extract_json_value(key)
    local buf = editor:GetActiveBuffer()
    local pattern = '"' .. key .. '"%s*:%s*"([^"]+)"'

    for i = 0, buf:GetNumLines() - 1 do
        local line = buf:GetLineText(i)
        local value = line:match(pattern)
        if value then
            return value
        end
    end

    return nil
end

-- Usage: Extract API endpoint from config
-- :lua api_url = extract_json_value("api_endpoint")
-- :lua print("API:", api_url)
```

## Performance Tips

### 1. Minimize Editor API Calls

**Naive approach** – many calls, slow:
```lua
-- SLOW: One call per line
for i = 0, buf:GetNumLines() - 1 do
    local len = buf:GetLineText(i):len()
    total = total + len
end
```

**Optimized approach** – batch operations:
```lua
-- FASTER: Cache line count and minimize calls
local num = buf:GetNumLines()
for i = 0, num - 1 do
    -- Single call per iteration (already optimal)
    local line = buf:GetLineText(i)
    total = total + #line  -- # operator is O(1) for strings
end
```

### 2. Use Local Variables

```lua
-- Good: cache editor reference
local editor = editor
local buf = editor:GetActiveBuffer()

-- Better: cache buffer methods
local get_line = buf.GetLineText
local num_lines = buf.GetNumLines

for i = 0, num_lines() - 1 do
    print(get_line(i))
end
```

### 3. Avoid Long-Running Scripts

Split heavy work into chunks:

```lua
-- DON'T: Block editor for 10 seconds
function heavy_processing()
    for i = 1, 1000000 do
        -- do work...
    end
end

-- DO: Process in chunks with editor updates
function chunked_processing()
    local CHUNK_SIZE = 1000
    for i = 1, 1000000, CHUNK_SIZE do
        -- Process chunk
        for j = i, math.min(i + CHUNK_SIZE - 1, 1000000) do
            -- work...
        end

        -- Yield to editor (optional)
        editor:UpdateDisplay()
    end
end
```

## Debugging Lua Scripts

### Print Debugging

```lua
function debug_buffer_state()
    local buf = editor:GetActiveBuffer()
    local cursor = buf:GetCursor()

    print("=== Debug Info ===")
    print("Buffer:", buf:GetName())
    print("Lines:", buf:GetNumLines())
    print("Cursor:", cursor.line, cursor.column)
    print("Modified:", buf:IsModified())
    print("==================")
end
```

### Error Traces

```lua
-- Wrap in pcall for safe errors
local ok, err = pcall(function()
    -- Your code that might fail
    local buf = editor:GetActiveBuffer()
    local line = buf:GetLineText(99999)  -- Out of bounds!
end)

if not ok then
    print("Script error:", err)
end
```

## Next Steps

- **Explore the Editor API** – Check `include/zep/editor.h` for all available methods
- **Read security model** – See `docs/SECURITY_REPORT.md` for sandbox details
- **Try REPL mode** – Use `:repl` to enter interactive Lua mode
- **Share scripts** – Export `.lua` files and load via configuration

## API Reference Summary

| Category | Available Functions | Notes |
|----------|-------------------|-------|
| Buffer   | `GetName()`, `GetLength()`, `GetNumLines()`, `GetLineText()`, `Insert()`, `ReplaceLine()`, `Save()` | Full buffer manipulation |
| Editor   | `GetActiveBuffer()`, `GetBuffers()`, `SetActiveBuffer()`, `ExecuteCommand()`, `UpdateDisplay()` | Editor control |
| Selection | `GetSelection()`, `SetSelection()` | Visual selection handling |
| String   | All `string` library functions (sub, gsub, match, format, etc.) | Complete Lua string lib |
| Math     | All `math` library functions (sin, cos, sqrt, random, etc.) | Complete Lua math lib |
| Table    | All `table` library functions (insert, remove, sort, etc.) | Complete Lua table lib |
| IO       | `print()` only | No file I/O |

---

**See also:**
- `README_REPL.md` – General REPL architecture and comparison
- `docs/SECURITY_REPORT.md` – Security model and sandbox details
- `REPL_IMPLEMENTATION_SUMMARY.md` – Implementation details
