#include "zep/fold.h"
#include "zep/buffer.h"

namespace Zep
{

ZepFold::ZepFold(ZepBuffer& buffer)
    : m_buffer(buffer)
{
}

ZepFold::~ZepFold()
{
}

void ZepFold::Clear()
{
    m_folds.clear();
}

void ZepFold::AddFold(long startLine, long endLine, FoldMethod method)
{
    if (startLine >= endLine)
        return;

    auto pFold = std::make_unique<FoldRegion>();
    pFold->startLine = startLine;
    pFold->endLine = endLine;
    pFold->method = method;
    pFold->isOpen = false;

    m_folds.push_back(std::move(pFold));
}

void ZepFold::RemoveFold(long line)
{
    auto itr = std::find_if(m_folds.begin(), m_folds.end(), [line](const std::unique_ptr<FoldRegion>& pFold) {
        return pFold->Contains(line);
    });

    if (itr != m_folds.end())
    {
        m_folds.erase(itr);
    }
}

void ZepFold::RemoveAllFolds()
{
    m_folds.clear();
}

bool ZepFold::ToggleFold(long line)
{
    auto* pFold = GetFold(line);
    if (pFold)
    {
        pFold->isOpen = !pFold->isOpen;
        return pFold->isOpen;
    }
    return false;
}

void ZepFold::OpenFold(long line)
{
    auto* pFold = GetFold(line);
    if (pFold)
    {
        pFold->isOpen = true;
    }
}

void ZepFold::CloseFold(long line)
{
    auto* pFold = GetFold(line);
    if (pFold)
    {
        pFold->isOpen = false;
    }
}

void ZepFold::OpenAllFolds()
{
    for (auto& pFold : m_folds)
    {
        pFold->isOpen = true;
    }
}

void ZepFold::CloseAllFolds()
{
    for (auto& pFold : m_folds)
    {
        pFold->isOpen = false;
    }
}

bool ZepFold::IsFolded(long line) const
{
    auto* pFold = const_cast<ZepFold*>(this)->GetFold(line);
    return pFold && !pFold->isOpen;
}

bool ZepFold::IsFoldOpen(long line) const
{
    auto* pFold = const_cast<ZepFold*>(this)->GetFold(line);
    return !pFold || pFold->isOpen;
}

FoldRegion* ZepFold::GetFold(long line)
{
    long idx = FindFoldIndex(line);
    if (idx >= 0 && idx < (long)m_folds.size())
    {
        return m_folds[idx].get();
    }
    return nullptr;
}

const FoldRegion* ZepFold::GetFold(long line) const
{
    long idx = FindFoldIndex(line);
    if (idx >= 0 && idx < (long)m_folds.size())
    {
        return m_folds[idx].get();
    }
    return nullptr;
}

bool ZepFold::CanOpen(long line) const
{
    return FindOuterFold(line) >= 0;
}

bool ZepFold::CanClose(long line) const
{
    return FindOuterFold(line) >= 0;
}

void ZepFold::CreateFoldFromSelection(long startLine, long endLine)
{
    AddFold(startLine, endLine, FoldMethod::Manual);
}

long ZepFold::FindFoldIndex(long line) const
{
    for (long i = (long)m_folds.size() - 1; i >= 0; i--)
    {
        if (m_folds[i]->Contains(line))
        {
            return i;
        }
    }
    return -1;
}

long ZepFold::FindOuterFold(long line) const
{
    long best = -1;
    for (long i = 0; i < (long)m_folds.size(); i++)
    {
        if (m_folds[i]->Contains(line) && m_folds[i]->startLine < m_folds[best]->startLine)
        {
            best = i;
        }
    }
    return best;
}

void ZepFold::GetVisibleLines(long bufferLine, long& outStart, long& outEnd) const
{
    outStart = bufferLine;
    outEnd = bufferLine;

    long visible = 0;
    for (auto& pFold : m_folds)
    {
        if (!pFold->isOpen)
        {
            visible += pFold->endLine - pFold->startLine;
        }
    }

    outEnd = bufferLine + visible;
}

long ZepFold::GetVisibleLineCount() const
{
    long lineCount = m_buffer.GetLineCount();
    long hidden = 0;

    for (auto& pFold : m_folds)
    {
        if (!pFold->isOpen)
        {
            hidden += pFold->endLine - pFold->startLine;
        }
    }

    return lineCount - hidden;
}

void ZepFold::RebuildFromIndentation()
{
    m_folds.clear();

    long lineCount = m_buffer.GetLineCount();
    if (lineCount <= 1)
        return;

    int currentIndent = 0;
    std::stack<std::pair<long, int>> indentStack;

    for (long line = 0; line < lineCount; line++)
    {
        ByteRange range;
        m_buffer.GetLineOffsets(line, range);

        std::string lineText;
        if (range.first < range.second)
        {
            lineText = m_buffer.GetBufferText(m_buffer.Begin() + range.first, m_buffer.Begin() + range.second);
        }

        int indent = 0;
        for (char c : lineText)
        {
            if (c == '\t')
                indent += 4;
            else if (c == ' ')
                indent++;
            else
                break;
        }

        if (indent > currentIndent && !indentStack.empty())
        {
            auto& top = indentStack.top();
            top.second = line - 1;
        }

        if (indent < currentIndent)
        {
            while (!indentStack.empty() && indentStack.top().second >= indent)
            {
                auto fold = indentStack.top();
                indentStack.pop();

                if (fold.second > fold.first)
                {
                    auto pFold = std::make_unique<FoldRegion>();
                    pFold->startLine = fold.first;
                    pFold->endLine = fold.second;
                    pFold->method = FoldMethod::Indent;
                    pFold->indentLevel = fold.second;
                    pFold->isOpen = true;
                    m_folds.push_back(std::move(pFold));
                }
            }
        }

        if (indent > currentIndent)
        {
            indentStack.push({ line, (int)lineCount });
        }

        currentIndent = indent;
    }

    while (!indentStack.empty())
    {
        auto fold = indentStack.top();
        indentStack.pop();

        if (fold.second > fold.first)
        {
            auto pFold = std::make_unique<FoldRegion>();
            pFold->startLine = fold.first;
            pFold->endLine = fold.second;
            pFold->method = FoldMethod::Indent;
            pFold->indentLevel = fold.second;
            pFold->isOpen = true;
            m_folds.push_back(std::move(pFold));
        }
    }
}

void ZepFold::RebuildFromSyntax()
{
    RebuildFromIndentation();
}

} // namespace Zep