#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mcommon/logger.h"
#include "zep/mode_repl.h"
#include "zep/repl_capabilities.h"
#include "zep/tab_window.h"
#include "zep/window.h"

// QuickJS single-file runtime
// If the library is installed via vcpkg or system, include <quickjs.h>
// If bundling, third_party/quickjs/quickjs.c should be compiled into the project
#ifdef ENABLE_QUICKJS_REPL
extern "C" {
#include <quickjs.h>
}
#endif

#include <memory>
#include <string>
#include <vector>

namespace Zep
{

namespace
{

// ============================================================
// QuickJS Capability Object Representation
// ============================================================

// BufferCapability userdata wrapper with finalizer
JSValue bufcap_gc_finalizer(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    JSValue* p = static_cast<JSValue*>(JS_GetOpaque(this_val, "BufferCapability"));
    if (p)
    {
        // shared_ptr destructor implicitly called when JSValue freed
    }
    return JS_UNDEFINED;
}

// Convert shared_ptr<BufferCapability> to JS object (with prototype)
JSValue pushBufferCapability(JSContext* ctx, std::shared_ptr<BufferCapability> bufCap)
{
    // Create JS object with a hidden class; store shared_ptr as opaque data
    JSValue obj = JS_NewObject(ctx);
    JS_SetOpaque(obj, new std::shared_ptr<BufferCapability>(std::move(bufCap)));

    // Set prototype to global "ZepBufferCap"
    JSValue proto = JS_GetProperty(ctx, JS_GetGlobalObject(ctx), "ZepBufferCap");
    if (!JS_IsException(proto))
    {
        JS_SetPrototype(ctx, obj, proto);
    }

    // Define methods directly on object using closure with shared_ptr capture? Easier to look up via prototype method table.

    return obj;
}

// Retrieve shared_ptr<BufferCapability> from JSValue
std::shared_ptr<BufferCapability> checkBufferCapability(JSContext* ctx, JSValueConst val)
{
    auto* p = static_cast<std::shared_ptr<BufferCapability>*>(JS_GetOpaque(val, "BufferCapability"));
    if (!p)
    {
        JS_ThrowTypeError(ctx, "Expected BufferCapability");
        return nullptr;
    }
    return *p;
}

// EditorCapability userdata
JSValue pushEditorCapability(JSContext* ctx, std::shared_ptr<EditorCapability> edCap)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetOpaque(obj, new std::shared_ptr<EditorCapability>(std::move(edCap)));
    JSValue proto = JS_GetProperty(ctx, JS_GetGlobalObject(ctx), "ZepEditorCap");
    if (!JS_IsException(proto))
    {
        JS_SetPrototype(ctx, obj, proto);
    }
    return obj;
}

std::shared_ptr<EditorCapability> checkEditorCapability(JSContext* ctx, JSValueConst val)
{
    auto* p = static_cast<std::shared_ptr<EditorCapability>*>(JS_GetOpaque(val, "ZepEditorCap"));
    if (!p)
    {
        JS_ThrowTypeError(ctx, "Expected EditorCapability");
        return nullptr;
    }
    return *p;
}

// ============================================================
// Finalizers (called when JS object is GC'd)
// ============================================================

JSValue bufcap_finalizer(JSContext* ctx, JSValueConst this_val)
{
    auto* p = static_cast<std::shared_ptr<BufferCapability>*>(JS_GetOpaque(this_val, "BufferCapability"));
    if (p)
    {
        delete p;
        JS_SetOpaque(this_val, nullptr);
    }
    return JS_UNDEFINED;
}

JSValue edcap_finalizer(JSContext* ctx, JSValueConst this_val)
{
    auto* p = static_cast<std::shared_ptr<EditorCapability>*>(JS_GetOpaque(this_val, "ZepEditorCap"));
    if (p)
    {
        delete p;
        JS_SetOpaque(this_val, nullptr);
    }
    return JS_UNDEFINED;
}

// ============================================================
// BufferCapability Methods (exposed to JS)
// ============================================================

JSValue bufcap_GetName(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto bufCap = checkBufferCapability(ctx, this_val);
    if (!bufCap)
        return JS_EXCEPTION;
    return JS_NewString(ctx, bufCap->GetName().c_str());
}

JSValue bufcap_GetLength(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto bufCap = checkBufferCapability(ctx, this_val);
    if (!bufCap)
        return JS_EXCEPTION;
    return JS_NewInt64(ctx, bufCap->GetLength());
}

JSValue bufcap_GetLineCount(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto bufCap = checkBufferCapability(ctx, this_val);
    if (!bufCap)
        return JS_EXCEPTION;
    return JS_NewInt64(ctx, bufCap->GetLineCount());
}

JSValue bufcap_GetLineText(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto bufCap = checkBufferCapability(ctx, this_val);
    if (!bufCap)
        return JS_EXCEPTION;
    if (argc < 1 || !JS_IsNumber(argv[0]))
    {
        JS_ThrowTypeError(ctx, "GetLineText expects line number");
        return JS_EXCEPTION;
    }
    long line;
    JS_ToInt64(ctx, &line, argv[0]);
    std::string text = bufCap->GetLineText(line);
    return JS_NewString(ctx, text.c_str());
}

JSValue bufcap_GetCursor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    ZEP_UNUSED(argc);
    ZEP_UNUSED(argv);
    auto bufCap = checkBufferCapability(ctx, this_val);
    if (!bufCap)
        return JS_EXCEPTION;
    auto cursor = bufCap->GetCursor();
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "line", JS_NewInt64(ctx, cursor.first));
    JS_SetPropertyStr(ctx, obj, "column", JS_NewInt64(ctx, cursor.second));
    return obj;
}

JSValue bufcap_IsModified(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto bufCap = checkBufferCapability(ctx, this_val);
    if (!bufCap)
        return JS_EXCEPTION;
    return JS_NewBool(ctx, bufCap->IsModified());
}

// ============================================================
// EditorCapability Methods
// ============================================================

JSValue edcap_GetBuffers(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto edCap = checkEditorCapability(ctx, this_val);
    if (!edCap)
        return JS_EXCEPTION;
    auto buffers = edCap->GetBuffers();
    JSValue arr = JS_NewArray(ctx);
    for (size_t i = 0; i < buffers.size(); ++i)
    {
        JSValue bufObj = pushBufferCapability(ctx, buffers[i]);
        JS_SetPropertyUint32(ctx, arr, i, bufObj);
    }
    return arr;
}

JSValue edcap_GetActiveBuffer(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto edCap = checkEditorCapability(ctx, this_val);
    if (!edCap)
        return JS_EXCEPTION;
    auto bufCap = edCap->GetActiveBuffer();
    if (bufCap)
    {
        return pushBufferCapability(ctx, std::move(bufCap));
    }
    return JS_NULL;
}

JSValue edcap_GetVersion(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto edCap = checkEditorCapability(ctx, this_val);
    if (!edCap)
        return JS_EXCEPTION;
    return JS_NewString(ctx, edCap->GetEditorVersion().c_str());
}

// ============================================================
// Safe Print Capture
// ============================================================

static JSValue js_print(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    std::ostringstream oss;
    for (int i = 0; i < argc; i++)
    {
        if (i > 0)
            oss << '\t';
        JSValue str = JS_ToString(ctx, argv[i]);
        const char* s = JS_ToCString(ctx, str);
        if (s)
        {
            oss << s;
            JS_FreeCString(ctx, s);
        }
        JS_FreeValue(ctx, str);
    }
    std::string* output = *static_cast<std::string**>(JS_GetOpaque(ctx, JS_GetGlobalObject(ctx), "printCapture"));
    output->append(oss.str());
    return JS_UNDEFINED;
}

// ============================================================
// Prototype Registration
// ============================================================

void registerBufferCapabilityClass(JSContext* ctx)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto,
        (const JSCFunctionListEntry[]){
            JS_CFUNC_DEF("GetName", 0, bufcap_GetName),
            JS_CFUNC_DEF("GetLength", 0, bufcap_GetLength),
            JS_CFUNC_DEF("GetLineCount", 0, bufcap_GetLineCount),
            JS_CFUNC_DEF("GetLineText", 1, bufcap_GetLineText),
            JS_CFUNC_DEF("GetCursor", 0, bufcap_GetCursor),
            JS_CFUNC_DEF("IsModified", 0, bufcap_IsModified),
            JS_PROP_STRING_DEF("[class]", "BufferCapability", JS_PROP_C_W_E),
        },
        7);
    JS_SetClassProto(ctx, "ZepBufferCap", proto);
    JS_SetPropertyStr(ctx, global, "ZepBufferCap", proto);
}

void registerEditorCapabilityClass(JSContext* ctx)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto,
        (const JSCFunctionListEntry[]){
            JS_CFUNC_DEF("GetBuffers", 0, edcap_GetBuffers),
            JS_CFUNC_DEF("GetActiveBuffer", 0, edcap_GetActiveBuffer),
            JS_CFUNC_DEF("GetVersion", 0, edcap_GetVersion),
            JS_PROP_STRING_DEF("[class]", "EditorCapability", JS_PROP_C_W_E),
        },
        4);
    JS_SetClassProto(ctx, "ZepEditorCap", proto);
    JS_SetPropertyStr(ctx, global, "ZepEditorCap", proto);
}

} // anonymous namespace

// ============================================================
// QuickJSReplProvider - Security Sandboxed
// ============================================================

QuickJSReplProvider::QuickJSReplProvider()
    : m_pEditor(nullptr)
    , ctx(nullptr)
    , m_printOutput(std::make_unique<std::string>())
    , m_edCap()
{
}

QuickJSReplProvider::~QuickJSReplProvider()
{
    if (ctx)
    {
        JS_FreeRuntime(rt);
        JS_FreeContext(ctx);
    }
}

void QuickJSReplProvider::Initialize(ZepEditor* pEditor)
{
    m_pEditor = pEditor;
    m_edCap = CreateEditorCapability(pEditor);

    // Create QuickJS runtime and context
    rt = JS_NewRuntime();
    if (!rt)
        return;
    ctx = JS_NewContext(rt);
    if (!ctx)
        return;

    // === SECURITY: Sandboxed print ===
    // Store print capture pointer in global object
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetOpaque(global, new std::string*(&m_printOutput)); // store pointer to unique_ptr's raw ptr
    JS_SetPropertyFunctionList(ctx, global,
        (const JSCFunctionListEntry[]){
            JS_CFUNC_DEF("print", 0, js_print),
        },
        1);

    // === SECURITY: Remove dangerous globals ===
    JS_DeleteProperty(ctx, global, "eval");
    JS_DeleteProperty(ctx, global, "Function");
    JS_DeleteProperty(ctx, global, "require");
    JS_DeleteProperty(ctx, global, "module");
    JS_DeleteProperty(ctx, global, "process");
    JS_DeleteProperty(ctx, global, "console");
    JS_DeleteProperty(ctx, global, "import");
    JS_DeleteProperty(ctx, global, "Module");
    // Remove require again; ensure

    // === Register Capability Classes ===
    registerBufferCapabilityClass(ctx);
    registerEditorCapabilityClass(ctx);

    // === EXPOSE CONTROL INTERFACE ===
    JSValue editorVal = pushEditorCapability(ctx, m_edCap);
    JS_SetPropertyStr(ctx, global, "editor", editorVal);
}

std::string QuickJSReplProvider::ReplParse(const std::string& text)
{
    if (!ctx)
        return "<QuickJS REPL not initialized>";

    m_printOutput->clear();

    // Compile and evaluate
    JSValue result = JS_Eval(ctx, text.c_str(), text.size(), "<repl>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result))
    {
        JSValue exception = JS_GetException(ctx);
        JSValue str = JS_ToString(ctx, exception);
        const char* err = JS_ToCString(ctx, str);
        std::string msg = "Error: ";
        if (err)
            msg += err;
        JS_FreeCString(ctx, err);
        JS_FreeValue(ctx, exception);
        JS_FreeValue(ctx, str);
        JS_FreeValue(ctx, result);
        return msg;
    }

    // Convert result to string
    std::ostringstream oss;
    // QuickJS returns a single value; we may want to stringify
    JSValue strVal = JS_ToString(ctx, result);
    const char* s = JS_ToCString(ctx, strVal);
    if (s)
    {
        oss << s;
        JS_FreeCString(ctx, s);
    }
    JS_FreeValue(ctx, strVal);
    JS_FreeValue(ctx, result);

    std::string resultStr = oss.str();
    if (!m_printOutput->empty())
    {
        if (!resultStr.empty())
            return *m_printOutput + "\n" + resultStr;
        else
            return *m_printOutput;
    }
    return resultStr;
}

std::string QuickJSReplProvider::ReplParse(ZepBuffer& text, const GlyphIterator& cursorOffset, ReplParseType type)
{
    ZEP_UNUSED(type);
    long line = text.GetBufferLine(cursorOffset);
    std::string lineText = text.GetLineText(line);
    return ReplParse(lineText);
}

bool QuickJSReplProvider::ReplIsFormComplete(const std::string& input, int& depth)
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
extern "C" IZepReplProvider* CreateQuickJSEvalReplProvider()
{
    return new QuickJSReplProvider();
}

extern "C" void DestroyQuickJSEvalReplProvider(IZepReplProvider* pProvider)
{
    delete pProvider;
}

// Registration
void RegisterQuickJSEvalReplProvider(ZepEditor& editor)
{
    auto provider = std::make_unique<QuickJSReplProvider>();
    provider->Initialize(&editor);
    ZepReplExCommand::Register(editor, provider.get());
    ZepReplEvaluateCommand::Register(editor, provider.get());
    ZepReplEvaluateOuterCommand::Register(editor, provider.get());
    ZepReplEvaluateInnerCommand::Register(editor, provider.get());
    editor.RegisterReplProvider(std::move(provider));
}

} // namespace Zep
