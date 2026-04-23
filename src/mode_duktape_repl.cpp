#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mcommon/logger.h"
#include "zep/mode_repl.h"
#include "zep/repl_capabilities.h"
#include "zep/tab_window.h"
#include "zep/window.h"

#ifdef ZEP_ENABLE_DUKTAPE_REPL
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

class DuktapeReplProvider : public IZepReplProvider
{
public:
    DuktapeReplProvider();
    ~DuktapeReplProvider();
    void Initialize(ZepEditor* pEditor);
    std::string ReplParse(ZepBuffer& text, const GlyphIterator& cursorOffset, ReplParseType type) override;
    std::string ReplParse(const std::string& text) override;
    bool ReplIsFormComplete(const std::string& input, int& depth) override;

private:
    ZepEditor* m_pEditor = nullptr;
    duk_context* ctx = nullptr;
    std::unique_ptr<std::string> m_printOutput;
    std::shared_ptr<EditorCapability> m_edCap;
};

// Stub implementations

DuktapeReplProvider::DuktapeReplProvider()
    : m_pEditor(nullptr)
    , ctx(nullptr)
    , m_printOutput(std::make_unique<std::string>())
    , m_edCap()
{
}

DuktapeReplProvider::~DuktapeReplProvider() = default;

void DuktapeReplProvider::Initialize(ZepEditor* pEditor)
{
    m_pEditor = pEditor;
    // m_edCap = CreateEditorCapability(pEditor); // optional
}

std::string DuktapeReplProvider::ReplParse(const std::string& text)
{
    return text; // Echo the input
}

std::string DuktapeReplProvider::ReplParse(ZepBuffer&, const GlyphIterator&, ReplParseType)
{
    return ""; // Not implemented
}

bool DuktapeReplProvider::ReplIsFormComplete(const std::string&, int&)
{
    return true;
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
    editor.RegisterReplProvider(std::move(provider));
}

} // namespace Zep
