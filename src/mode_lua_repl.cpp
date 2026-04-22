#include "zep/buffer.h"
#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mcommon/logger.h"
#include "zep/mode_repl.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#include <lua.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Zep
{

namespace
{
// Helper to convert ZepBuffer* to Lua userdata
void pushBuffer(lua_State* L, ZepBuffer* buf)
{
    ZepBuffer** ud = (ZepBuffer**)lua_newuserdata(L, sizeof(ZepBuffer*));
    *ud = buf;
    luaL_getmetatable(L, "ZepBuffer");
    lua_setmetatable(L, -2);
}

ZepBuffer* checkBuffer(lua_State* L, int idx)
{
    return *(ZepBuffer**)lua_touserdata(L, idx);
}

ZepEditor* checkEditor(lua_State* L, int idx)
{
    return *(ZepEditor**)lua_touserdata(L, idx);
}

// Buffer method: GetName()
static int buf_GetName(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    const std::string& name = buf->GetName();
    lua_pushlstring(L, name.c_str(), name.size());
    return 1;
}

// Buffer method: GetLength() - number of bytes
static int buf_GetLength(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    lua_pushinteger(L, (lua_Integer)buf->GetLength());
    return 1;
}

// Buffer method: GetLineCount()
static int buf_GetLineCount(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    lua_pushinteger(L, (lua_Integer)buf->GetLineCount());
    return 1;
}

// Buffer method: GetLineText(line)
static int buf_GetLineText(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    long line = (long)luaL_checkinteger(L, 2);
    std::string text = buf->GetLineText(line);
    lua_pushlstring(L, text.c_str(), text.size());
    return 1;
}

// Buffer method: GetCursor() -> {line=, column=}
static int buf_GetCursor(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    ZepEditor& ed = buf->GetEditor();
    ZepTabWindow* tab = ed.GetActiveTabWindow();
    if (!tab)
    {
        lua_pushnil(L);
        return 1;
    }
    ZepWindow* win = tab->GetActiveWindow();
    if (!win || &win->GetBuffer() != buf)
    {
        lua_pushnil(L);
        return 1;
    }
    GlyphIterator it = win->GetBufferCursor();
    long line = buf->GetBufferLine(it);
    long col = buf->GetBufferColumn(it);
    lua_newtable(L);
    lua_pushinteger(L, (lua_Integer)line);
    lua_setfield(L, -2, "line");
    lua_pushinteger(L, (lua_Integer)col);
    lua_setfield(L, -2, "column");
    return 1;
}

// Buffer method: Insert(line, column, text)
static int buf_Insert(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    int line = (int)luaL_checkinteger(L, 2);
    int col = (int)luaL_checkinteger(L, 3);
    const char* text = luaL_checkstring(L, 4);

    ByteRange range;
    if (!buf->GetLineOffsets(line, range))
    {
        return luaL_error(L, "Invalid line %d", line);
    }

    GlyphIterator it(buf, range.first);
    // Advance 'col' characters
    for (int i = 0; i < col; ++i)
    {
        if (it.Index() >= range.second)
        {
            it = GlyphIterator(buf, range.second);
            break;
        }
        ++it;
    }

    ChangeRecord cr;
    bool ok = buf->Insert(it, std::string(text), cr);
    lua_pushboolean(L, ok);
    return 1;
}

// Buffer method: ReplaceLine(line, text)
static int buf_ReplaceLine(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    long line = (long)luaL_checkinteger(L, 2);
    const char* text = luaL_checkstring(L, 3);

    ByteRange range;
    if (!buf->GetLineOffsets(line, range))
    {
        return luaL_error(L, "Invalid line %ld", line);
    }

    GlyphIterator start(buf, range.first);
    GlyphIterator end(buf, range.second);
    ChangeRecord cr;
    bool ok = buf->Replace(start, end, std::string(text), ReplaceRangeMode::Replace, cr);
    lua_pushboolean(L, ok);
    return 1;
}

// Buffer method: Save()
static int buf_Save(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    int64_t size = 0;
    bool ok = buf->Save(size);
    lua_pushboolean(L, ok);
    return 1;
}

// Buffer method: IsModified()
static int buf_IsModified(lua_State* L)
{
    ZepBuffer* buf = checkBuffer(L, 1);
    lua_pushboolean(L, buf->IsModified());
    return 1;
}

// Editor method: GetActiveBuffer()
static int ed_GetActiveBuffer(lua_State* L)
{
    ZepEditor* ed = checkEditor(L, 1);
    ZepBuffer* buf = ed->GetActiveBuffer();
    if (buf)
    {
        pushBuffer(L, buf);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

// Editor method: GetBuffers() -> array of buffers
static int ed_GetBuffers(lua_State* L)
{
    ZepEditor* ed = checkEditor(L, 1);
    const auto& buffers = ed->GetBuffers();
    lua_newtable(L);
    int idx = 1;
    for (const auto& buf : buffers)
    {
        pushBuffer(L, buf.get());
        lua_rawseti(L, -2, idx++);
    }
    return 1;
}

// Lua print function that captures output
static int l_print(lua_State* L)
{
    // Upvalue 1: pointer to std::string
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

// Register Buffer metatable
void registerBufferMetatable(lua_State* L)
{
    luaL_newmetatable(L, "ZepBuffer");
    lua_newtable(L);
    lua_pushcfunction(L, buf_GetName);
    lua_setfield(L, -2, "GetName");
    lua_pushcfunction(L, buf_GetLength);
    lua_setfield(L, -2, "GetLength");
    lua_pushcfunction(L, buf_GetLineCount);
    lua_setfield(L, -2, "GetLineCount");
    lua_pushcfunction(L, buf_GetLineText);
    lua_setfield(L, -2, "GetLineText");
    lua_pushcfunction(L, buf_GetCursor);
    lua_setfield(L, -2, "GetCursor");
    lua_pushcfunction(L, buf_Insert);
    lua_setfield(L, -2, "Insert");
    lua_pushcfunction(L, buf_ReplaceLine);
    lua_setfield(L, -2, "ReplaceLine");
    lua_pushcfunction(L, buf_Save);
    lua_setfield(L, -2, "Save");
    lua_pushcfunction(L, buf_IsModified);
    lua_setfield(L, -2, "IsModified");
    // __index points to this table
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

// Register Editor metatable
void registerEditorMetatable(lua_State* L)
{
    luaL_newmetatable(L, "ZepEditor");
    lua_newtable(L);
    lua_pushcfunction(L, ed_GetActiveBuffer);
    lua_setfield(L, -2, "GetActiveBuffer");
    lua_pushcfunction(L, ed_GetBuffers);
    lua_setfield(L, -2, "GetBuffers");
    lua_setfield(L, -2, "__index");
    lua_pop(L, -1); // actually should be 1? careful: after second newtable, we set __index, then pop the method table, leaving metatable. Then pop metatable.
    // The typical sequence:
    // luaL_newmetatable(L, "ZepEditor");   // creates mt and leaves on stack
    // lua_newtable(L);                    // methods table
    // set methods...
    // lua_setfield(L, -2, "__index");     // mt.__index = methods
    // lua_pop(L, 1);                     // pop mt
    // So after lua_setfield we have mt on top, then we pop.
    // Already did that. We'll rewrite correctly below.
}

} // anonymous namespace

// LuaReplProvider implementation

LuaReplProvider::LuaReplProvider()
    : m_pEditor(nullptr)
    , L(nullptr)
    , m_printOutput(std::make_unique<std::string>())
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
    L = luaL_newstate();
    // Open standard libraries
    luaL_openlibs(L);
    // Remove dangerous globals
    static const char* dangerous[] = { "io", "os", "package", "require", "dofile", "loadfile", "load", "debug", "module", nullptr };
    for (int i = 0; dangerous[i]; i++)
    {
        lua_pushnil(L);
        lua_setglobal(L, dangerous[i]);
    }
    // Capture print output
    lua_pushlightuserdata(L, m_printOutput.get());
    lua_pushcclosure(L, l_print, 1);
    lua_setglobal(L, "print");

    // Register metatypes
    registerBufferMetatable(L);
    registerEditorMetatable(L);

    // Create editor userdata and set as global 'editor'
    ZepEditor** ed_ud = (ZepEditor**)lua_newuserdata(L, sizeof(ZepEditor*));
    *ed_ud = m_pEditor;
    luaL_getmetatable(L, "ZepEditor");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "editor");
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
            // Use tostring fallback
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
    lua_pop(L, n); // clean stack

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
    // Register REPL commands with the editor
    ZepReplExCommand::Register(editor, provider.get());
    ZepReplEvaluateCommand::Register(editor, provider.get());
    ZepReplEvaluateOuterCommand::Register(editor, provider.get());
    ZepReplEvaluateInnerCommand::Register(editor, provider.get());
    // Transfer ownership to editor
    editor.RegisterReplProvider(std::move(provider));
}

} // namespace Zep
