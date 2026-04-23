#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mcommon/logger.h"
#include "zep/mode_repl.h"
#include "zep/repl_capabilities.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#ifdef ZEP_ENABLE_QUICKJS_REPL
extern "C" {
#include <quickjs.h>
}
#endif

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Zep
{

class QuickJSReplProvider : public IZepReplProvider
{
public:
    QuickJSReplProvider();
    ~QuickJSReplProvider();
    void Initialize(ZepEditor* pEditor);
    std::string ReplParse(ZepBuffer& text, const GlyphIterator& cursorOffset, ReplParseType type) override;
    std::string ReplParse(const std::string& text) override;
    bool ReplIsFormComplete(const std::string& input, int& depth) override;

private:
    ZepEditor* m_pEditor = nullptr;
    JSRuntime* rt = nullptr;
    JSContext* ctx = nullptr;
    std::unique_ptr<std::string> m_printOutput;
    std::shared_ptr<EditorCapability> m_edCap;
};

// Stub implementations

QuickJSReplProvider::QuickJSReplProvider()
    : m_pEditor(nullptr)
    , rt(nullptr)
    , ctx(nullptr)
    , m_printOutput(std::make_unique<std::string>())
    , m_edCap()
{
}

QuickJSReplProvider::~QuickJSReplProvider() = default;

void QuickJSReplProvider::Initialize(ZepEditor* pEditor)
{
    m_pEditor = pEditor;
    // rt = JS_NewRuntime();
    // ctx = JS_NewContext(rt);
}

std::string QuickJSReplProvider::ReplParse(const std::string& text)
{
    return text; // Echo
}

std::string QuickJSReplProvider::ReplParse(ZepBuffer&, const GlyphIterator&, ReplParseType)
{
    return ""; // Not implemented
}

bool QuickJSReplProvider::ReplIsFormComplete(const std::string&, int&)
{
    return true;
}

// Factory functions

extern "C" IZepReplProvider* CreateQuickJSReplProvider()
{
    return new QuickJSReplProvider();
}

extern "C" void DestroyQuickJSReplProvider(IZepReplProvider* pProvider)
{
    delete pProvider;
}

// Registration

void RegisterQuickJSEvalReplProvider(ZepEditor& editor)
{
    auto provider = std::make_unique<QuickJSReplProvider>();
    provider->Initialize(&editor);
    ZepReplExCommand::Register(editor, provider.get());
    editor.RegisterReplProvider(std::move(provider));
}

} // namespace Zep
