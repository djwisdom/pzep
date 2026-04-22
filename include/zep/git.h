#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "zep/buffer.h"
#include "zep/editor.h"
#include "zep/range_markers.h"

namespace Zep
{

enum class GitStatus
{
    Unknown,
    Added,
    Modified,
    Deleted,
    Renamed,
    Untracked,
    Unmodified
};

struct GitLineInfo
{
    int line = 0;
    GitStatus status = GitStatus::Unknown;
    std::string commitHash;
    std::string author;
    std::string date;
    std::string message;
};

struct GitBlameInfo
{
    std::string commitHash;
    std::string author;
    std::string date;
    std::string message;
};

class ZepGit : public ZepComponent
{
public:
    ZepGit(ZepEditor& editor);
    ~ZepGit();

    void Refresh(const fs::path& filePath);
    void RefreshBlame(const fs::path& filePath);

    bool IsGitRepo(const fs::path& path) const;
    fs::path GetGitRoot(const fs::path& path) const;

    GitStatus GetLineStatus(int line) const;
    const GitBlameInfo* GetBlameInfo(int line) const;

    std::string GetStatus();
    std::string GetDiff(const fs::path& filePath);
    std::string GetBlame(const fs::path& filePath);

    bool Commit(const std::string& message);
    bool Push();
    bool Pull();

    void UpdateBufferMarkers(ZepBuffer& buffer);

private:
    void ParseGitStatusOutput(const std::string& output);
    void ParseGitDiffOutput(const std::string& output);
    void ParseGitBlameOutput(const std::string& output);
    std::string RunGitCommand(const std::vector<std::string>& args, const fs::path& workingDir = fs::path());

private:
    fs::path m_gitRoot;
    std::map<int, GitStatus> m_lineStatus;
    std::map<int, GitBlameInfo> m_blameInfo;
    bool m_validRepo = false;
};

class ZepExCommand_GitStatus : public ZepExCommand
{
public:
    ZepExCommand_GitStatus(ZepEditor& editor, std::shared_ptr<ZepGit> spGit)
        : ZepExCommand(editor)
        , m_spGit(spGit)
    {
    }

    const char* ExCommandName() const override
    {
        return "Gstatus";
    }
    void Run(const std::vector<std::string>& args) override;

private:
    std::shared_ptr<ZepGit> m_spGit;
};

class ZepExCommand_GitDiff : public ZepExCommand
{
public:
    ZepExCommand_GitDiff(ZepEditor& editor, std::shared_ptr<ZepGit> spGit)
        : ZepExCommand(editor)
        , m_spGit(spGit)
    {
    }

    const char* ExCommandName() const override
    {
        return "Gdiff";
    }
    void Run(const std::vector<std::string>& args) override;

private:
    std::shared_ptr<ZepGit> m_spGit;
};

class ZepExCommand_VGitDiff : public ZepExCommand
{
public:
    ZepExCommand_VGitDiff(ZepEditor& editor, std::shared_ptr<ZepGit> spGit)
        : ZepExCommand(editor)
        , m_spGit(spGit)
    {
    }

    const char* ExCommandName() const override
    {
        return "vdiff";
    }
    void Run(const std::vector<std::string>& args) override;

private:
    std::shared_ptr<ZepGit> m_spGit;
};

class ZepExCommand_GitBlame : public ZepExCommand
{
public:
    ZepExCommand_GitBlame(ZepEditor& editor, std::shared_ptr<ZepGit> spGit)
        : ZepExCommand(editor)
        , m_spGit(spGit)
    {
    }

    const char* ExCommandName() const override
    {
        return "Gblame";
    }
    void Run(const std::vector<std::string>& args) override;

private:
    std::shared_ptr<ZepGit> m_spGit;
};

class ZepExCommand_GitCommit : public ZepExCommand
{
public:
    ZepExCommand_GitCommit(ZepEditor& editor, std::shared_ptr<ZepGit> spGit)
        : ZepExCommand(editor)
        , m_spGit(spGit)
    {
    }

    const char* ExCommandName() const override
    {
        return "Gcommit";
    }
    void Run(const std::vector<std::string>& args) override;

private:
    std::shared_ptr<ZepGit> m_spGit;
};

class ZepExCommand_GitPush : public ZepExCommand
{
public:
    ZepExCommand_GitPush(ZepEditor& editor, std::shared_ptr<ZepGit> spGit)
        : ZepExCommand(editor)
        , m_spGit(spGit)
    {
    }

    const char* ExCommandName() const override
    {
        return "Gpush";
    }
    void Run(const std::vector<std::string>& args) override;

private:
    std::shared_ptr<ZepGit> m_spGit;
};

class ZepExCommand_GitPull : public ZepExCommand
{
public:
    ZepExCommand_GitPull(ZepEditor& editor, std::shared_ptr<ZepGit> spGit)
        : ZepExCommand(editor)
        , m_spGit(spGit)
    {
    }

    const char* ExCommandName() const override
    {
        return "Gpull";
    }
    void Run(const std::vector<std::string>& args) override;

private:
    std::shared_ptr<ZepGit> m_spGit;
};

} // namespace Zep