#include "zep/git.h"
#include "zep/commands.h"
#include "zep/editor.h"
#include "zep/splits.h"
#include "zep/tab_window.h"
#include "zep/theme.h"
#include "zep/window.h"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace Zep
{

ZepGit::ZepGit(ZepEditor& editor)
    : ZepComponent(editor)
{
}

ZepGit::~ZepGit()
{
}

bool ZepGit::IsGitRepo(const fs::path& path) const
{
    if (fs::exists(path / ".git"))
    {
        return true;
    }

    auto parent = path.parent_path();
    if (parent == path)
    {
        return false;
    }

    return IsGitRepo(parent);
}

fs::path ZepGit::GetGitRoot(const fs::path& path) const
{
    if (fs::exists(path / ".git"))
    {
        return path;
    }

    auto parent = path.parent_path();
    if (parent == path)
    {
        return fs::path();
    }

    return GetGitRoot(parent);
}

void ZepGit::Refresh(const fs::path& filePath)
{
    m_lineStatus.clear();
    m_validRepo = false;

    if (!filePath.has_filename())
    {
        return;
    }

    m_gitRoot = GetGitRoot(filePath);
    if (m_gitRoot.empty())
    {
        return;
    }

    m_validRepo = true;

    auto output = RunGitCommand({ "diff", "--unified=0", "--no-color", filePath.filename().string() }, m_gitRoot);
    ParseGitDiffOutput(output);

    auto statusOutput = RunGitCommand({ "status", "--porcelain", "-uall", "--", filePath.filename().string() }, m_gitRoot);
    ParseGitStatusOutput(statusOutput);
}

void ZepGit::RefreshBlame(const fs::path& filePath)
{
    m_blameInfo.clear();

    if (!filePath.has_filename())
    {
        return;
    }

    m_gitRoot = GetGitRoot(filePath);
    if (m_gitRoot.empty())
    {
        return;
    }

    auto output = RunGitCommand({ "blame", "--line-porcelain", "-w", "--", filePath.filename().string() }, m_gitRoot);
    ParseGitBlameOutput(output);
}

void ZepGit::ParseGitDiffOutput(const std::string& output)
{
    std::istringstream stream(output);
    std::string line;
    int currentLine = 0;

    while (std::getline(stream, line))
    {
        if (line.length() > 0 && line[0] == '@')
        {
            size_t pos = line.find("@@");
            if (pos != std::string::npos)
            {
                std::string info = line.substr(pos + 2);
                size_t minusPos = info.find("-");
                if (minusPos != std::string::npos)
                {
                    std::string before = info.substr(0, minusPos);
                    size_t commaPos = before.find(",");
                    if (commaPos != std::string::npos)
                    {
                        currentLine = std::stoi(before.substr(1, commaPos - 1)) - 1;
                    }
                    else
                    {
                        currentLine = std::stoi(before.substr(1)) - 1;
                    }
                }
            }
        }
        else if (line.length() > 0)
        {
            if (line[0] == '+' && line[1] != '+')
            {
                currentLine++;
                if (currentLine > 0)
                {
                    m_lineStatus[currentLine] = GitStatus::Added;
                }
            }
            else if (line[0] == '-' && line[1] != '-')
            {
                m_lineStatus[currentLine] = GitStatus::Deleted;
            }
            else if (line[0] == ' ' || (line.length() > 1 && (line[0] == '\\' && line[1] == '*')))
            {
                currentLine++;
            }
        }
    }
}

void ZepGit::ParseGitStatusOutput(const std::string& output)
{
    if (output.empty())
    {
        return;
    }

    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.length() < 2)
        {
            continue;
        }

        char statusChar = line[1];

        switch (statusChar)
        {
        case 'M':
        case 'm':
            for (auto& pair : m_lineStatus)
            {
                if (pair.second == GitStatus::Unknown)
                {
                    pair.second = GitStatus::Modified;
                }
            }
            break;
        case 'A':
            for (auto& pair : m_lineStatus)
            {
                if (pair.second == GitStatus::Unknown)
                {
                    pair.second = GitStatus::Added;
                }
            }
            break;
        case 'D':
            for (auto& pair : m_lineStatus)
            {
                if (pair.second == GitStatus::Unknown)
                {
                    pair.second = GitStatus::Deleted;
                }
            }
            break;
        case 'R':
            for (auto& pair : m_lineStatus)
            {
                if (pair.second == GitStatus::Unknown)
                {
                    pair.second = GitStatus::Renamed;
                }
            }
            break;
        case '?':
            for (auto& pair : m_lineStatus)
            {
                if (pair.second == GitStatus::Unknown)
                {
                    pair.second = GitStatus::Untracked;
                }
            }
            break;
        default:
            break;
        }
    }
}

void ZepGit::ParseGitBlameOutput(const std::string& output)
{
    if (output.empty())
    {
        return;
    }

    std::istringstream stream(output);
    std::string line;
    int currentLine = 0;
    GitBlameInfo info;
    bool inCommit = false;

    while (std::getline(stream, line))
    {
        if (line.substr(0, 7) == "author ")
        {
            info.author = line.substr(7);
        }
        else if (line.substr(0, 9) == "author-time ")
        {
            info.date = line.substr(11);
        }
        else if (line.substr(0, 7) == "summary ")
        {
            info.message = line.substr(7);
        }
        else if (line.substr(0, 7) == "commitsha ")
        {
            info.commitHash = line.substr(10);
            inCommit = true;
        }
        else if (line.substr(0, 6) == "\tfile ")
        {
            currentLine = std::stoi(line.substr(6));
            if (inCommit)
            {
                m_blameInfo[currentLine] = info;
            }
        }
        else if (line.substr(0, 9) == "\tboundary")
        {
            inCommit = false;
        }
        else if (line.length() > 0 && line[0] >= '0' && line[0] <= '9')
        {
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos)
            {
                currentLine = std::stoi(line.substr(0, spacePos));
            }
        }
        else if (line.empty())
        {
            inCommit = false;
        }
    }
}

GitStatus ZepGit::GetLineStatus(int line) const
{
    auto it = m_lineStatus.find(line);
    if (it != m_lineStatus.end())
    {
        return it->second;
    }
    return GitStatus::Unknown;
}

const GitBlameInfo* ZepGit::GetBlameInfo(int line) const
{
    auto it = m_blameInfo.find(line);
    if (it != m_blameInfo.end())
    {
        return &(it->second);
    }
    return nullptr;
}

std::string ZepGit::GetStatus()
{
    if (!m_validRepo)
    {
        auto pWindow = GetEditor().GetActiveWindow();
        if (!pWindow)
        {
            return "Not a git repository";
        }

        auto path = pWindow->GetBuffer().GetFilePath();
        m_gitRoot = GetGitRoot(path);

        if (m_gitRoot.empty())
        {
            return "Not a git repository";
        }

        m_validRepo = true;
    }

    return RunGitCommand({ "status", "--porcelain" }, m_gitRoot);
}

std::string ZepGit::GetDiff(const fs::path& filePath)
{
    auto root = GetGitRoot(filePath);
    if (root.empty())
    {
        return "";
    }

    return RunGitCommand({ "diff", "--no-color", "--", filePath.filename().string() }, root);
}

std::string ZepGit::GetBlame(const fs::path& filePath)
{
    auto root = GetGitRoot(filePath);
    if (root.empty())
    {
        return "";
    }

    return RunGitCommand({ "blame", "--no-color", "-w", "--", filePath.filename().string() }, root);
}

bool ZepGit::Commit(const std::string& message)
{
    if (!m_validRepo || message.empty())
    {
        return false;
    }

    auto output = RunGitCommand({ "commit", "-m", message }, m_gitRoot);
    return output.find("nothing to commit") == std::string::npos;
}

bool ZepGit::Push()
{
    if (!m_validRepo)
    {
        return false;
    }

    auto output = RunGitCommand({ "push" }, m_gitRoot);
    return output.find("error") == std::string::npos && output.find("fatal") == std::string::npos;
}

bool ZepGit::Pull()
{
    if (!m_validRepo)
    {
        return false;
    }

    auto output = RunGitCommand({ "pull" }, m_gitRoot);
    return output.find("error") == std::string::npos && output.find("fatal") == std::string::npos;
}

void ZepGit::UpdateBufferMarkers(ZepBuffer& buffer)
{
    auto path = buffer.GetFilePath();
    if (path.empty())
    {
        return;
    }

    Refresh(path);

    for (const auto& pair : m_lineStatus)
    {
        int line = pair.first;
        GitStatus status = pair.second;

        if (status == GitStatus::Unknown)
        {
            continue;
        }

        ByteRange range;
        if (buffer.GetLineOffsets(line, range))
        {
            auto marker = std::make_shared<RangeMarker>(buffer);
            marker->SetRange(range);

            ThemeColor backColor = ThemeColor::Background;
            ThemeColor textColor = ThemeColor::Text;

            switch (status)
            {
            case GitStatus::Added:
                backColor = ThemeColor::GitAdded;
                textColor = ThemeColor::Text;
                break;
            case GitStatus::Modified:
                backColor = ThemeColor::GitModified;
                textColor = ThemeColor::Text;
                break;
            case GitStatus::Deleted:
                backColor = ThemeColor::GitDeleted;
                textColor = ThemeColor::Text;
                break;
            case GitStatus::Renamed:
                backColor = ThemeColor::GitRenamed;
                textColor = ThemeColor::Text;
                break;
            case GitStatus::Untracked:
                backColor = ThemeColor::GitUntracked;
                textColor = ThemeColor::Text;
                break;
            default:
                break;
            }

            marker->SetColors(backColor, textColor);
            marker->displayType = RangeMarkerDisplayType::Background;

            buffer.AddRangeMarker(marker);
        }
    }
}

std::string ZepGit::RunGitCommand(const std::vector<std::string>& args, const fs::path& workingDir)
{
    std::string cmd = "git";
    for (const auto& arg : args)
    {
        cmd += " " + arg;
    }

    cmd += " 2>&1";

    std::string result;
    char buffer[4096];

    FILE* pipe = _popen(cmd.c_str(), "r");

    if (!pipe)
    {
        return "";
    }

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }

    _pclose(pipe);

    return result;
}

} // namespace Zep

namespace Zep
{

// Git Status Command
void ZepExCommand_GitStatus::Run(const std::vector<std::string>& args)
{
    if (m_spGit)
    {
        GetEditor().SetCommandText(m_spGit->GetStatus());
    }
}

void ZepExCommand_GitDiff::Run(const std::vector<std::string>& args)
{
    auto pWindow = GetEditor().GetActiveWindow();
    if (!pWindow || !m_spGit)
    {
        return;
    }

    auto output = m_spGit->GetDiff(pWindow->GetBuffer().GetFilePath());
    GetEditor().SetCommandText(output);
}

void ZepExCommand_VGitDiff::Run(const std::vector<std::string>& args)
{
    if (!m_spGit)
        return;

    auto pWindow = GetEditor().GetActiveWindow();
    if (!pWindow)
    {
        return;
    }

    auto output = m_spGit->GetDiff(pWindow->GetBuffer().GetFilePath());

    auto pTab = GetEditor().GetActiveTabWindow();
    if (!pTab)
    {
        return;
    }

    auto pBuffer = GetEditor().GetFileBuffer(":Git Diff:", 0, true);
    if (pBuffer)
    {
        pBuffer->SetText(output);
        pTab->AddWindow(pBuffer);
    }
}

void ZepExCommand_GitBlame::Run(const std::vector<std::string>& args)
{
    if (!m_spGit)
        return;

    auto pWindow = GetEditor().GetActiveWindow();
    if (!pWindow)
    {
        return;
    }

    m_spGit->RefreshBlame(pWindow->GetBuffer().GetFilePath());

    std::string output;
    for (int i = 1; i <= pWindow->GetBuffer().GetLineCount(); i++)
    {
        auto info = m_spGit->GetBlameInfo(i);
        if (info)
        {
            output += std::to_string(i) + ": " + info->commitHash.substr(0, 7) + " " + info->author + " - " + info->message + "\n";
        }
        else
        {
            output += std::to_string(i) + ": (no blame info)\n";
        }
    }

    auto pTab = GetEditor().GetActiveTabWindow();
    if (!pTab)
    {
        return;
    }

    auto pBuffer = GetEditor().GetFileBuffer(":Git Blame:", 0, true);
    if (pBuffer)
    {
        pBuffer->SetText(output);
        pTab->AddWindow(pBuffer);
    }
}

void ZepExCommand_GitCommit::Run(const std::vector<std::string>& args)
{
    if (!m_spGit)
        return;

    if (args.size() < 2)
    {
        GetEditor().SetCommandText("Gcommit requires a commit message");
        return;
    }

    std::string message;
    for (size_t i = 1; i < args.size(); i++)
    {
        message += args[i];
        if (i < args.size() - 1)
        {
            message += " ";
        }
    }

    if (m_spGit->Commit(message))
    {
        GetEditor().SetCommandText("Commit successful");
    }
    else
    {
        GetEditor().SetCommandText("Commit failed or nothing to commit");
    }
}

void ZepExCommand_GitPush::Run(const std::vector<std::string>& args)
{
    if (!m_spGit)
        return;

    if (m_spGit->Push())
    {
        GetEditor().SetCommandText("Push successful");
    }
    else
    {
        GetEditor().SetCommandText("Push failed");
    }
}

void ZepExCommand_GitPull::Run(const std::vector<std::string>& args)
{
    if (!m_spGit)
        return;

    if (m_spGit->Pull())
    {
        GetEditor().SetCommandText("Pull successful");
    }
    else
    {
        GetEditor().SetCommandText("Pull failed");
    }
}

} // namespace Zep