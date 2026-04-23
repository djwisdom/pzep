#pragma once

#include <memory>
#include <string>
#include <utility> // for std::pair
#include <vector>

#include "buffer.h"
#include "editor.h"

namespace Zep
{

// ============================================================
// Capability Interfaces - Read-Only, Audited API Surface
// ============================================================

// BufferCapability: Read-only buffer access
// Allowed operations: inspection, no modification
struct BufferCapability
{
    virtual ~BufferCapability() = default;

    // Read operations
    virtual std::string GetName() const = 0;
    virtual long GetLength() const = 0;
    virtual long GetLineCount() const = 0;
    virtual std::string GetLineText(long line) const = 0;
    virtual std::pair<long, long> GetCursor() const = 0; // {line, column}
    virtual bool IsModified() const = 0;

    // DENIED: Insert, Replace, Save, Delete, Undo/Redo
};

// EditorCapability: Read-only editor access
struct EditorCapability
{
    virtual ~EditorCapability() = default;

    // Read operations
    virtual std::vector<std::shared_ptr<BufferCapability>> GetBuffers() const = 0;
    virtual std::shared_ptr<BufferCapability> GetActiveBuffer() const = 0;
    virtual std::string GetEditorVersion() const = 0;

    // DENIED: CreateBuffer, CloseBuffer, SetMode, AddWindow, etc.
};

// Audit logger for capability usage (optional but recommended)
struct CapabilityAuditLogger
{
    virtual ~CapabilityAuditLogger() = default;
    virtual void Log(const std::string& capability, const std::string& method, const std::vector<std::string>& args) = 0;
};

// ============================================================
// Capability Implementation - Wraps real Zep objects
// ============================================================

class BufferCapabilityImpl : public BufferCapability
{
public:
    explicit BufferCapabilityImpl(ZepBuffer* pBuffer)
        : m_pBuffer(pBuffer)
    {
    }

    std::string GetName() const override
    {
        return m_pBuffer->GetName();
    }
    long GetLength() const override
    {
        return m_pBuffer->GetLength();
    }
    long GetLineCount() const override
    {
        return m_pBuffer->GetLineCount();
    }
    std::string GetLineText(long line) const override
    {
        return m_pBuffer->GetLineText(line);
    }
    std::pair<long, long> GetCursor() const override
    {
        // Cannot directly get cursor from buffer; need editor context
        // Return {0,0} as fallback (this is a known limitation)
        return { 0, 0 };
    }
    bool IsModified() const override
    {
        return m_pBuffer->IsModified();
    }

private:
    ZepBuffer* m_pBuffer; // non-owning raw pointer (ZepEditor owns buffer)
};

class EditorCapabilityImpl : public EditorCapability
{
public:
    explicit EditorCapabilityImpl(ZepEditor* pEditor, std::shared_ptr<CapabilityAuditLogger> pLogger = nullptr)
        : m_pEditor(pEditor)
        , m_pLogger(std::move(pLogger))
    {
    }

    std::vector<std::shared_ptr<BufferCapability>> GetBuffers() const override
    {
        Log("EditorCapability", "GetBuffers", {});
        std::vector<std::shared_ptr<BufferCapability>> result;
        const auto& buffers = m_pEditor->GetBuffers();
        for (const auto& buf : buffers)
        {
            result.push_back(std::make_shared<BufferCapabilityImpl>(buf.get()));
        }
        return result;
    }

    std::shared_ptr<BufferCapability> GetActiveBuffer() const override
    {
        Log("EditorCapability", "GetActiveBuffer", {});
        ZepBuffer* pBuf = m_pEditor->GetActiveBuffer();
        if (pBuf)
        {
            return std::make_shared<BufferCapabilityImpl>(pBuf);
        }
        return nullptr;
    }

    std::string GetEditorVersion() const override
    {
        Log("EditorCapability", "GetEditorVersion", {});
        return "pZep 0.5.13 (sandboxed)";
    }

private:
    void Log(const std::string& capability, const std::string& method, const std::vector<std::string>& args) const
    {
        if (m_pLogger)
        {
            m_pLogger->Log(capability, method, args);
        }
    }

    ZepEditor* m_pEditor; // non-owning
    std::shared_ptr<CapabilityAuditLogger> m_pLogger;
};

// Factory function: create capability objects from real editor
inline std::shared_ptr<EditorCapability> CreateEditorCapability(ZepEditor* pEditor, std::shared_ptr<CapabilityAuditLogger> pLogger = nullptr)
{
    return std::make_shared<EditorCapabilityImpl>(pEditor, std::move(pLogger));
}

} // namespace Zep
