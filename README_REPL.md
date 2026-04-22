# REPL/SCRIPTING Language Integration - Documentation

## Overview

pZep supports optional integration with multiple embedded scripting/repl languages. This document provides introductory to intermediate documentation for the three supported languages: Lua, Duktape, and QuickJS.

## Architecture

The REPL infrastructure uses a provider pattern with a common interface (`IZepReplProvider`). Each language implements this interface to provide:

- Code parsing and validation
- Execution sandboxing
- Editor API bindings
- Error handling and reporting

### Core Components

- **`IZepReplProvider`** (in `include/zep/mode_repl.h`): Abstract interface all providers implement
- **`ZepReplEvaluateCommand`**: Executes complete forms/statements
- **`ZepReplEvaluateOuterCommand`**: Evaluates outermost expressions
- **`ZepReplEvaluateInnerCommand`**: Evaluates inner subexpressions
- **Provider Registration**: `RegisterLuaReplProvider`, `RegisterDuktapeReplProvider`, `RegisterQuickJSEvalReplProvider`

## Language-Specific Documentation

### Lua

#### Overview
Lua is a lightweight, embeddable scripting language designed for extensibility. It's widely used in game engines and embedded systems due to its small footprint and simple C API.

#### Integration Details
- **Source**: `src/mode_lua_repl.cpp`
- **Header inclusion**: `#include "zep/mode_repl.h"`
- **Factory function**: `CreateLuaReplProvider()` / `DestroyLuaReplProvider()`
- **Library**: Links against Lua 5.4+ (or LuaJIT optionally)

#### Security Model
- Runs in a dedicated Lua state (`lua_State*`)
- Metatable-based sandboxing restricts global environment access
- Only editor-bridge functions are exposed to scripts
- No direct filesystem or OS API access by default

#### Key Features
- Full Lua 5.4 syntax support
- Incremental garbage collection
- Lightweight coroutines for async patterns
- Configurable memory limits

#### Getting Started
```cpp
// Register Lua provider during editor initialization
RegisterLuaReplProvider(editor);
```

#### Use Cases
- Game logic scripting
- Configuration and rule definition
- User-defined macros and automation
- Dynamic content generation

---

### Duktape

#### Overview
Duktape is an ultra-lightweight embeddable JavaScript engine with a focus on portability and minimal footprint. It's designed to be compiled with minimal dependencies.

#### Integration Details
- **Source**: `src/mode_duktape_repl.cpp`
- **Header inclusion**: `#include "zep/mode_repl.h"`
- **Factory function**: `CreateDuktapeReplProvider()` / `DestroyDuktapeReplProvider()`
- **Library**: Single-header, compiles directly into pZep (no external dependency)

#### Security Model
- Runs in a dedicated Duktape heap context
- Property-based access control for global object
- Custom C function bindings for editor API exposure
- No built-in Node.js-style filesystem access

#### Key Features
- ECMAScript 2020+ subset support
- Very small binary footprint (~200-400KB)
- No external dependencies
- Configurable call stack limits
- Built-in JSON support

#### Getting Started
```cpp
// Register Duktape provider during editor initialization
RegisterDuktapeReplProvider(editor);
```

#### Use Cases
- Web-inspired scripting in desktop tools
- Quick prototyping and testing
- Cross-platform compatibility priority
- Minimal deployment footprint requirements

---

### QuickJS

#### Overview
QuickJS is a small and complete JavaScript engine that supports modern ECMAScript features. It's known for its balance between size, performance, and feature completeness.

#### Integration Details
- **Source**: `src/mode_quickjs_repl.cpp`
- **Header inclusion**: `#include "zep/mode_repl.h"`
- **Factory function**: `CreateQuickJSEvalReplProvider()` / `DestroyQuickJSEvalReplProvider()`
- **Library**: Optional external dependency or bundled source

#### Security Model
- Runs in isolated QuickJS runtime context
- Bytecode verification before execution
- Controlled host function binding
- Memory usage limits configurable per runtime

#### Key Features
- Modern JavaScript (ES2020+) support
- Bytecode compilation and caching
- Module system support (customizable)
- Debugger bytecode instrumentation
- Small (~300-600KB with JIT disabled)

#### Getting Started
```cpp
// Register QuickJS provider during editor initialization
RegisterQuickJSEvalReplProvider(editor);
```

#### Use Cases
- Modern JavaScript tooling
- Plugin systems with language expressiveness
- Cross-runtime code sharing
- Developer tools with JS extensibility

---

## Common Configuration

### Build Options

All three languages are optional features controlled via CMake:

```bash
# Enable specific language support
cmake -DENABLE_LUA_REPL=ON -DENABLE_DUKTAPE_REPL=ON -DENABLE_QUICKJS_REPL=ON ..
```

If unspecified, providers are compiled but can be enabled/disabled at runtime via configuration.

### Runtime Configuration

Providers can be configured through `zep.cfg`:

```ini
[repl]
; Which provider to use (lua, duktape, quickjs)
active_provider = lua

; Resource limits
max_execution_time_ms = 500
memory_limit_kb = 10240
```

### Toggle Between Runtimes

The REPL command system supports multiple registered providers. Users can switch between languages at runtime through configuration or UI controls.

## Security Considerations

All providers implement the same security posture:

1. **Sandboxed Execution**: Each language runs in an isolated runtime
2. **API Restriction**: Only editor-bridge APIs are exposed
3. **Input Validation**: All code undergoes validation before execution
4. **Resource Limits**: Execution time and memory can be bounded
5. **No OS Access**: No direct filesystem, network, or system call access

## Integration Pattern

To add a new REPL command handler:

```cpp
class MyReplCommand : public ZepExCommand {
public:
    void Run(const std::vector<std::string>& args) override {
        IZepReplProvider* provider = GetEditor().GetActiveReplProvider();
        if (provider) {
            std::string result = provider->ReplParse(inputBuffer, cursor, type);
            // Handle result
        }
    }
};
```

## Troubleshooting

### Provider Not Found
- Ensure the language library is linked (check CMake output)
- Verify factory function is exported with `extern "C"`

### Security Errors
- Check that provider sandbox is properly initialized
- Verify metatable/proxy configurations are applied
- Review API exposure boundaries

### Performance Issues
- Enable bytecode caching (QuickJS)
- Adjust resource limits in configuration
- Consider JIT settings for QuickJS/LuaJIT