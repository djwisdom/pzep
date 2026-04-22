# Duktape Scripting in pZep

pZep supports optional Duktape scripting integration, providing an ultra-lightweight JavaScript engine for editor automation and extensibility. Duktape is an embeddable JavaScript engine with a focus on portability and minimal footprint (~200-400KB), designed to be compiled with minimal dependencies.

## Architecture

Duktape integration follows the same **sandboxed provider model** as other scripting backends:

```
┌─────────────────────────────────────────────┐
│  pZep Editor (Core C++ Engine)              │
├─────────────────────────────────────────────┤
│  IZepReplProvider Interface                 │
├─────────────────────────────────────────────┤
│  DuktapeReplProvider (Sandbox)              │
│  ├── Duktape heap (isolated context)        │
│  ├── Property-based access control          │
│  ├── Custom C function bindings             │
│  └── No built-in Node.js-style APIs         │
└─────────────────────────────────────────────┘
```

**Key Characteristics:**
- **ECMAScript 2020+** subset support
- **No external dependencies** – single-header implementation
- **Very small binary footprint** – ideal for embedded deployment
- **Custom C API bindings** – Editor bridge exposed as native functions
- **Sandboxed execution** – No filesystem, network, or OS access

## Quick Start

### Enable Duktape Support

Duktape support is compiled in by default. No configuration needed.

### Basic JavaScript Expressions

Open the command region and type JavaScript:

```javascript
// Simple arithmetic
> 2 + 2 * 10
24

// String concatenation
> "Hello, " + "pZep!"
Hello, pZep!

// Arrays and objects
> let colors = ["red", "green", "blue"]
> colors[1]
green

> let config = {theme: "dark", fontsize: 14}
> config.theme
dark

// Template literals
> let name = "pZep"
> `Welcome to ${name}!`
Welcome to pZep!
```

### JavaScript as a Calculator

```javascript
// Unit conversion: Fahrenheit to Celsius
function fToC(f) {
    return (f - 32) * 5/9;
}
fToC(212)
100

// Fibonacci (memoized)
const fib = (function() {
    const memo = {};
    return function(n) {
        if (n <= 1) return n;
        if (memo[n]) return memo[n];
        memo[n] = fib(n-1) + fib(n-2);
        return memo[n];
    };
})();
fib(10)
55

// Prime number checker
function isPrime(n) {
    if (n < 2) return false;
    for (let i = 2; i * i <= n; i++) {
        if (n % i === 0) return false;
    }
    return true;
}
isPrime(42)
false
isPrime(17)
true
```

## Editor Automation

Duktape scripts access the editor through the **Editor Bridge API** – a set of carefully curated functions that allow text manipulation without compromising security.

### Buffer Operations

```javascript
// Get the active buffer (current file)
const buf = editor.GetActiveBuffer();

// Buffer metadata
print("Buffer name:", buf.GetName());
print("Buffer length:", buf.GetLength());
print("Modified:", buf.IsModified());

// Cursor position
const cursor = buf.GetCursor();
print("Line:", cursor.line, "Column:", cursor.column);

// Read current line
const lineText = buf.GetLineText(cursor.line);
print("Current line:", lineText);

// Insert text at cursor
buf.Insert(cursor.line, cursor.column, "// inserted via Duktape\n");

// Save buffer
buf.Save();
print("Buffer saved");
```

### Practical Automation Examples

#### Example 1: Comment/Uncomment Toggle

```javascript
// Toggle C-style comments (/* */) on selected region or current line
function toggleComment() {
    const buf = editor.GetActiveBuffer();
    const cursor = buf.GetCursor();
    const line = buf.GetLineText(cursor.line);

    // Check if line is commented
    if (line.trim().startsWith("/*") && line.trim().endsWith("*/")) {
        // Uncomment: remove wrappers
        const uncommented = line.replace(/^(\s*)\/\*/, "$1").replace(/\*\/\s*$/, "");
        buf.ReplaceLine(cursor.line, uncommented);
        print("Uncommented");
    } else if (line.trim().startsWith("//")) {
        // Uncomment single-line slash
        const uncommented = line.replace(/^(\s*)\/\/\s?/, "$1");
        buf.ReplaceLine(cursor.line, uncommented);
        print("Uncommented");
    } else {
        // Comment: add /* */
        const commented = "/* " + line + " */";
        buf.ReplaceLine(cursor.line, commented);
        print("Commented");
    }
}

// Usage: :duk toggleComment()
// Or bind to key: :map <F5> :duk toggleComment()<CR>
```

#### Example 2: Insert License Header

```javascript
// Insert MIT license header at top of file
function insertLicense() {
    const buf = editor.GetActiveBuffer();

    const license = `/*
 * MIT License
 *
 * Copyright (c) ${new Date().getFullYear()} Your Company
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 */\n`;

    buf.Insert(0, 0, license);
    print("License header inserted");
}
```

#### Example 3: Word Count Statistics

```javascript
// Comprehensive buffer statistics
function bufferStats() {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();

    let totalChars = 0;
    let totalWords = 0;
    let maxLineLength = 0;

    for (let i = 0; i < numLines; i++) {
        const line = buf.GetLineText(i);
        totalChars += line.length;
        maxLineLength = Math.max(maxLineLength, line.length);

        // Count words (sequences of non-whitespace)
        const words = line.match(/\S+/g);
        if (words) totalWords += words.length;
    }

    print("=== Buffer Statistics ===");
    print("Lines:      " + numLines);
    print("Words:      " + totalWords);
    print("Characters: " + totalChars);
    print("Avg words/line: " + (totalWords / numLines).toFixed(1));
    print("Max line length: " + maxLineLength);
    print("========================");
}

// :duk bufferStats()
```

#### Example 4: Duplicate Line with Pattern

```javascript
// Duplicate current line with prefix/suffix transformation
function duplicateLine(prefix, suffix) {
    prefix = prefix || "";
    suffix = suffix || "";

    const buf = editor.GetActiveBuffer();
    const cursor = buf.GetCursor();
    const line = buf.GetLineText(cursor.line);

    // Transform and insert below
    const transformed = prefix + line + suffix;
    buf.InsertLine(cursor.line + 1, transformed);
    print("Duplicated line with transformation");
}

// Usage: :duk duplicateLine("// ", " // TODO")
// Result: adds comment wrapper
```

## Configuration & Keymaps

### Bind JavaScript Functions to Keys

```vim
" Map F5 to run statistics
:map <F5> :duk bufferStats()<CR>

" Map F6 to insert timestamp
:map <F6> :duk insertTimestamp()<CR>

" Map Ctrl+Alt+C for comment toggle
:map <C-A-C> :duk toggleComment()<CR>
```

### Startup Script with Multiple Functions

```javascript
// Place in ~/.config/pzep/duk/init.js or zep.cfg [duk] section

// Utility: trim whitespace
function trim(str) {
    return str.replace(/^\s+|\s+$/g, "");
}

// Utility: camelCase to snake_case
function camelToSnake(name) {
    return name.replace(/([A-Z])/g, "_$1").toLowerCase().substring(1);
}

// Insert timestamp in ISO format
function insertTimestamp() {
    const buf = editor.GetActiveBuffer();
    const cursor = buf.GetCursor();
    const iso = new Date().toISOString();
    buf.Insert(cursor.line, cursor.column, "// " + iso + " - ");
}

// Search and replace (case-insensitive)
function replaceAll(find, replace) {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();
    let count = 0;

    const re = new RegExp(find, "gi");

    for (let i = 0; i < numLines; i++) {
        const line = buf.GetLineText(i);
        const newLine = line.replace(re, replace);
        if (newLine !== line) {
            buf.ReplaceLine(i, newLine);
            count++;
        }
    }

    print("Replaced " + count + " occurrences");
}

// Export to global scope for REPL access
this.toggleComment = toggleComment;
this.insertTimestamp = insertTimestamp;
this.bufferStats = bufferStats;
this.replaceAll = replaceAll;

print("Duktape environment loaded");
```

## Security Model

Duktape runs in an **isolated heap context** with property-based access control. Only explicitly exposed editor APIs are available.

### What JavaScript CAN Do

- **Buffer manipulation** – Read/write via editor bridge
- **String operations** – Full ECMAScript string API
- **Array/object manipulation** – Complete data structure support
- **Math operations** – Full Math object
- **Date/time** – Date object (no system time unless exposed)
- **RegEx operations** – Full regular expression engine
- **Script state persistence** – Global variables across commands

### What JavaScript CANNOT Do

- **No `require()` or `import`** – No module loading
- **No `eval()` with global scope** – Eval restricted
- **No `Function` constructor** – Dynamic code generation blocked
- **No `Object` prototype manipulation** – Prototypes frozen
- **No `console` object** – Only `print()` available
- **No `process` or `global`** – Node.js globals absent
- **No `Reflect` or `Proxy`** – Meta-programming limited
- **No file I/O** – All `fs` operations missing
- **No network** – No `http`, `net`, `WebSocket` modules
- **No environment variables** – `process.env` unavailable

### Protected Global Object

```javascript
// These globals are UNSAFE and thus NOT available:
// Function      → removed (no dynamic code generation)
// eval          → restricted
// setTimeout     → removed
// setInterval    → removed
// require        → removed
// module         → removed
// process        → removed
// console        → replaced with safe print()

// These ARE available (safe):
Math       // Math library
String     // String constructor
Array      // Array methods
Object     // Basic object creation (frozen prototype)
JSON       // JSON parse/stringify (safe)
Date       // Date manipulation (no OS time access)
Number     // Number utilities
Boolean    // Boolean type
RegExp     // Regular expressions
Infinity   // Infinity constant
NaN        // Not-a-number
undefined  // Undefined value
null       // Null value
print      // Safe print to console
```

### Resource Limits

Configure in `zep.cfg`:

```ini
[duktape]
; Maximum execution time in milliseconds
max_execution_time_ms = 500

; Memory limit in kilobytes (Duktape heap size)
memory_limit_kb = 8192

; Enable Duktape scripting
enabled = true

; Stack size for native calls (bytes)
native_stack_size = 8192
```

### Error Handling Pattern

```javascript
// Safe execution wrapper
function safeRun(fn) {
    try {
        fn();
        return true;
    } catch (e) {
        print("ERROR: " + e);
        if (e.stack) print(e.stack);  // Duktape includes stack traces
        return false;
    }
}

// Usage
safeRun(function() {
    // Your code here
    throw new Error("Something went wrong");
});
```

## Advanced Editor Integration

### Working with Multiple Buffers

```javascript
// List all open buffers
function listBuffers() {
    const buffers = editor.GetBuffers();
    print("Open buffers:");
    for (let i = 0; i < buffers.length; i++) {
        const buf = buffers[i];
        const modified = buf.IsModified() ? "[*]" : "   ";
        print("  " + (i+1) + ". " + modified + " " + buf.GetName());
    }
}

// Switch to buffer by name (fuzzy match)
function switchBuffer(nameFragment) {
    const buffers = editor.GetBuffers();

    for (let i = 0; i < buffers.length; i++) {
        if (buffers[i].GetName().includes(nameFragment)) {
            editor.SetActiveBuffer(buffers[i]);
            print("Switched to:", buffers[i].GetName());
            return;
        }
    }

    print("Buffer not found:", nameFragment);
}

// Usage:
// :duk listBuffers()
// :duk switchBuffer("main.cpp")
```

### Syntax-Aware Operations

```javascript
// Insert header comment with filename
function insertHeader() {
    const buf = editor.GetActiveBuffer();
    const filename = buf.GetName();

    const header = `/* ${filename} */
 * Description: TODO
 * Author:      TODO
 * Created:     ${new Date().toISOString().split('T')[0]}
 */

`;

    buf.Insert(0, 0, header);
}

// Trim trailing whitespace from all lines
function trimTrailingWhitespace() {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();
    let trimmed = 0;

    for (let i = 0; i < numLines; i++) {
        const line = buf.GetLineText(i);
        const trimmedLine = line.replace(/\s+$/, "");
        if (line !== trimmedLine) {
            buf.ReplaceLine(i, trimmedLine);
            trimmed++;
        }
    }

    print("Trimmed " + trimmed + " lines");
}
```

### Search and Replace with Regex

```javascript
// Global regex-based find and replace
function regexReplace(pattern, replacement, caseSensitive) {
    caseSensitive = caseSensitive || false;

    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();
    const flags = caseSensitive ? "g" : "gi";
    const re = new RegExp(pattern, flags);
    let count = 0;

    for (let i = 0; i < numLines; i++) {
        const line = buf.GetLineText(i);
        const newLine = line.replace(re, replacement);
        if (line !== newLine) {
            buf.ReplaceLine(i, newLine);
            count++;
        }
    }

    print("Replaced " + count + " matches");
}

// Usage:
// :duk regexReplace("\\bTODO\\b", "FIXME", true)
// :duk regexReplace("  +", "\\t", false)  // Convert spaces to tabs
```

### Batch Processing

```javascript
// Process all open buffers
function processAllBuffers(processor) {
    const buffers = editor.GetBuffers();
    let total = 0;

    for (let i = 0; i < buffers.length; i++) {
        editor.SetActiveBuffer(buffers[i]);
        processor(buffers[i]);
        total++;
    }

    print("Processed " + total + " buffers");
}

// Example: normalize line endings across all files
function normalizeLineEndings() {
    processAllBuffers(function(buf) {
        const numLines = buf.GetNumLines();
        for (let i = 0; i < numLines; i++) {
            let line = buf.GetLineText(i);
            // Convert CRLF to LF
            line = line.replace(/\r\n/g, "\n");
            // Convert CR to LF
            line = line.replace(/\r/g, "\n");
            buf.ReplaceLine(i, line);
        }
    });
}
```

## Enterprise Use Cases

### 1. Compliance Audit Scanner

```javascript
// Scan current file for compliance violations
function complianceCheck() {
    const buf = editor.GetActiveBuffer();
    const filename = buf.GetName().toLowerCase();
    const violations = [];

    for (let i = 0; i < buf.GetNumLines(); i++) {
        const line = buf.GetLineText(i);
        const lineNum = i + 1;

        // Check for hardcoded credentials
        if (line.match(/(password|pwd|secret|api_key|token)\s*=\s*["'][^"']+["']/i)) {
            violations.push({
                line: lineNum,
                type: "SECURITY",
                msg: "Hardcoded credential detected"
            });
        }

        // Check for TODO without ticket
        if (line.match(/TODO(?!\[[A-Z]+-\d+\])/i)) {
            violations.push({
                line: lineNum,
                type: "PROCESS",
                msg: "TODO without ticket reference"
            });
        }

        // Check for debug statements
        if (line.match(/console\.(log|debug|warn)/) && !line.trim().startsWith("//")) {
            violations.push({
                line: lineNum,
                type: "CODE_QUALITY",
                msg: "Debug statement in production code"
            });
        }

        // Check line length
        if (line.length > 120) {
            violations.push({
                line: lineNum,
                type: "STYLE",
                msg: "Line exceeds 120 characters (" + line.length + ")"
            });
        }

        // Check for forbidden functions
        if (line.match(/\beval\(/) || line.match(/\bexec\(/)) {
            violations.push({
                line: lineNum,
                type: "SECURITY",
                msg: "Use of eval() or exec() is prohibited"
            });
        }
    }

    // Report
    if (violations.length > 0) {
        print("=== Compliance Issues ===");
        for (let v of violations) {
            print("L" + v.line + " [" + v.type + "]: " + v.msg);
        }
        print("Total: " + violations.length + " issues");
    } else {
        print("No compliance issues found");
    }
}
```

### 2. API Contract Validator

```javascript
// Validate API response format in code
function validateApiContract() {
    const buf = editor.GetActiveBuffer();

    // Extract expected response structure from comments
    const responseSchema = {
        required: ["data", "success", "error"],
        types: {
            data: "object",
            success: "boolean",
            error: "string|null"
        }
    };

    const issues = [];

    for (let i = 0; i < buf.GetNumLines(); i++) {
        const line = buf.GetLineText(i);

        // Check for missing error handling
        if (line.includes(".then(") && !buf.GetLineText(i+1).trim().startsWith(".catch")) {
            issues.push("L" + (i+1) + ": Promise without .catch()");
        }

        // Check for unvalidated user input
        if (line.includes("req.body") && !line.includes("validate")) {
            issues.push("L" + (i+1) + ": Unvalidated request body");
        }
    }

    if (issues.length > 0) {
        print("API Contract Issues:");
        issues.forEach(function(msg) { print("  " + msg); });
    } else {
        print("API contract appears valid");
    }
}
```

### 3. Code Formatter (Lightweight)

```javascript
// Simple code formatter for specific patterns
function formatCode() {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();

    for (let i = 0; i < numLines; i++) {
        let line = buf.GetLineText(i);

        // Fix multiple spaces to single
        line = line.replace(/ {2,}/g, " ");

        // Ensure space after commas (in arrays/function args)
        line = line.replace(/,([^\s])/g, ", $1");

        // Fix spacing around operators
        line = line.replace(/([+\-*/=])\s*/g, " $1 ");
        line = line.replace(/\s+([+\-*/=])\s+/g, " $1 ");

        // Trim trailing whitespace
        line = line.replace(/\s+$/, "");

        buf.ReplaceLine(i, line);
    }

    print("Formatting complete");
}
```

### 4. Dependency Checker

```javascript
// Track import statements and suggest updates
function analyzeImports() {
    const buf = editor.GetActiveBuffer();
    const imports = {};

    for (let i = 0; i < buf.GetNumLines(); i++) {
        const line = buf.GetLineText(i);

        // ES6 imports
        let match = line.match(/import\s+.*?\s+from\s+['"]([^'"]+)['"]/);
        if (match) {
            const pkg = match[1].split('/')[0].split('@')[0];
            if (!imports[pkg]) imports[pkg] = [];
            imports[pkg].push(i + 1);
        }

        // CommonJS requires
        match = line.match(/require\(['"]([^'"]+)['"]\)/);
        if (match) {
            const pkg = match[1].split('/')[0];
            if (!imports[pkg]) imports[pkg] = [];
            imports[pkg].push(i + 1);
        }
    }

    print("Dependencies found:");
    for (let pkg in imports) {
        print("  " + pkg + ": lines " + imports[pkg].join(", "));
    }

    // Security suggestions
    const knownRisky = ["eval", "child_process", "fs", "net"];
    for (let risky of knownRisky) {
        if (imports[risky]) {
            print("WARNING: Potentially dangerous module: " + risky);
        }
    }
}
```

## Performance Considerations

### Minimize Bridge Calls

Each call to `editor` or `buf` methods crosses the C++/JS boundary. Batch operations:

```javascript
// ❌ SLOW: One call per line
for (let i = 0; i < buf.GetNumLines(); i++) {
    let line = buf.GetLineText(i);  // Bridge call
    // ... process
}

// ✅ FASTER: Cache where possible
const numLines = buf.GetNumLines();  // One call
const getLine = buf.GetLineText.bind(buf);

for (let i = 0; i < numLines; i++) {
    let line = getLine(i);  // Still bridge, but no GetNumLines call
}
```

### Use Local Variables

```javascript
// Cache frequently-used methods
const getName = buf.GetName.bind(buf);
const getLine = buf.GetLineText.bind(buf);
const replaceLine = buf.ReplaceLine.bind(buf);

// Then use cached functions
print("Processing: " + getName());
```

## Debugging JavaScript Scripts

### Try-Catch Wrapper

```javascript
// Safe execution with detailed error
function safeExecute(fn) {
    try {
        fn();
        return true;
    } catch (e) {
        print("Script error:", e.message);
        if (e.stack) {
            print("Stack trace:");
            print(e.stack);
        }
        return false;
    }
}

// Usage
safeExecute(function() {
    // Risky code
    const buf = editor.GetActiveBuffer();
    const line = buf.GetLineText(99999);  // Out of bounds
});
```

### Inspect Values

```javascript
// Detailed inspection
function inspect(obj) {
    print("Type:", typeof obj);
    if (obj === null) {
        print("null");
    } else if (typeof obj === "object" || typeof obj === "function") {
        print("Keys:", Object.keys(obj).join(", "));
    } else {
        print("Value:", obj);
    }
}

// Debug buffer state
function debugState() {
    const buf = editor.GetActiveBuffer();
    inspect(buf);
    print("Num lines:", buf.GetNumLines());
    print("Cursor:", buf.GetCursor());
}
```

## Limitations & Gotchas

1. **No `JSON.parse` stringify** – JSON object available but limited
2. **No promises/async** – Synchronous execution only
3. **No closures with upvalues** – Limited functional programming
4. **Max string length** – Configurable heap limit (~8MB default)
5. **No tail call optimization** – Deep recursion may stack overflow
6. **RegExp limits** – Complex patterns may hit backtracking limits

## Next Steps

- **Read Editor API** – Explore `include/zep/editor.h` for available methods
- **Security deep dive** – See `docs/SECURITY_REPORT.md` for sandbox internals
- **Interactive REPL** – Type `:repl` then select Duktape mode
- **Compare runtimes** – Test Lua vs Duktape vs QuickJS for your use case

## Comparison: Duktape vs Lua vs QuickJS

| Feature | Duktape | Lua | QuickJS |
|---------|---------|-----|---------|
| **Syntax** | JavaScript (ES2020) | Lua 5.4 | JavaScript (ES2020+) |
| **Binary Size** | ~200–400 KB | ~300–500 KB | ~300–600 KB |
| **Familiarity** | High (web devs) | Medium | High |
| **Performance** | Fast interpretation | Very fast (LuaJIT faster) | Moderate, JIT optional |
| **Standard Lib** | Partial JS stdlib | Full Lua stdlib | Full JS stdlib + modules |
| **Debugging** | Stack traces only | Debug lib disabled | Bytecode debug support |

---

**See also:**
- `README_REPL.md` – Overview of all REPL providers
- `docs/lua_scripting.md` – Lua alternative
- `docs/quickjs_scripting.md` – QuickJS alternative
- `docs/SECURITY_REPORT.md` – Sandbox security model
