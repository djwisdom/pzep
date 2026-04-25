#include "zep/buffer.h"
#include "zep/commands_repl.h"
#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mcommon/logger.h"
#include "zep/mode_repl.h"
#include "zep/repl_capabilities.h"
#include "zep/security.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#include <lua.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace Zep
{

// Lua REPL Provider implementation - Security-hardened
class LuaReplProvider : public IZepReplProvider
{
public:
    LuaReplProvider();
    ~LuaReplProvider();
    void Initialize(ZepEditor* pEditor);
    std::string ReplParse(ZepBuffer& text, const GlyphIterator& cursorOffset, ReplParseType type) override;
    std::string ReplParse(const std::string& text) override;
    bool ReplIsFormComplete(const std::string& input, int& depth) override;

private:
    // === SECURITY STATE ===
    struct SecurityState
    {
        std::atomic<int> instructionCount{ 0 };
        std::atomic<size_t> memoryUsed{ 0 };
        std::chrono::steady_clock::time_point executionStart;
        bool timeoutTriggered = false;
    };

    ZepEditor* m_pEditor = nullptr;
    lua_State* L = nullptr;
    std::unique_ptr<std::string> m_printOutput;
    std::shared_ptr<EditorCapability> m_edCap;
    SecurityState m_secState;

    // Helper methods
    static void luaHook(lua_State* L, lua_Debug* ar);
    static void* luaAlloc(void* ud, void* ptr, size_t osize, size_t nsize);
    static void luaPanic(lua_State* L);
    bool CheckTimeLimit() const;
    void ResetSecurityState();

    // Registry key for provider pointer
    static constexpr const char* PROVIDER_REGISTRY_KEY = "LuaReplProvider";
};

ZepEditor* m_pEditor = nullptr;
lua_State* L = nullptr;
std::unique_ptr<std::string> m_printOutput;
std::shared_ptr<EditorCapability> m_edCap;
SecurityState m_secState;

// Helper methods
static void luaHook(lua_State* L, lua_Debug* ar);
static void* luaAlloc(void* ud, void* ptr, size_t osize, size_t nsize);
static void luaPanic(lua_State* L);
bool CheckTimeLimit();
void ResetSecurityState();
};

// ============================================================
// Lua Security: Instruction counting hook
// ============================================================

void LuaReplProvider::luaHook(lua_State* L, lua_Debug* ar)
{
    ZEP_UNUSED(ar);
    LuaReplProvider* self = static_cast<LuaReplProvider*>(lua_getextraspace(L));
    if (self)
    {
        self->m_secState.instructionCount++;
        if (self->m_secState.instructionCount >= Security::LUA_MAX_INSTRUCTIONS)
        {
            self->m_secState.timeoutTriggered = true;
            luaL_error(L, "Execution limit exceeded (%d instructions)", Security::LUA_MAX_INSTRUCTIONS);
        }
        if (!self->CheckTimeLimit())
        {
            self->m_secState.timeoutTriggered = true;
            luaL_error(L, "Execution timeout (%d ms)", Security::LUA_EXECUTION_TIMEOUT_MS);
        }
    }
}

bool LuaReplProvider::CheckTimeLimit()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_secState.executionStart);
    if (elapsed.count() >= Security::LUA_EXECUTION_TIMEOUT_MS)
    {
        return false;
    }
    return true;
}

void LuaReplProvider::ResetSecurityState()
{
    m_secState.instructionCount = 0;
    m_secState.memoryUsed = 0;
    m_secState.timeoutTriggered = false;
    m_secState.executionStart = std::chrono::steady_clock::now();
}

// ============================================================
// Lua Security: Memory allocation tracking
// ============================================================

void* LuaReplProvider::luaAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    LuaReplProvider* self = static_cast<LuaReplProvider*>(ud);
    if (!self)
        return nullptr;

    // Track memory usage
    if (ptr)
    {
        self->m_secState.memoryUsed -= osize;
    }
    if (nsize > 0)
    {
        self->m_secState.memoryUsed += nsize;
        if (self->m_secState.memoryUsed > Security::LUA_MAX_MEMORY)
        {
            ZLOG(WARN, "Lua memory limit exceeded");
            return nullptr;
        }
    }

    if (nsize == 0)
    {
        free(ptr);
        return nullptr;
    }
    else
    {
        void* newptr = realloc(ptr, nsize);
        if (!newptr && nsize > 0)
        {
            self->m_secState.memoryUsed -= nsize;
        }
        return newptr;
    }
}

void LuaReplProvider::luaPanic(lua_State* L)
{
    const char* msg = lua_tostring(L, -1);
    ZLOG(ERROR, "Lua panic: " << (msg ? msg : "unknown error"));
    lua_close(L);
    std::abort();
}

// ============================================================
// Lua-side Capability Object Representation
// ============================================================

namespace
{

// Store shared_ptr<BufferCapability> as Lua userdata
void pushBufferCapability(lua_State* L, std::shared_ptr<BufferCapability> bufCap)
{
    std::shared_ptr<BufferCapability>* ud = (std::shared_ptr<BufferCapability>*)lua_newuserdata(L, sizeof(std::shared_ptr<BufferCapability>));
    new (ud) std::shared_ptr<BufferCapability>(std::move(bufCap));
    luaL_getmetatable(L, "ZepBufferCap");
    lua_setmetatable(L, -2);
}

std::shared_ptr<BufferCapability> checkBufferCapability(lua_State* L, int idx)
{
    void* ud = lua_touserdata(L, idx);
    if (!ud)
    {
        luaL_error(L, "Expected BufferCapability userdata");
        return nullptr;
    }
    std::shared_ptr<BufferCapability>* pShared = (std::shared_ptr<BufferCapability>*)ud;
    return *pShared;
}

// Store shared_ptr<EditorCapability> as Lua userdata
void pushEditorCapability(lua_State* L, std::shared_ptr<EditorCapability> edCap)
{
    std::shared_ptr<EditorCapability>* ud = (std::shared_ptr<EditorCapability>*)lua_newuserdata(L, sizeof(std::shared_ptr<EditorCapability>));
    new (ud) std::shared_ptr<EditorCapability>(std::move(edCap));
    luaL_getmetatable(L, "ZepEditorCap");
    lua_setmetatable(L, -2);
}

std::shared_ptr<EditorCapability> checkEditorCapability(lua_State* L, int idx)
{
    void* ud = lua_touserdata(L, idx);
    if (!ud)
    {
        luaL_error(L, "Expected EditorCapability userdata");
        return nullptr;
    }
    std::shared_ptr<EditorCapability>* pShared = (std::shared_ptr<EditorCapability>*)ud;
    return *pShared;
}

// ============================================================
// BufferCapability Lua Methods (Read-only)
// ============================================================

static int bufcap_GetName(lua_State* L)
{
    auto bufCap = checkBufferCapability(L, 1);
    std::string name = bufCap->GetName();
    lua_pushlstring(L, name.c_str(), name.size());
    return 1;
}

static int bufcap_GetLength(lua_State* L)
{
    auto bufCap = checkBufferCapability(L, 1);
    lua_pushinteger(L, (lua_Integer)bufCap->GetLength());
    return 1;
}

static int bufcap_GetLineCount(lua_State* L)
{
    auto bufCap = checkBufferCapability(L, 1);
    lua_pushinteger(L, (lua_Integer)bufCap->GetLineCount());
    return 1;
}

static int bufcap_GetLineText(lua_State* L)
{
    auto bufCap = checkBufferCapability(L, 1);
    long line = (long)luaL_checkinteger(L, 2);
    // SECURITY: Limit line index to prevent out-of-bounds access
    if (line < 0 || line >= bufCap->GetLineCount())
    {
        lua_pushstring(L, "");
        return 1;
    }
    std::string text = bufCap->GetLineText(line);
    // SECURITY: Truncate excessively long lines
    constexpr size_t MAX_LINE_RETURN = 4096;
    if (text.size() > MAX_LINE_RETURN)
        text.resize(MAX_LINE_RETURN);
    lua_pushlstring(L, text.c_str(), text.size());
    return 1;
}

static int bufcap_GetCursor(lua_State* L)
{
    auto bufCap = checkBufferCapability(L, 1);
    auto cursor = bufCap->GetCursor();
    lua_newtable(L);
    lua_pushinteger(L, (lua_Integer)cursor.first);
    lua_setfield(L, -2, "line");
    lua_pushinteger(L, (lua_Integer)cursor.second);
    lua_setfield(L, -2, "column");
    return 1;
}

static int bufcap_IsModified(lua_State* L)
{
    auto bufCap = checkBufferCapability(L, 1);
    lua_pushboolean(L, bufCap->IsModified());
    return 1;
}

// ============================================================
// EditorCapability Lua Methods (Read-only)
// ============================================================

static int edcap_GetBuffers(lua_State* L)
{
    auto edCap = checkEditorCapability(L, 1);
    auto buffers = edCap->GetBuffers();
    lua_newtable(L);
    int idx = 1;
    for (const auto& bufCap : buffers)
    {
        pushBufferCapability(L, bufCap);
        lua_rawseti(L, -2, idx++);
    }
    return 1;
}

static int edcap_GetActiveBuffer(lua_State* L)
{
    auto edCap = checkEditorCapability(L, 1);
    auto bufCap = edCap->GetActiveBuffer();
    if (bufCap)
    {
        pushBufferCapability(L, std::move(bufCap));
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

static int edcap_GetVersion(lua_State* L)
{
    auto edCap = checkEditorCapability(L, 1);
    std::string version = edCap->GetEditorVersion();
    lua_pushlstring(L, version.c_str(), version.size());
    return 1;
}

// ============================================================
// Garbage Collection
// ============================================================

static int bufcap_gc(lua_State* L)
{
    void* ud = lua_touserdata(L, 1);
    if (ud)
    {
        std::shared_ptr<BufferCapability>* pShared = (std::shared_ptr<BufferCapability>*)ud;
        pShared->~shared_ptr<BufferCapability>();
    }
    return 0;
}

static int edcap_gc(lua_State* L)
{
    void* ud = lua_touserdata(L, 1);
    if (ud)
    {
        std::shared_ptr<EditorCapability>* pShared = (std::shared_ptr<EditorCapability>*)ud;
        pShared->~shared_ptr<EditorCapability>();
    }
    return 0;
}

// ============================================================
// Metatable Registration
// ============================================================

void registerBufferCapabilityMetatable(lua_State* L)
{
    luaL_newmetatable(L, "ZepBufferCap");
    lua_newtable(L);
    lua_pushcfunction(L, bufcap_GetName);
    lua_setfield(L, -2, "GetName");
    lua_pushcfunction(L, bufcap_GetLength);
    lua_setfield(L, -2, "GetLength");
    lua_pushcfunction(L, bufcap_GetLineCount);
    lua_setfield(L, -2, "GetLineCount");
    lua_pushcfunction(L, bufcap_GetLineText);
    lua_setfield(L, -2, "GetLineText");
    lua_pushcfunction(L, bufcap_GetCursor);
    lua_setfield(L, -2, "GetCursor");
    lua_pushcfunction(L, bufcap_IsModified);
    lua_setfield(L, -2, "IsModified");
    lua_pushcfunction(L, bufcap_gc);
    lua_setfield(L, -2, "__gc");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void registerEditorCapabilityMetatable(lua_State* L)
{
    luaL_newmetatable(L, "ZepEditorCap");
    lua_newtable(L);
    lua_pushcfunction(L, edcap_GetBuffers);
    lua_setfield(L, -2, "GetBuffers");
    lua_pushcfunction(L, edcap_GetActiveBuffer);
    lua_setfield(L, -2, "GetActiveBuffer");
    lua_pushcfunction(L, edcap_GetVersion);
    lua_setfield(L, -2, "GetVersion");
    lua_pushcfunction(L, edcap_gc);
    lua_setfield(L, -2, "__gc");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

// ============================================================
// Print Capture
// ============================================================

static int l_print(lua_State* L)
{
    std::string* output = *(std::string**)lua_touserdata(L, lua_upvalueindex(1));
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++)
    {
        size_t len;
        const char* s = lua_tolstring(L, i, &len);
        if (s)
        {
            // SECURITY: Limit total output size
            if (output->size() + len < Security::LUA_MAX_OUTPUT_SIZE)
                output->append(s, len);
        }
        if (i < n)
            output->push_back('\t');
    }
    output->push_back('\n');
    return 0;
}

} // anonymous namespace

// ============================================================
// LuaReplProvider - Security-Hardened Implementation
// ============================================================

LuaReplProvider::LuaReplProvider()
    : m_pEditor(nullptr)
    , L(nullptr)
    , m_printOutput(std::make_unique<std::string>())
    , m_edCap()
{
}

LuaReplProvider::~LuaReplProvider()
{
    if (L)
    {
        lua_close(L);
    }
}

void LuaReplProvider::Initialize(ZepEditor* pEditor)
{
    m_pEditor = pEditor;

    // Create capability-based API (sandbox)
    m_edCap = CreateEditorCapability(pEditor);

    // Create Lua state with custom allocator for memory tracking
    L = lua_newstate(luaAlloc, this);
    luaL_openlibs(L);

    // Store provider pointer in extra space for hook access
    lua_setextraspace(L, this);

    // === SECURITY: Remove dangerous globals ===
    static const char* dangerous[] = {
        "io", // file I/O
        "os", // system commands
        "package", // module loading
        "require", // dynamic loading
        "dofile", // file execution
        "loadfile", // file loading
        "load", // arbitrary code generation
        "debug", // introspection/mutation
        "module", // legacy module system
        "jit", // LuaJIT extensions
        nullptr
    };
    for (int i = 0; dangerous[i]; i++)
    {
        lua_pushnil(L);
        lua_setglobal(L, dangerous[i]);
    }

    // === SECURITY: Override os.execute, io.* if any remain ===
    lua_pushnil(L);
    lua_setglobal(L, "os");
    lua_pushnil(L);
    lua_setglobal(L, "io");

    // === SECURITY: Set panic handler ===
    lua_atpanic(L, luaPanic);

    // === SECURITY: Install instruction count hook ===
    lua_sethook(L, luaHook, LUA_MASKLINE | LUA_MASKCOUNT, 100); // Check every 100 instructions

    // === SECURITY: Sandboxed print with output limit ===
    lua_pushlightuserdata(L, m_printOutput.get());
    lua_pushcclosure(L, l_print, 1);
    lua_setglobal(L, "print");

    // Register Capability metatypes
    registerBufferCapabilityMetatable(L);
    registerEditorCapabilityMetatable(L);

    // === EXPOSE CONTROL INTERFACE ===
    std::shared_ptr<EditorCapability>* ed_ud = (std::shared_ptr<EditorCapability>*)lua_newuserdata(L, sizeof(std::shared_ptr<EditorCapability>));
    new (ed_ud) std::shared_ptr<EditorCapability>(m_edCap);
    luaL_getmetatable(L, "ZepEditorCap");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "editor");

    ResetSecurityState();
}

std::string LuaReplProvider::ReplParse(const std::string& text)
{
    if (!L)
        return "<Lua REPL not initialized>";

    // SECURITY: Limit input size
    if (text.size() > Security::MAX_REPL_INPUT_SIZE)
    {
        return "Error: Input exceeds maximum size";
    }

    m_printOutput->clear();
    ResetSecurityState();

    // SECURITY: Load with protected mode
    int status = luaL_loadstring(L, text.c_str());
    if (status != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        std::string msg = "Error: ";
        if (err)
            msg += err;
        lua_pop(L, 1);
        return msg;
    }

    // SECURITY: Execute with protected call
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        std::string msg = "Runtime error: ";
        if (err)
        {
            msg += err;
            // Log security-relevant errors
            if (m_pEditor)
            {
                ZLOG(WARN, "Lua runtime error: " << err);
            }
        }
        lua_pop(L, 1);
        return msg;
    }

    // Collect results (limit number)
    int n = lua_gettop(L);
    if (n > 10)
    {
        // Too many results, truncate
        n = 10;
    }
    std::string result;
    for (int i = 1; i <= n; i++)
    {
        size_t len;
        const char* s = lua_tolstring(L, i, &len);
        if (s)
        {
            // SECURITY: Limit individual result size
            if (result.size() + len < Security::LUA_MAX_OUTPUT_SIZE)
                result.append(s, len);
        }
        else
        {
            lua_getglobal(L, "tostring");
            lua_pushvalue(L, i);
            lua_call(L, 1, 1);
            const char* s2 = lua_tostring(L, -1);
            if (s2 && result.size() < Security::LUA_MAX_OUTPUT_SIZE)
                result += s2;
            lua_pop(L, 1);
        }
        if (i < n)
            result += "\t";
    }
    lua_pop(L, n);

    // Combine captured print output with result
    if (!m_printOutput->empty())
    {
        if (!result.empty())
            return *m_printOutput + "\n" + result;
        else
            return *m_printOutput;
    }
    return result;
}

std::string LuaReplProvider::ReplParse(ZepBuffer& text, const GlyphIterator& cursorOffset, ReplParseType type)
{
    ZEP_UNUSED(type);
    long line = text.GetBufferLine(cursorOffset);
    std::string lineText = text.GetLineText(line);
    return ReplParse(lineText);
}

bool LuaReplProvider::ReplIsFormComplete(const std::string& input, int& depth)
{
    depth = 0;
    int balance = 0;
    for (char c : input)
    {
        if (c == '(' || c == '[' || c == '{')
            balance++;
        else if (c == ')' || c == ']' || c == '}')
            balance--;
    }
    depth = balance;
    return balance == 0;
}

// Factory functions
extern "C" IZepReplProvider* CreateLuaReplProvider()
{
    return new LuaReplProvider();
}

extern "C" void DestroyLuaReplProvider(IZepReplProvider* pProvider)
{
    delete pProvider;
}

// Registration
void RegisterLuaReplProvider(ZepEditor& editor)
{
    auto provider = std::make_unique<LuaReplProvider>();
    provider->Initialize(&editor);
    ZepReplExCommand::Register(editor, provider.get());
    RegisterLuaReplCommand(editor, provider.get());
    editor.RegisterReplProvider(std::move(provider));
}

} // namespace Zep
