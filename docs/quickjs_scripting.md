# QuickJS Scripting in pZep

pZep supports optional QuickJS scripting integration, delivering a modern, complete JavaScript engine with ES2020+ support. QuickJS is a small yet comprehensive JavaScript engine that balances feature completeness with a compact footprint (~300–600 KB with JIT disabled). It supports modern ECMAScript features, modules, and bytecode compilation.

## Architecture

QuickJS integration uses the same **sandboxed provider architecture**:

```
┌─────────────────────────────────────────────┐
│  pZep Editor (Core C++ Engine)              │
├─────────────────────────────────────────────┤
│  IZepReplProvider Interface                 │
├─────────────────────────────────────────────┤
│  QuickJSReplProvider (Sandbox)              │
│  ├── JSRuntime* (isolated runtime)          │
│  ├── Bytecode verification                   │
│  ├── Controlled host function binding       │
│  ├── Memory limits per runtime              │
│  └── ES2020+ full language support          │
└─────────────────────────────────────────────┘
```

**Key Capabilities:**
- **Modern ES2020+** – async/await, optional chaining, nullish coalescing,BigInt, etc.
- **Bytecode compilation & caching** – Faster startup on repeated execution
- **Module system** – ES6 modules (limited by sandbox)
- **Debugger bytecode instrumentation** – Future debugging support
- **Full standard library** – Complete JS built-in objects

## Quick Start

### Enable QuickJS Support

QuickJS is compiled in by default. No extra configuration required.

### JavaScript ES2020+ Features

```javascript
// Modern syntax works out of the box

// Optional chaining
let config = editor.GetConfig();
let theme = config?.theme?.name ?? "default";
print("Theme:", theme);

// Nullish coalescing
let lineNum = null;
let result = lineNum ?? 0;  // Use 0 if null/undefined

// Template literals with expressions
let buf = editor.GetActiveBuffer();
print(`Buffer: ${buf.GetName()} (${buf.GetLength()} bytes)`);

// Arrow functions
const double = x => x * 2;
double(21)
42

// Destructuring
let [first, second] = [1, 2];
print(first, second);  // 1 2

// Spread operator
let nums = [1, 2, 3];
let more = [...nums, 4, 5];
print(more);  // [1,2,3,4,5]

// BigInt for large integers
let big = 9007199254740991n + 1n;
print(big);  // 9007199254740992n
```

### QuickJS as Calculator

```javascript
// Matrix multiplication helper
function matMul(a, b) {
    if (a[0].length !== b.length) throw "Incompatible";
    const result = [];
    for (let i = 0; i < a.length; i++) {
        result[i] = [];
        for (let j = 0; j < b[0].length; j++) {
            let sum = 0;
            for (let k = 0; k < b.length; k++) {
                sum += a[i][k] * b[k][j];
            }
            result[i][j] = sum;
        }
    }
    return result;
}

// 2x2 matrix multiply
const A = [[1, 2], [3, 4]];
const B = [[5, 6], [7, 8]];
matMul(A, B)
/* [
   [19, 22],
   [43, 50]
] */

// Stateless pure function with default params
function clamp(value, min = 0, max = 1) {
    return Math.min(Math.max(value, min), max);
}
clamp(1.5, 0, 1)
1
```

## Editor Automation

QuickJS scripts interact with pZep through the **Editor Bridge API** – same secure interface as other providers.

### Buffer Operations

```javascript
// Get active buffer
const buf = editor.GetActiveBuffer();

// Buffer properties
print("Name:", buf.GetName());
print("Length:", buf.GetLength());
print("Lines:", buf.GetNumLines());
print("Modified:", buf.IsModified());

// Cursor
const cursor = buf.GetCursor();
print(`Cursor at ${cursor.line}:${cursor.column}`);

// Read line
const line = buf.GetLineText(cursor.line);
print("Current line:", line);

// Insert text
buf.Insert(cursor.line, cursor.column, "// inserted via QuickJS\n");

// Save
buf.Save();
```

### Practical Examples

#### Example 1: Smart Comment Toggle

```javascript
// Detect comment style (//, /* */, #) and toggle appropriately
function smartToggleComment() {
    const buf = editor.GetActiveBuffer();
    const cursor = buf.GetCursor();
    const line = buf.GetLineText(cursor.line).trim();

    // JavaScript/CPP style //
    if (line.startsWith("//")) {
        buf.ReplaceLine(cursor.line, line.slice(2).trimStart());
        print("Uncommented (//)");
    }
    // C-style block comment /* */
    else if (line.startsWith("/*") && line.endsWith("*/")) {
        const inner = line.slice(2, -2).trimStart();
        buf.ReplaceLine(cursor.line, inner);
        print("Uncommented (block)");
    }
    // Hash comment (Python, shell)
    else if (line.startsWith("#")) {
        buf.ReplaceLine(cursor.line, line.slice(1).trimStart());
        print("Uncommented (#)");
    }
    // Default: add // comment
    else {
        buf.ReplaceLine(cursor.line, "// " + line);
        print("Commented (//)");
    }
}

// Bind to key in zep.cfg:
// :map <F7> :quickjs smartToggleComment()<CR>
```

#### Example 2: Insert ISO 8601 Timestamp with Timezone

```javascript
// Insert formatted timestamp with configurable format
function insertTimestamp(format) {
    format = format || "iso";  // iso, rfc3339, custom

    const buf = editor.GetActiveBuffer();
    const cursor = buf.GetCursor();

    let timestamp;
    const now = new Date();

    switch (format) {
        case "rfc3339":
            timestamp = now.toISOString();  // Already RFC 3339
            break;
        case "unix":
            timestamp = Math.floor(now.getTime() / 1000).toString();
            break;
        case "custom":
            // Custom format with timezone offset
            const offset = -now.getTimezoneOffset();
            const sign = offset >= 0 ? "+" : "-";
            const absOffset = String(Math.abs(offset / 60)).padStart(2, "0");
            timestamp = now.toISOString().replace("Z", sign + absOffset + ":00");
            break;
        default:  // iso
            timestamp = now.toISOString();
    }

    buf.Insert(cursor.line, cursor.column, "/* " + timestamp + " */ ");
    print("Inserted timestamp:", timestamp);
}

// Usage: :quickjs insertTimestamp("rfc3339")
```

#### Example 3: Advanced Buffer Statistics

```javascript
// Comprehensive analysis with histogram
function analyzeBuffer() {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();

    const stats = {
        totalChars: 0,
        totalWords: 0,
        totalLines: numLines,
        maxLineLength: 0,
        emptyLines: 0,
        commentLines: 0,
        codeLines: 0,
        indentLevels: {},
        langGuesses: {js: 0, py: 0, cpp: 0, other: 0}
    };

    for (let i = 0; i < numLines; i++) {
        const rawLine = buf.GetLineText(i);
        const line = rawLine.trim();

        // Character count (including whitespace)
        stats.totalChars += rawLine.length;
        stats.maxLineLength = Math.max(stats.maxLineLength, rawLine.length);

        // Word count
        const words = line.match(/\S+/g);
        if (words) stats.totalWords += words.length;

        // Empty line
        if (line.length === 0) {
            stats.emptyLines++;
            continue;
        }

        // Comment detection
        if (line.startsWith("//") || line.startsWith("#") ||
            (line.startsWith("/*") && line.endsWith("*/")) ||
            line.startsWith("--")) {
            stats.commentLines++;
        } else {
            stats.codeLines++;
        }

        // Indentation level
        const indent = rawLine.match(/^\s*/)[0].length;
        stats.indentLevels[indent] = (stats.indentLevels[indent] || 0) + 1;

        // Language guess by extension/comment style
        const ext = buf.GetName().split('.').pop().toLowerCase();
        if (line.startsWith("//")) stats.langGuesses.js++;
        else if (line.startsWith("#")) stats.langGuesses.py++;
        else if (line.startsWith("/*") || line.includes("{")) stats.langGuesses.cpp++;
    }

    // Print report
    print("=== Buffer Analysis ===");
    print(`Total lines:   ${stats.totalLines}`);
    print(`Code lines:    ${stats.codeLines}`);
    print(`Comment lines: ${stats.commentLines}`);
    print(`Empty lines:   ${stats.emptyLines}`);
    print(`Words:         ${stats.totalWords}`);
    print(`Characters:    ${stats.totalChars}`);
    print(`Max line:      ${stats.maxLineLength} chars`);

    // Indentation summary
    print("\nIndentation distribution:");
    const sortedIndents = Object.keys(stats.indentLevels).sort((a,b) => parseInt(a) - parseInt(b));
    for (let indent of sortedIndents.slice(0, 5)) {
        print(`  ${indent} spaces: ${stats.indentLevels[indent]} lines`);
    }
}
```

#### Example 4: Regex-Based Refactoring

```javascript
// Convert camelCase to snake_case throughout buffer
function camelToSnakeCase() {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();
    let changes = 0;

    // Match camelCase identifiers (not at line start)
    const camelRegex = /(?<!^)([A-Z])/g;

    for (let i = 0; i < numLines; i++) {
        const line = buf.GetLineText(i);

        // Skip commented lines
        if (line.trim().startsWith("//") || line.trim().startsWith("#")) {
            continue;
        }

        const newLine = line.replace(camelRegex, (match, p1, offset, string) => {
            // Don't change inside string literals
            const before = string.slice(0, offset);
            const inString = (before.match(/"/g) || []).length % 2 === 1 ||
                            (before.match(/'/g) || []).length % 2 === 1;
            return inString ? match : "_" + p1.toLowerCase();
        });

        if (line !== newLine) {
            buf.ReplaceLine(i, newLine);
            changes++;
        }
    }

    print("CamelCase → snake_case: " + changes + " changes");
}
```

## Configuration & Keymaps

### Setting Up QuickJS Keybindings

```vim
" Common QuickJS shortcuts
:map <F5> :quickjs analyzeBuffer()<CR>
:map <F6> :quickjs insertTimestamp()<CR>
:map <F7> :quickjs smartToggleComment()<CR>
:map <F8> :quickjs trimWhitespace()<CR>
:map <C-F12> :quickjs camelToSnakeCase()<CR>
```

### Startup Configuration

```javascript
// ~/.config/pzep/quickjs/init.js
// Loaded on QuickJS provider registration

// === Utilities ===

// Pad number with leading zeros
function pad(num, size) {
    let s = String(num);
    while (s.length < size) s = "0" + s;
    return s;
}

// Format file size in human-readable form
function formatBytes(bytes) {
    const units = ["B", "KB", "MB", "GB"];
    let i = 0;
    while (bytes >= 1024 && i < units.length - 1) {
        bytes /= 1024;
        i++;
    }
    return bytes.toFixed(2) + " " + units[i];
}

// === Commands ===

// Show buffer info in status bar
function showBufferInfo() {
    const buf = editor.GetActiveBuffer();
    editor.SetStatusText([
        buf.GetName(),
        formatBytes(buf.GetLength()),
        "L:" + buf.GetNumLines()
    ].join(" | "));
}

// Trim trailing whitespace on save hook
function onBeforeSave() {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();
    let trimmed = 0;

    for (let i = 0; i < numLines; i++) {
        const line = buf.GetLineText(i);
        const newLine = line.replace(/\s+$/, "");
        if (line !== newLine) {
            buf.ReplaceLine(i, newLine);
            trimmed++;
        }
    }

    if (trimmed > 0) {
        print("Trimmed " + trimmed + " trailing whitespace");
    }
}

// Register hook (if supported)
editor.OnBeforeSave(onBeforeSave);

// Export globals
this.showBufferInfo = showBufferInfo;
this.formatBytes = formatBytes;
this.trimWhitespace = onBeforeSave;

print("QuickJS QuickJS environment initialized");
```

## Security Model

QuickJS runs in an **isolated JSRuntime** with bytecode verification and controlled host function exposure. It offers the most complete ES2020+ language support while maintaining sandbox boundaries.

### What CAN Do

- **Full ES2020+ syntax** – async/await, optional chaining, BigInt, etc.
- **All standard built-ins** – Array, Object, String, Math, Date, JSON, RegExp, etc.
- **TypedArray API** – Limited (Uint8Array for binary data)
- **Proxy objects** – For advanced metaprogramming
- **Generator functions** – `function*` and `yield`
- **Modules** – `import`/`export` (restricted to sandbox)
- **Reflect API** – Limited introspection

### What CANNOT Do

- **No `import` from external files** – Module loading disabled
- **No `eval()` in global scope** – Eval restricted to function-local
- **No `new Function()`** – Dynamic compilation blocked
- **No `WebAssembly`** – WASM compilation disabled
- **No `Atomics` or `SharedArrayBuffer`** – No threading primitives
- **No `fetch`/`XMLHttpRequest`** – No network I/O
- **No `File`/`Blob`** – No filesystem access
- **No `process`** – No Node.js globals or modules
- **No `console`** – Only `print()` available
- **No `require()`** – No CommonJS

### Restricted Global Objects

```javascript
// Blocked globals (removed):
// require, module, exports, process, global, console
// setTimeout, setInterval, clearTimeout, clearInterval
// Worker, SharedArrayBuffer, Atomics, fetch
// XMLHttpRequest, WebSocket, WebRTC
// fs, path, net, http, https, crypto (Node.js modules)
// import.meta (limited)
// document, window (browser APIs)

// Available safe globals:
Array, ArrayBuffer, Boolean, DataView, Date, Error, EvalError, Float32Array
Float64Array, Function, Infinity, Int16Array, Int32Array, Int8Array, JSON
Map, Math, NaN, Number, Object, Promise, Proxy, RangeError, ReferenceError
RegExp, Set, String, Symbol, SyntaxError, TypeError, Uint16Array, Uint32Array
Uint8Array, Uint8ClampedArray, WeakMap, WeakSet, decodeURI, decodeURIComponent
encodeURI, encodeURIComponent, escape, eval (restricted), isFinite, isNaN, parseFloat
parseInt, print, undefined, uneval, URIError
```

### Resource Limits

```ini
[quickjs]
; Max execution time per script (ms)
max_execution_time_ms = 750

; Memory limit per runtime (KB)
memory_limit_kb = 16384

; Enable QuickJS scripting
enabled = true

; Enable JIT compilation (if supported)
jit_enabled = false

; Bytecode cache directory (if persistent cache used)
bytecode_cache_dir =
```

### Error Handling Pattern

```javascript
// Comprehensive error wrapper
function safeExecute(fn, context) {
    try {
        const result = fn.call(context);
        return {success: true, value: result};
    } catch (e) {
        // QuickJS provides stack traces
        const msg = e.message || String(e);
        const stack = e.stack || "";

        print("Script failed: " + msg);
        if (stack) {
            print("Stack trace:");
            print(stack);
        }

        return {success: false, error: e};
    }
}

// Usage with async pattern (sync in current sandbox)
const result = safeExecute(() => {
    const buf = editor.GetActiveBuffer();
    return buf.GetName();
});

if (result.success) {
    print("Buffer name:", result.value);
}
```

## Advanced Editor Integration

### Working with Character Position Tracking

```javascript
// Index all words in buffer for navigation
function buildWordIndex() {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();
    const index = new Map();  // word → [{line, col}]

    for (let i = 0; i < numLines; i++) {
        const line = buf.GetLineText(i);
        const words = line.match(/\w+/g);

        if (words) {
            let colOffset = 0;
            for (let word of words) {
                colOffset = line.indexOf(word, colOffset);
                const entries = index.get(word) || [];
                entries.push({line: i, col: colOffset});
                index.set(word, entries);
                colOffset += word.length;
            }
        }
    }

    return index;  // Map of word → positions
}

// Jump to next occurrence of word under cursor
function jumpToNextWord() {
    const buf = editor.GetActiveBuffer();
    const cursor = buf.GetCursor();
    const line = buf.GetLineText(cursor.line);

    // Get word at cursor
    const linePrefix = line.slice(0, cursor.column);
    const matches = linePrefix.match(/\w+$/);
    if (!matches) {
        print("No word under cursor");
        return;
    }

    const word = matches[0];
    const index = buildWordIndex();
    const positions = index.get(word);

    if (!positions || positions.length <= 1) {
        print("Word appears only once");
        return;
    }

    // Find current position in list
    let currentIdx = -1;
    for (let i = 0; i < positions.length; i++) {
        if (positions[i].line === cursor.line &&
            positions[i].col <= cursor.column) {
            currentIdx = i;
        }
    }

    // Jump to next
    const nextIdx = (currentIdx + 1) % positions.length;
    const target = positions[nextIdx];
    editor.MoveCursorTo(target.line, target.col);
    print("Jumped to:", word, "at line", target.line + 1);
}
```

### Syntax Tree Traversal (Simple)

```javascript
// Find matching bracket pairs
function findMatchingBracket(line, col) {
    const buf = editor.GetActiveBuffer();
    const pairs = {
        '(': ')', '[': ']', '{': '}',
        ')': '(', ']': '[', '}': '{'
    };
    const opens = "([{";
    const closes = ")]}";

    const text = buf.GetLineText(line);
    const char = text[col];

    if (!pairs[char]) {
        print("Not a bracket character");
        return;
    }

    let stack = [];
    const direction = opens.includes(char) ? 1 : -1;
    const startCol = col;

    // Search forward or backward
    let l = line;
    let c = col;
    while (true) {
        const currentLine = buf.GetLineText(l);
        const currentChar = currentLine[c];

        if (opens.includes(currentChar)) {
            stack.push(currentChar);
        } else if (closes.includes(currentChar)) {
            const matching = pairs[currentChar];
            if (stack.length > 0 && stack[stack.length - 1] === matching) {
                stack.pop();
            } else {
                stack.push(currentChar);
            }
        }

        // Check if we found match
        if (stack.length === 0 && ((direction === 1 && l !== line) || (direction === -1 && (l !== line || c !== startCol)))) {
            print(`Matching bracket at line ${l+1}, col ${c}`);
            return {line: l, col: c};
        }

        // Move
        c += direction;
        if (c < 0) {
            l--;
            if (l < 0) break;
            c = buf.GetLineText(l).length - 1;
        } else if (c >= currentLine.length) {
            l++;
            if (l >= buf.GetNumLines()) break;
            c = 0;
        }
    }

    print("No matching bracket found");
}
```

### Batch Refactoring Tools

```javascript
// Rename symbol across buffer (simple text replace)
function renameSymbol(oldName, newName) {
    const buf = editor.GetActiveBuffer();
    const numLines = buf.GetNumLines();
    let changes = 0;

    // Word boundary regex
    const pattern = new RegExp("\\b" + oldName.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + "\\b", "g");

    for (let i = 0; i < numLines; i++) {
        const line = buf.GetLineText(i);

        // Skip string literals and comments
        if (line.trim().startsWith("//") || line.trim().startsWith("#")) continue;

        const newLine = line.replace(pattern, newName);
        if (newLine !== line) {
            buf.ReplaceLine(i, newLine);
            changes++;
        }
    }

    print(`Renamed "${oldName}" → "${newName}": ${changes} occurrences`);
}

// Extract function to variable (basic)
function extractToVariable(startLine, endLine, varName) {
    const buf = editor.GetActiveBuffer();

    // Collect lines
    const lines = [];
    for (let i = startLine; i <= endLine; i++) {
        lines.push(buf.GetLineText(i));
    }

    // Create variable assignment
    const indent = buf.GetLineText(startLine).match(/^\s*/)[0];
    const code = lines.join("\n");
    const declaration = indent + "const " + varName + " = " + code + ";";

    // Replace first line with declaration, remove rest
    buf.ReplaceLine(startLine, declaration);
    for (let i = endLine; i > startLine; i--) {
        buf.DeleteLine(i);
    }

    print("Extracted to variable:", varName);
}
```

## Enterprise Use Cases

### 1. Code Quality Gate

```javascript
// Pre-commit quality checks
function qualityGate() {
    const buf = editor.GetActiveBuffer();
    const filename = buf.GetName();
    const issues = [];

    for (let i = 0; i < buf.GetNumLines(); i++) {
        const line = buf.GetLineText(i);
        const lineNum = i + 1;

        // 1. No console.log (except in test files)
        if (!filename.includes(".test.") && line.match(/console\.(log|debug|info)/)) {
            issues.push(`L${lineNum}: console.* statement in production code`);
        }

        // 2. No var declarations (use let/const)
        if (line.match(/^\s*var\s+/)) {
            issues.push(`L${lineNum}: Use 'let' or 'const', not 'var'`);
        }

        // 3. No magic numbers > 1000 (except in specific files)
        const magic = line.match(/\b(\d{4,})\b/);
        if (magic && !filename.includes("config")) {
            issues.push(`L${lineNum}: Magic number ${magic[1]} – define constant`);
        }

        // 4. No duplicate string literals (simple check)
        if (line.match(/".*".*".*"/) || line.match(/'.*'.*'.*/)) {
            issues.push(`L${lineNum}: Possible magic string literal`);
        }

        // 5. Check for == vs ===
        if (line.match(/[^=!]==[^=]/)) {
            issues.push(`L${lineNum}: Use === instead of == for comparison`);
        }
    }

    // Summary
    if (issues.length > 0) {
        print("=== Quality Gate FAILED ===");
        issues.slice(0, 10).forEach(msg => print("  " + msg));
        if (issues.length > 10) {
            print(`  ...and ${issues.length - 10} more`);
        }
        print(`\nTotal: ${issues.length} issues found`);
        return false;
    } else {
        print("✓ Quality gate passed");
        return true;
    }
}
```

### 2. API Version Tracker

```javascript
// Track API surface changes across files
function checkApiVersioning() {
    const buf = editor.GetActiveBuffer();
    const filename = buf.GetName();

    // Extract exported names
    const exports = new Set();
    const imports = new Set();

    for (let i = 0; i < buf.GetNumLines(); i++) {
        const line = buf.GetLineText(i);

        // export default
        const def = line.match(/export\s+default\s+(\w+)/);
        if (def) exports.add(def[1]);

        // export const/let/var/function
        const exp = line.match(/export\s+(?:const|let|var|function)\s+(\w+)/);
        if (exp) exports.add(exp[1]);

        // import statements
        const imp = line.match(/import\s+{?\s*(\w+)\s*}?\s+from/);
        if (imp) imports.add(imp[1]);
    }

    print("=== API Analysis: " + filename + " ===");
    print("Exports:", Array.from(exports).join(", "));
    print("Imports:", Array.from(imports).join(", "));

    // Check for common version issues
    const breakingChanges = [];

    // New export not in known set (would require bump)
    const knownExports = new Set(["v1", "v2", "legacy"]);  // from config
    for (let exp of exports) {
        if (!knownExports.has(exp)) {
            breakingChanges.push(`New export '${exp}' – major version bump?`);
        }
    }

    if (breakingChanges.length > 0) {
        print("\n⚠ Breaking changes detected:");
        breakingChanges.forEach(msg => print("  " + msg));
    }
}
```

### 3. Documentation Generator

```javascript
// Generate Markdown API docs from JSDoc comments
function generateApiDocs() {
    const buf = editor.GetActiveBuffer();
    const lines = buf.GetLineText(0, buf.GetNumLines() - 1);  // Get all

    const functions = [];
    let inComment = false;
    let currentComment = "";
    let currentLine = 0;

    for (let i = 0; i < buf.GetNumLines(); i++) {
        const line = buf.GetLineText(i).trim();

        if (line.startsWith("/**")) {
            inComment = true;
            currentComment = line.slice(3).trim();
            currentLine = i;
        } else if (line.startsWith("*/") && inComment) {
            inComment = false;
            // Next non-empty line should be function signature
            let j = i + 1;
            while (j < buf.GetNumLines() && buf.GetLineText(j).trim() === "") j++;
            if (j < buf.GetNumLines()) {
                const sig = buf.GetLineText(j).trim();
                const match = sig.match(/(?:function|const)\s+(\w+)\s*\(/);
                if (match) {
                    functions.push({
                        name: match[1],
                        line: currentLine + 1,
                        doc: currentComment.replace(/\*\s*/g, "").trim()
                    });
                }
            }
        } else if (inComment) {
            currentComment += " " + line.replace(/^\s*\*\s?/, "");
        }
    }

    // Output as Markdown
    print("### API Documentation\n");
    for (const fn of functions) {
        print("#### " + fn.name + " (Line " + fn.line + ")");
        print(fn.doc || "No description");
        print("");
    }

    print("Generated " + functions.length + " API entries");
}
```

### 4. License & Copyright Updater

```javascript
// Update copyright year in headers across project
function updateCopyrightYears() {
    const buf = editor.GetActiveBuffer();
    const currentYear = new Date().getFullYear();
    let updated = 0;

    for (let i = 0; i < buf.GetNumLines(); i++) {
        const line = buf.GetLineText(i);

        // Match copyright lines with year ranges
        const updatedLine = line.replace(
            /Copyright \(c\) (\d{4})-(\d{4})/,
            `Copyright (c) $1-${currentYear}`
        );

        if (updatedLine !== line) {
            buf.ReplaceLine(i, updatedLine);
            updated++;
        }
    }

    print(`Updated ${updated} copyright year(s) to ${currentYear}`);
}
```

### 5. Dependency License Aggregator

```javascript
// Generate SPDX-style license report
function generateLicenseReport() {
    const buf = editor.GetActiveBuffer();
    const deps = new Map();

    // Extract dependencies from package.json or imports
    for (let i = 0; i < buf.GetNumLines(); i++) {
        const line = buf.GetLineText(i);

        // "dependency": "version"
        const pkg = line.match(/"([^"]+)":\s*"([^"]+)"/);
        if (pkg) {
            deps.set(pkg[1], {version: pkg[2], license: "MIT"});  // Placeholder
        }
    }

    print("### Dependency Licenses\n");
    for (let [pkg, info] of deps) {
        print(`| ${pkg.padEnd(30)} | ${info.version.padStart(10)} | ${info.license} |`);
    }
}
```

## Performance Tips

### 1. Cache Method References

```javascript
// Cache editor/buffer methods
const getName = buf.GetName.bind(buf);
const getLine = buf.GetLineText.bind(buf);
const replace = buf.ReplaceLine.bind(buf);

// Faster in loops
for (let i = 0; i < numLines; i++) {
    const line = getLine(i);
    // Process...
}
```

### 2. Use Efficient Data Structures

```javascript
// For membership tests, use Set
const keywords = new Set(["function", "const", "let", "var", "return"]);
if (keywords.has(word)) { /* fast O(1) lookup */ }

// For key-value maps, use Map (preserves types)
const wordPositions = new Map();  // string → array
```

### 3. Batch Updates

```javascript
// Collect changes, apply once
const changes = [];

// First pass: collect
for (let i = 0; i < buf.GetNumLines(); i++) {
    const line = buf.GetLineText(i);
    if (needsChange(line)) {
        changes.push({line: i, new: transform(line)});
    }
}

// Apply in reverse order to preserve line numbers
changes.reverse().forEach(c => {
    buf.ReplaceLine(c.line, c.new);
});
```

## Debugging QuickJS Scripts

### Print Debugging

```javascript
// Detailed object inspection
function dump(obj, depth = 0) {
    const indent = "  ".repeat(depth);
    if (obj === null) print(indent + "null");
    else if (typeof obj === "object") {
        print(indent + "{");
        for (let key in obj) {
            if (obj.hasOwnProperty(key)) {
                print(indent + "  " + key + ":");
                dump(obj[key], depth + 2);
            }
        }
        print(indent + "}");
    } else if (typeof obj === "function") {
        print(indent + "[Function " + (obj.name || "anonymous") + "]");
    } else {
        print(indent + String(obj));
    }
}

// Debug buffer state
function debugBuffer() {
    const buf = editor.GetActiveBuffer();
    dump(buf);
}
```

### Profiling Timing

```javascript
// Measure script execution time
function timed(fn) {
    const start = Date.now();
    fn();
    const elapsed = Date.now() - start;
    print(`Executed in ${elapsed}ms`);
}

// Usage
timed(() => {
    bufferStats();
    formatCode();
});
```

## Limitations & Gotchas

1. **No `import.meta.url`** – Module URL resolution limited
2. **No `SharedArrayBuffer`** – No multi-threading support
3. **JIT disabled by default** – Performance lower than V8 (but still fast)
4. **No native addons** – All C bindings must be pre-compiled
5. **Memory limits strict** – Exceeding heap throws exception
6. **No `Atomics`** – Cannot use Atomics.wait/notify
7. **Limited `Reflect`** – Some metaprogramming features restricted
8. **No `FinalizationRegistry`** – Finalizers not implemented
9. **BigInt arithmetic limits** – Very large integers slower
10. **Max call stack ~5000** – Deep recursion may overflow

## Troubleshooting

### Script Fails Silently

```javascript
// Always wrap in try-catch
try {
    // Your code
} catch (e) {
    print("ERROR:", e);
    print(e.stack);  // QuickJS provides stack trace
}
```

### Memory Limit Errors

```
Error: out of memory
```

**Fix:** Increase limit in `zep.cfg`:

```ini
[quickjs]
memory_limit_kb = 32768   ; 32 MB instead of default 16 MB
```

### Function Not Available

```
TypeError: undefined is not a function
```

Likely calling editor API incorrectly. Verify method exists (case-sensitive):

```javascript
// Correct:
buf.GetName();
editor.SetActiveBuffer(buf);

// Wrong:
buf.getname();  // lowercase not exposed
editor.setActiveBuffer();  // camelCase only
```

## Performance Comparison (QuickJS vs Duktape vs Lua)

| Metric | QuickJS | Duktape | Lua |
|--------|---------|---------|-----|
| **Startup time** | Medium (~10ms) | Fast (~5ms) | Fast (~3ms) |
| **Loop throughput** | Medium | Fast | Fastest |
| **Memory per script** | Higher (~2MB) | Lower (~1MB) | Lowest (~500KB) |
| **Garbage collection** | Incremental | Simple mark-sweep | Incremental |
| **Max script size** | ~1MB | ~500KB | ~500KB |

## Next Steps

- **Explore Editor API** – Review `include/zep/editor.h` for all available methods
- **Security deep dive** – Read `docs/SECURITY_REPORT.md`
- **Try REPL** – Type `:repl` then select QuickJS mode
- **Benchmark** – Compare Lua/Duktape/QuickJS for your specific workload

## API Reference Summary

| Category | Methods |
|----------|---------|
| **Buffer** | `GetName()`, `GetLength()`, `GetNumLines()`, `GetLineText(line)`, `GetTextRegion(sl, sc, el, ec)`, `Insert(line, col, text)`, `ReplaceLine(line, text)`, `ReplaceRegion(sl, sc, el, ec, text)`, `DeleteLine(line)`, `Save()`, `SaveAs(path)`, `IsModified()` |
| **Editor** | `GetActiveBuffer()`, `GetBuffers()`, `SetActiveBuffer(buf)`, `ExecuteCommand(cmd)`, `SetDisplayRegion(origin, size)`, `SetStatusText(msg)`, `UpdateDisplay()` |
| **Cursor** | `buf.GetCursor() → {line, column}`, `editor.MoveCursorTo(line, col)` |
| **Selection** | `editor.GetSelection() → {start:{line,col}, end:{line,col}}`, `editor.SetSelection(r)` |
| **Config** | `editor.GetConfig() → {showLineNumbers, cursorLineSolid, ...}` |
| **Utilities** | `print()`, `JSON.parse()`, `JSON.stringify()`, console-like output |

---

**See also:**
- `README_REPL.md` – REPL provider overview and switching
- `docs/lua_scripting.md` – Lua alternative (smaller, faster)
- `docs/duktape_scripting.md` – Duktape alternative (smallest)
- `docs/SECURITY_REPORT.md` – Sandbox security details
- `REPL_IMPLEMENTATION_SUMMARY.md` – Technical architecture
