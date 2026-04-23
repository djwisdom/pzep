#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mcommon/logger.h"
#include "zep/mode_repl.h"
#include "zep/repl_capabilities.h"
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

// ============================================================
// Duktape-side Capability Object Representation
// ============================================================

// Store shared_ptr<BufferCapability> as Duktape heap-allocated object with finalizer
duk_ret_t bufcap_gc(duk_context* ctx)
{
    void* ud = duk_get_pointer(ctx, 0);
    if (ud)
    {
        auto* pShared = static_cast<std::shared_ptr<BufferCapability>*>(ud);
        pShared->~shared_ptr<BufferCapability>();
    }
    return 0;
}

// BufferCapability -> JS object
void pushBufferCapability(duk_context* ctx, std::shared_ptr<BufferCapability> bufCap)
{
    // Allocate userdata that holds shared_ptr
    auto* ud = static_cast<std::shared_ptr<BufferCapability>*>(duk_push_pointer(ctx, nullptr));
    new (ud) std::shared_ptr<BufferCapability>(std::move(bufCap));

    // Create and set metatable with methods and __gc
    duk_push_object(ctx); // proto

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<BufferCapability>*>(duk_get_pointer(c, 0));
        auto bufCap = *pShared;
        std::string name = bufCap->GetName();
        duk_push_lstring(c, name.c_str(), name.size());
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetName");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<BufferCapability>*>(duk_get_pointer(c, 0));
        auto bufCap = *pShared;
        duk_push_number(c, static_cast<duk_double_t>(bufCap->GetLength()));
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetLength");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<BufferCapability>*>(duk_get_pointer(c, 0));
        auto bufCap = *pShared;
        duk_push_number(c, static_cast<duk_double_t>(bufCap->GetLineCount()));
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetLineCount");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<BufferCapability>*>(duk_get_pointer(c, 0));
        auto bufCap = *pShared;
        long line = static_cast<long>(duk_get_number(c, 1));
        std::string text = bufCap->GetLineText(line);
        duk_push_lstring(c, text.c_str(), text.size());
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetLineText");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<BufferCapability>*>(duk_get_pointer(c, 0));
        auto bufCap = *pShared;
        auto cursor = bufCap->GetCursor();
        duk_push_object(c);
        duk_push_number(c, static_cast<duk_double_t>(cursor.first));
        duk_put_prop_string(c, -2, "line");
        duk_push_number(c, static_cast<duk_double_t>(cursor.second));
        duk_put_prop_string(c, -2, "column");
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetCursor");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<BufferCapability>*>(duk_get_pointer(c, 0));
        auto bufCap = *pShared;
        duk_push_boolean(c, bufCap->IsModified() ? 1 : 0);
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "IsModified");

    // __gc finalizer
    duk_push_c_function(ctx, bufcap_gc, 0);
    duk_put_prop_string(ctx, -2, "__gc");

    duk_set_prototype(ctx, -2); // userdata.__proto__ = proto
}

// Retrieve shared_ptr<BufferCapability> from Duktape userdata at index
std::shared_ptr<BufferCapability> checkBufferCapability(duk_context* ctx, duk_idx_t idx)
{
    void* ud = duk_get_pointer(ctx, idx);
    if (!ud)
    {
        duk_type_error(ctx, "Expected BufferCapability userdata");
        return nullptr;
    }
    auto* pShared = static_cast<std::shared_ptr<BufferCapability>*>(ud);
    return *pShared;
}

// Store shared_ptr<EditorCapability> as userdata
void pushEditorCapability(duk_context* ctx, std::shared_ptr<EditorCapability> edCap)
{
    auto* ud = static_cast<std::shared_ptr<EditorCapability>*>(duk_push_pointer(ctx, nullptr));
    new (ud) std::shared_ptr<EditorCapability>(std::move(edCap));

    duk_push_object(ctx); // proto

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<EditorCapability>*>(duk_get_pointer(c, 0));
        auto edCap = *pShared;
        auto buffers = edCap->GetBuffers();
        duk_push_array(c);
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            // Push BufferCapability object
            auto* buf_ud = static_cast<std::shared_ptr<BufferCapability>*>(duk_push_pointer(c, nullptr));
            new(buf_ud) std::shared_ptr<BufferCapability>(buffers[i]);
            duk_push_object(c);  // BufferCap proto (re-use same prototype)
            // Note: prototype assignment deferred to Python side or via global
            // For now, we push raw userdata with methods attached inline
            // Actually Duktape has prototype chain; set prototype to global ZepBufferCap if available
            duk_get_global_string(c, "ZepBufferCap");
            duk_set_prototype(c, -2);
            duk_put_prop_index(c, -3, i);
        }
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetBuffers");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<EditorCapability>*>(duk_get_pointer(c, 0));
        auto edCap = *pShared;
        auto bufCap = edCap->GetActiveBuffer();
        if (bufCap)
        {
            auto* buf_ud = static_cast<std::shared_ptr<BufferCapability>*>(duk_push_pointer(c, nullptr));
            new(buf_ud) std::shared_ptr<BufferCapability>(std::move(bufCap));
            duk_get_global_string(c, "ZepBufferCap");
            duk_set_prototype(c, -2);
        }
        else
        {
            duk_push_null(c);
        }
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetActiveBuffer");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto* pShared = static_cast<std::shared_ptr<EditorCapability>*>(duk_get_pointer(c, 0));
        auto edCap = *pShared;
        duk_push_lstring(c, edCap->GetEditorVersion().c_str(), edCap->GetEditorVersion().size());
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetVersion");

    // __gc
    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        void* ud = duk_get_pointer(c, 0);
        if (ud)
        {
            auto* pShared = static_cast<std::shared_ptr<EditorCapability>*>(ud);
            pShared->~shared_ptr<EditorCapability>();
        }
        return 0; }, 0);
    duk_put_prop_string(ctx, -2, "__gc");

    duk_set_prototype(ctx, -2);
}

std::shared_ptr<EditorCapability> checkEditorCapability(duk_context* ctx, duk_idx_t idx)
{
    void* ud = duk_get_pointer(ctx, idx);
    if (!ud)
    {
        duk_type_error(ctx, "Expected EditorCapability userdata");
        return nullptr;
    }
    auto* pShared = static_cast<std::shared_ptr<EditorCapability>*>(ud);
    return *pShared;
}

// ============================================================
// Safe Print Capture
// ============================================================

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
    std::string* output = *static_cast<std::string**>(duk_get_pointer(ctx, duk_upvalueindex(0)));
    output->append(oss.str());
    return 0;
}

// ============================================================
// Prototype Registration
// ============================================================

void registerBufferCapabilityPrototype(duk_context* ctx)
{
    duk_push_object(ctx); // ZepBufferCap prototype

    // Methods (same as Lua but as Duktape C functions)
    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto bufCap = checkBufferCapability(c, 0);
        duk_push_lstring(c, bufCap->GetName().c_str(), bufCap->GetName().size());
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetName");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto bufCap = checkBufferCapability(c, 0);
        duk_push_number(c, static_cast<duk_double_t>(bufCap->GetLength()));
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetLength");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto bufCap = checkBufferCapability(c, 0);
        duk_push_number(c, static_cast<duk_double_t>(bufCap->GetLineCount()));
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetLineCount");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto bufCap = checkBufferCapability(c, 0);
        long line = static_cast<long>(duk_get_number(c, 1));
        duk_push_lstring(c, bufCap->GetLineText(line).c_str(), bufCap->GetLineText(line).size());
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetLineText");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto bufCap = checkBufferCapability(c, 0);
        auto cursor = bufCap->GetCursor();
        duk_push_object(c);
        duk_push_number(c, static_cast<duk_double_t>(cursor.first));
        duk_put_prop_string(c, -2, "line");
        duk_push_number(c, static_cast<duk_double_t>(cursor.second));
        duk_put_prop_string(c, -2, "column");
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetCursor");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto bufCap = checkBufferCapability(c, 0);
        duk_push_boolean(c, bufCap->IsModified() ? 1 : 0);
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "IsModified");

    // Store prototype globally as "ZepBufferCap"
    duk_put_global_string(ctx, "ZepBufferCap");
}

void registerEditorCapabilityPrototype(duk_context* ctx)
{
    duk_push_object(ctx); // ZepEditorCap prototype

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto edCap = checkEditorCapability(c, 0);
        auto buffers = edCap->GetBuffers();
        duk_push_array(c);
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            // Allocate BufferCapability userdata
            auto* buf_ud = static_cast<std::shared_ptr<BufferCapability>*>(duk_push_pointer(c, nullptr));
            new(buf_ud) std::shared_ptr<BufferCapability>(buffers[i]);
            // Set prototype to ZepBufferCap
            duk_get_global_string(c, "ZepBufferCap");
            duk_set_prototype(c, -2);
            duk_put_prop_index(c, -3, i);
        }
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetBuffers");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto edCap = checkEditorCapability(c, 0);
        auto bufCap = edCap->GetActiveBuffer();
        if (bufCap)
        {
            auto* buf_ud = static_cast<std::shared_ptr<BufferCapability>*>(duk_push_pointer(c, nullptr));
            new(buf_ud) std::shared_ptr<BufferCapability>(std::move(bufCap));
            duk_get_global_string(c, "ZepBufferCap");
            duk_set_prototype(c, -2);
        }
        else
        {
            duk_push_null(c);
        }
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetActiveBuffer");

    duk_push_c_function(ctx, [](duk_context* c) -> duk_ret_t {
        auto edCap = checkEditorCapability(c, 0);
        duk_push_lstring(c, edCap->GetEditorVersion().c_str(), edCap->GetEditorVersion().size());
        return 1; }, 0);
    duk_put_prop_string(ctx, -2, "GetVersion");

    duk_put_global_string(ctx, "ZepEditorCap");
}

} // anonymous namespace

// ============================================================
// DuktapeReplProvider - Security-Sandboxed Implementation
// ============================================================

DuktapeReplProvider::DuktapeReplProvider()
    : m_pEditor(nullptr)
    , ctx(nullptr)
    , m_printOutput(std::make_unique<std::string>())
    , m_edCap()
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

    // Create capability-based API (sandbox)
    m_edCap = CreateEditorCapability(pEditor);

    ctx = duk_create_heap_default();
    if (!ctx)
        return;

    // === SECURITY: Sandboxed print ===
    duk_push_pointer(ctx, m_printOutput.get());
    duk_push_c_function(ctx, duk_print, 1);
    duk_put_global_string(ctx, "print");

    // === SECURITY: Remove dangerous globals ===
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
    duk_push_undefined(ctx);
    duk_put_global_string(ctx, "import");
    duk_push_undefined(ctx);
    duk_put_global_string(ctx, "Module");

    // === Register Capability prototypes ===
    registerBufferCapabilityPrototype(ctx);
    registerEditorCapabilityPrototype(ctx);

    // === EXPOSE CONTROL INTERFACE ===
    // Create shared_ptr<EditorCapability> userdata and expose as global 'editor'
    auto* ed_ud = static_cast<std::shared_ptr<EditorCapability>*>(duk_push_pointer(ctx, nullptr));
    new (ed_ud) std::shared_ptr<EditorCapability>(m_edCap);
    duk_get_global_string(ctx, "ZepEditorCap");
    duk_set_prototype(ctx, -2);
    duk_put_global_string(ctx, "editor");
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
    duk_pop(ctx);

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
