#pragma once

#include <memory>
#include <vector>

namespace Zep
{

class ZepBuffer;

enum class FoldMethod
{
    Manual,
    Marker,
    Indent,
    Syntax
};

struct FoldRegion
{
    long startLine = 0;
    long endLine = 0;
    bool isOpen = true;
    FoldMethod method = FoldMethod::Manual;
    int indentLevel = 0;

    bool Contains(long line) const
    {
        return line >= startLine && line <= endLine;
    }

    bool IsNested(const FoldRegion& parent) const
    {
        return startLine > parent.startLine && endLine < parent.endLine;
    }
};

class ZepBuffer;

class ZepFold
{
public:
    ZepFold(ZepBuffer& buffer);
    ~ZepFold();

    void Clear();

    void AddFold(long startLine, long endLine, FoldMethod method = FoldMethod::Manual);
    void RemoveFold(long line);
    void RemoveAllFolds();

    bool ToggleFold(long line);
    void OpenFold(long line);
    void CloseFold(long line);
    void OpenAllFolds();
    void CloseAllFolds();

    bool IsFolded(long line) const;
    bool IsFoldOpen(long line) const;
    FoldRegion* GetFold(long line);
    const FoldRegion* GetFold(long line) const;

    bool CanOpen(long line) const;
    bool CanClose(long line) const;

    void CreateFoldFromSelection(long startLine, long endLine);

    void GetVisibleLines(long bufferLine, long& outStart, long& outEnd) const;

    long GetVisibleLineCount() const;

    void RebuildFromIndentation();
    void RebuildFromSyntax();

    const std::vector<std::unique_ptr<FoldRegion>>& GetFolds() const
    {
        return m_folds;
    }

private:
    long FindFoldIndex(long line) const;
    long FindOuterFold(long line) const;

    ZepBuffer& m_buffer;
    std::vector<std::unique_ptr<FoldRegion>> m_folds;
};

} // namespace Zep