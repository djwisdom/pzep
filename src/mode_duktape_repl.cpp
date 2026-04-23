#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mcommon/logger.h"
#include "zep/mode_repl.h"
#include "zep/tab_window.h"
#include "zep/window.h"

// Duktape single-header include
// Ensure DUK_SINGLE_FILE is defined when building duktape.c
// If using vcpkg or system install, include from <duktape.h>
#ifdef ENABLE_DUKTAPE_REPL
extern "C" {
#include <duktape.h>
}
#endif

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Zep
{

namespace
{
// Helper to convert ZepBuffer* to void* for Duktape
void* bufferToUserdata(ZepBuffer* buf)
{
    return static_cast<void*>(buf);
}

ZepBuffer* userdataToBuffer(duk_context* ctx, duk_idx_t idx)
{
    return static_cast<ZepBuffer*>(duk_get_pointer(ctx, idx));
}

ZepEditor* editorToUserdata(duk_context* ctx, duk_idx_t idx)
{
    return static_cast<ZepEditor*>(duk_get_pointer(ctx, idx));
}

// === Buffer methods ===

// buf:GetName() -> string
static duk_ret_t buf_GetName(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    const std::string& name = buf->GetName();
    duk_push_lstring(ctx, name.c_str(), name.size());
    return 1;
}

// buf:GetLength() -> number
static duk_ret_t buf_GetLength(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    duk_push_number(ctx, static_cast<duk_double_t>(buf->GetLength()));
    return 1;
}

// buf:GetLineCount() -> number
static duk_ret_t buf_GetLineCount(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    duk_push_number(ctx, static_cast<duk_double_t>(buf->GetLineCount()));
    return 1;
}

// buf:GetLineText(line) -> string
static duk_ret_t buf_GetLineText(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    long line = static_cast<long>(duk_get_number(ctx, 1));
    std::string text = buf->GetLineText(line);
    duk_push_lstring(ctx, text.c_str(), text.size());
    return 1;
}

// buf:GetCursor() -> {line: number, column: number}
static duk_ret_t buf_GetCursor(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    ZepEditor& ed = buf->GetEditor();
    ZepTabWindow* tab = ed.GetActiveTabWindow();
    if (!tab)
    {
        duk_push_null(ctx);
        return 1;
    }
    ZepWindow* win = tab->GetActiveWindow();
    if (!win || &win->GetBuffer() != buf)
    {
        duk_push_null(ctx);
        return 1;
    }
    GlyphIterator it = win->GetBufferCursor();
    long line = buf->GetBufferLine(it);
    long col = buf->GetBufferColumn(it);
    duk_push_object(ctx);
    duk_push_number(ctx, static_cast<duk_double_t>(line));
    duk_put_prop_string(ctx, -2, "line");
    duk_push_number(ctx, static_cast<duk_double_t>(col));
    duk_put_prop_string(ctx, -2, "column");
    return 1;
}

// buf:Insert(line, column, text) -> boolean
static duk_ret_t buf_Insert(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    int line = static_cast<int>(duk_get_number(ctx, 1));
    int col = static_cast<int>(duk_get_number(ctx, 2));
    const char* text = duk_get_string(ctx, 3);

    ByteRange range;
    if (!buf->GetLineOffsets(line, range))
    {
        duk_push_boolean(ctx, 0);
        return 1;
    }

    GlyphIterator it(buf, range.first);
    int maxCol = static_cast<int>(range.second - range.first);
    col = (col > maxCol) ? maxCol : col;
    for (int i = 0; i < col && it.Index() < range.second; ++i)
    {
        ++it;
    }

    ChangeRecord cr;
    bool ok = buf->Insert(it, std::string(text), cr);
    duk_push_boolean(ctx, ok ? 1 : 0);
    return 1;
}

// buf:ReplaceLine(line, text) -> boolean
static duk_ret_t buf_ReplaceLine(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    long line = static_cast<long>(duk_get_number(ctx, 1));
    const char* text = duk_get_string(ctx, 2);

    ByteRange range;
    if (!buf->GetLineOffsets(line, range))
    {
        duk_push_boolean(ctx, 0);
        return 1;
    }

    GlyphIterator start(buf, range.first);
    GlyphIterator end(buf, range.second);
    ChangeRecord cr;
    bool ok = buf->Replace(start, end, std::string(text), ReplaceRangeMode::Replace, cr);
    duk_push_boolean(ctx, ok ? 1 : 0);
    return 1;
}

// buf:Save() -> boolean
static duk_ret_t buf_Save(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    int64_t size = 0;
    bool ok = buf->Save(size);
    duk_push_boolean(ctx, ok ? 1 : 0);
    return 1;
}

// buf:IsModified() -> boolean
static duk_ret_t buf_IsModified(duk_context* ctx)
{
    ZepBuffer* buf = userdataToBuffer(ctx, 0);
    duk_push_boolean(ctx, buf->IsModified() ? 1 : 0);
    return 1;
}

// === Editor methods ===

// editor:GetActiveBuffer() -> buffer|nil
static duk_ret_t ed_GetActiveBuffer(duk_context* ctx)
{
    ZepEditor* ed = editorToUserdata(ctx, 0);
    ZepBuffer* buf = ed->GetActiveBuffer();
    if (buf)
    {
        // Push buffer userdata
        ZepBuffer** ud = static_cast<ZepBuffer**>(duk_push_pointer(ctx, static_cast<void*>(buf)));
            duk_push_c_function(ctx, [ctx) -> duk_ret_t {
            // Return the buffer object itself
            return 0;
            }, 0);
            // Simplified: just push a wrapper
            duk_push_pointer(ctx, buf);
            duk_push_string(ctx, "ZepBuffer");
            duk_put_prop_string(ctx, -2, "\xff""__type");
    }
    else
    {
        duk_push_null(ctx);
    }
    return 1;
}

// editor:GetBuffers() -> array of buffers
static duk_ret_t ed_GetBuffers(duk_context* ctx)
{
    ZepEditor* ed = editorToUserdata(ctx, 0);
    const auto& buffers = ed->GetBuffers();
    duk_push_array(ctx);
    int idx = 0;
    for (const auto& buf : buffers)
    {
        duk_push_pointer(ctx, buf.get());
        // Mark as buffer type
        duk_push_string(ctx, "ZepBuffer");
        duk_put_prop_string(ctx, -2, "\xff"
                                     "__type");
        duk_put_prop(ctx, -3);
        idx++;
    }
    return 1;
}

// Safe print that captures output
static duk_ret_t duk_print(duk_context* ctx)
{
    std::ostringstream oss;
    int n = duk_get_top(ctx);
    for (int i = 0; i < n; i++)
    {
        if (i > 0)
            oss << '\t';
        if (duk_is_string(ctx, i))
        {
            oss << duk_get_string(ctx, i);
        }
        else if (duk_is_number(ctx, i))
        {
            oss << duk_get_number(ctx, i);
        }
        else if (duk_is_boolean(ctx, i))
        {
            oss << (duk_get_boolean(ctx, i) ? "true" : "false");
        }
        else
        {
            oss << "[object]";
        }
    }
    // Store in upvalue (string*)
    std::string* output = *static_cast<std::string**>(duk_get_pointer(ctx, duk_upvalueindex(0)));
    *output = oss.str();
    return 0;
}

// Register Buffer prototype
void registerBufferPrototype(duk_context* ctx)
{
    duk_push_object(ctx); // Buffer proto

    duk_push_c_function(ctx, buf_GetName, 0);
    duk_put_prop_string(ctx, -2, "GetName");

    duk_push_c_function(ctx, buf_GetLength, 0);
    duk_put_prop_string(ctx, -2, "GetLength");

    duk_push_c_function(ctx, buf_GetLineCount, 0);
    duk_put_prop_string(ctx, -2, "GetLineCount");

    duk_push_c_function(ctx, buf_GetLineText, 1);
    duk_put_prop_string(ctx, -2, "GetLineText");

    duk_push_c_function(ctx, buf_GetCursor, 0);
    duk_put_prop_string(ctx, -2, "GetCursor");

    duk_push_c_function(ctx, buf_Insert, 3);
    duk_put_prop_string(ctx, -2, "Insert");

    duk_push_c_function(ctx, buf_ReplaceLine, 2);
    duk_put_prop_string(ctx, -2, "ReplaceLine");

    duk_push_c_function(ctx, buf_Save, 0);
    duk_put_prop_string(ctx, -2, "Save");

    duk_push_c_function(ctx, buf_IsModified, 0);
    duk_put_prop_string(ctx, -2, "IsModified");

    // Store prototype globally
    duk_put_global_string(ctx, "ZepBuffer");
}

// Create a buffer userdata object
duk_push_buffer_userdata(duk_context* ctx, ZepBuffer* buf)
{
    // Create a simple pointer-wrapped object
    duk_push_pointer(ctx, buf);
    duk_push_string(ctx, "ZepBuffer");
    duk_put_prop_string(ctx, -2, "\xff"
                                 "__type");
    return 1;
}

// Register Editor prototype
void registerEditorPrototype(duk_context* ctx)
{
    duk_push_object(ctx); // Editor proto

    duk_push_c_function(ctx, ed_GetActiveBuffer, 0);
    duk_put_prop_string(ctx, -2, "GetActiveBuffer");

    duk_push_c_function(ctx, ed_GetBuffers, 0);
    duk_put_prop_string(ctx, -2, "GetBuffers");

    duk_put_global_string(ctx, "ZepEditor");
}

} // anonymous namespace

// DuktapeReplProvider implementation

DuktapeReplProvider::DuktapeReplProvider()
    : m_pEditor(nullptr)
    , ctx(nullptr)
    , m_printOutput(std::make_unique<std::string>())
{
}

DuktapeReplProvider::~DuktapeReplProvider()
{
    if (ctx)
    {
        duk_destroy_context(ctx);
    }
}

void DuktapeReplProvider::Initialize(ZepEditor* pEditor)
{
    m_pEditor = pEditor;
    ctx = duk_create_heap_default();
    if (!ctx)
        return;

    // Setup global print that captures output
    duk_push_pointer(ctx, m_printOutput.get());
    duk_push_c_function(ctx, duk_print, 1);
    duk_put_global_string(ctx, "print");

    // Remove dangerous globals
    duk_push_undefined(ctx);
    duk_put_global_string(ctx, "eval");
    duk_push_undefined(ctx);
    duk_put_global_string(ctx, "Function");
    duk_push_undefined(ctx);
    duk_put_global_string(ctx, "require");
    duk_push_undefined(ctx);
    duk_put_global_string(ctx, "module");
    duk_push_undefined(ctx);
    duk_put_global_string(ctx, "process");
    duk_push_undefined(ctx);
    duk_put_global_string(ctx, "console");

    // Register prototypes
    // Note: simplified - in practice need to create constructors for Buffer/Editor objects
    // For now, use raw pointers wrapped as objects with type tags

    // Create editor userdata
    duk_push_global_object(ctx);
    duk_push_pointer(ctx, m_pEditor);
    duk_put_prop_string(ctx, -2, "\xff"
                                 "editor_ptr");
    duk_pop(ctx);
}

std::string DuktapeReplProvider::ReplParse(const std::string& text)
{
    if (!ctx)
        return "<Duktape REPL not initialized>";

    m_printOutput->clear();

    duk_int_t rc = duk_peval_string(ctx, text.c_str());
    if (rc != DUK_EXEC_SUCCESS)
    {
        const char* err = duk_safe_to_string(ctx, -1);
        std::string msg = "Error: ";
        if (err)
            msg += err;
        duk_pop(ctx);
        return msg;
    }

    // Collect results (top of stack)
    std::ostringstream oss;
    int n = duk_get_top(ctx);
    for (int i = 0; i < n; i++)
    {
        if (i > 0)
            oss << '\t';
        if (duk_is_string(ctx, i))
        {
            oss << duk_get_string(ctx, i);
        }
        else if (duk_is_number(ctx, i))
        {
            oss << duk_get_number(ctx, i);
        }
        else if (duk_is_boolean(ctx, i))
        {
            oss << (duk_get_boolean(ctx, i) ? "true" : "false");
        }
        else
        {
            oss << "[object]";
        }
    }
    duk_pop(ctx); // clean stack

    std::string result = oss.str();
    if (!m_printOutput->empty())
    {
        if (!result.empty())
            return *m_printOutput + "\n" + result;
        else
            return *m_printOutput;
    }
    return result;
}

std::string DuktapeReplProvider::ReplParse(ZepBuffer& text, const GlyphIterator& cursorOffset, ReplParseType type)
{
    ZEP_UNUSED(type);
    long line = text.GetBufferLine(cursorOffset);
    std::string lineText = text.GetLineText(line);
    return ReplParse(lineText);
}

bool DuktapeReplProvider::ReplIsFormComplete(const std::string& input, int& depth)
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
extern "C" IZepReplProvider* CreateDuktapeReplProvider()
{
    return new DuktapeReplProvider();
}

extern "C" void DestroyDuktapeReplProvider(IZepReplProvider* pProvider)
{
    delete pProvider;
}

// Registration
void RegisterDuktapeReplProvider(ZepEditor& editor)
{
    auto provider = std::make_unique<DuktapeReplProvider>();
    provider->Initialize(&editor);
    ZepReplExCommand::Register(editor, provider.get());
    ZepReplEvaluateCommand::Register(editor, provider.get());
    ZepReplEvaluateOuterCommand::Register(editor, provider.get());
    ZepReplEvaluateInnerCommand::Register(editor, provider.get());
    editor.RegisterReplProvider(std::move(provider));
}

} // namespace Zep
