#include "zep/buffer.h"
#include "zep/commands_repl.h"
#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mcommon/logger.h"
#include "zep/mode_repl.h"
#include "zep/repl_capabilities.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#include <lua.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Zep
{

// Lua REPL Provider implementation
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
    ZepEditor* m_pEditor = nullptr;
    lua_State* L = nullptr;
    std::unique_ptr<std::string> m_printOutput;
    std::shared_ptr<EditorCapability> m_edCap;
};

namespace
{

// ============================================================
// Lua-side Capability Object Representation
// ============================================================

// Store shared_ptr<BufferCapability> as Lua userdata
void pushBufferCapability(lua_State* L, std::shared_ptr<BufferCapability> bufCap)
{
    // Create userdata containing shared_ptr
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

// Helper: convert std::vector<std::string> to Lua table (for audit)
void pushStringVector(lua_State* L, const std::vector<std::string>& vec)
{
    lua_newtable(L);
    for (size_t i = 0; i < vec.size(); ++i)
    {
        lua_pushlstring(L, vec[i].c_str(), vec[i].size());
        lua_rawseti(L, -2, i + 1);
    }
}

// ============================================================
// BufferCapability Lua Methods
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
    std::string text = bufCap->GetLineText(line);
    lua_pushlstring(L, text.c_str(), text.size());
    return 1;
}

static int bufcap_GetCursor(lua_State* L)
{
    ZEP_UNUSED(L);
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

// Note: No Insert/Replace/Save methods - those are denied in capability

// ============================================================
// EditorCapability Lua Methods
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
// Garbage Collection for userdata (shared_ptr destruction)
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

    // __gc for shared_ptr cleanup
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

    // __gc for shared_ptr cleanup
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
// LuaReplProvider - Security-Sandboxed Implementation
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

    // Create Lua state
    L = luaL_newstate();
    luaL_openlibs(L);

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
        nullptr
    };
    for (int i = 0; dangerous[i]; i++)
    {
        lua_pushnil(L);
        lua_setglobal(L, dangerous[i]);
    }

    // === SECURITY: Sandboxed print ===
    lua_pushlightuserdata(L, m_printOutput.get());
    lua_pushcclosure(L, l_print, 1);
    lua_setglobal(L, "print");

    // Register Capability metatypes
    registerBufferCapabilityMetatable(L);
    registerEditorCapabilityMetatable(L);

    // === EXPOSE CONTROL INTERFACE ===
    // Push editor capability shared_ptr as userdata and set global 'editor'
    std::shared_ptr<EditorCapability>* ed_ud = (std::shared_ptr<EditorCapability>*)lua_newuserdata(L, sizeof(std::shared_ptr<EditorCapability>));
    new (ed_ud) std::shared_ptr<EditorCapability>(m_edCap);
    luaL_getmetatable(L, "ZepEditorCap");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "editor");

    // For backwards compatibility, also expose as 'editor' (now capability-wrapped)
    // Previously exposed raw ZepEditor*, now replaced with capability wrapper

    // === AUDIT LOGGING ===
    // Optionally expose audit interface (future extension)
    // lua_pushlightuserdata(L, this);  // for callbacks to audit
}

std::string LuaReplProvider::ReplParse(const std::string& text)
{
    if (!L)
        return "<Lua REPL not initialized>";

    m_printOutput->clear();

    // Load the code
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

    // Execute
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        std::string msg = "Runtime error: ";
        if (err)
            msg += err;
        lua_pop(L, 1);
        return msg;
    }

    // Collect results
    int n = lua_gettop(L);
    std::string result;
    for (int i = 1; i <= n; i++)
    {
        size_t len;
        const char* s = lua_tolstring(L, i, &len);
        if (s)
        {
            result.append(s, len);
        }
        else
        {
            lua_getglobal(L, "tostring");
            lua_pushvalue(L, i);
            lua_call(L, 1, 1);
            const char* s2 = lua_tostring(L, -1);
            if (s2)
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
    // Simple approach: evaluate the current line at cursor
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
