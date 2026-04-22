#pragma once

// Serious Developer Notifications for Zep
// Actionable, Minimal, Context-Rich

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ZepNotifications
{

// ============================================================================
// CORE TYPES
// ============================================================================

enum class NotificationSeverity
{
    Critical, // Must act immediately
    High, // Should act soon
    Medium, // Worth attention
    Low, // Informational
    Info // Noise/aggregated
};

enum class NotificationType
{
    BuildFailure,
    TestFailure,
    MergeCIStatus,
    CodeReviewRequest,
    RuntimeError,
    Deployment,
    SecurityAlert,
    PerformanceRegression,
    FlakyTest,
    DependencyVuln,
    LongRunningTask,
    WorkspaceEvent
};

struct Notification
{
    // Required: one-line summary (the "what")
    std::string summary;

    // Primary identifier for lookups
    std::string id;

    // Type classification
    NotificationType type;
    NotificationSeverity severity;

    // Timestamps
    std::chrono::system_clock::time_point timestamp;

    // Context fields (who/where/when)
    std::string project; // Which project/repo
    std::string branch; // Which branch
    std::string commit; // Which commit
    std::string author; // Who triggered

    // Context-rich details
    std::string target; // e.g., build target, test suite
    std::string first_error; // Short error message
    std::string error_location; // file:line or function
    std::string request_id; // For tracing

    // Link to full details
    std::string link;

    // Explicit action or next step
    std::string action;
    std::string action_link;

    // Payload for editor integration
    // If set, clicking opens this file at this line
    std::optional<std::string> file_path;
    std::optional<int> file_line;
};

// ============================================================================
// FORMATTING HELPERS
// ============================================================================

inline std::string SeverityToString(NotificationSeverity s)
{
    switch (s)
    {
    case NotificationSeverity::Critical:
        return "CRITICAL";
    case NotificationSeverity::High:
        return "HIGH";
    case NotificationSeverity::Medium:
        return "MEDIUM";
    case NotificationSeverity::Low:
        return "LOW";
    case NotificationSeverity::Info:
        return "INFO";
    }
    return "UNKNOWN";
}

inline std::string TypeToString(NotificationType t)
{
    switch (t)
    {
    case NotificationType::BuildFailure:
        return "BUILD";
    case NotificationType::TestFailure:
        return "TESTS";
    case NotificationType::MergeCIStatus:
        return "CI";
    case NotificationType::CodeReviewRequest:
        return "REVIEW";
    case NotificationType::RuntimeError:
        return "RUNTIME";
    case NotificationType::Deployment:
        return "DEPLOY";
    case NotificationType::SecurityAlert:
        return "SECURITY";
    case NotificationType::PerformanceRegression:
        return "PERF";
    case NotificationType::FlakyTest:
        return "FLAKY";
    case NotificationType::DependencyVuln:
        return "VULN";
    case NotificationType::LongRunningTask:
        return "TASK";
    case NotificationType::WorkspaceEvent:
        return "WORKSPACE";
    }
    return "UNKNOWN";
}

// Format: "Build failed — core-lib (target: all) — error: undefined reference at src/util.cpp:128 — Open log [ID#1234]"
inline std::string FormatBuildFailure(const Notification& n)
{
    std::string out = "Build failed — " + n.project;
    if (!n.target.empty())
        out += " (target: " + n.target + ")";
    if (!n.first_error.empty())
        out += " — error: " + n.first_error;
    if (!n.error_location.empty())
        out += " at " + n.error_location;
    if (!n.id.empty())
        out += " — Open log [ID#" + n.id + "]";
    return out;
}

// Format: "Tests broken — auth-suite — 3 fails — test_login: expected 200 got 500 — Re-run / View details"
inline std::string FormatTestFailure(const Notification& n)
{
    std::string out = "Tests broken — " + n.target;
    if (!n.first_error.empty())
        out += " — " + n.first_error;
    out += " — ";
    if (!n.action.empty())
        out += n.action;
    out += " / ";
    if (!n.link.empty())
        out += "View details";
    return out;
}

// Format: "Prod ERROR — auth-service — NullPointerException @ AuthController.login() — request=abc123 — Open trace"
inline std::string FormatRuntimeError(const Notification& n)
{
    std::string out = "Prod ERROR — " + n.project + " — " + n.first_error;
    if (!n.error_location.empty())
        out += " @ " + n.error_location;
    if (!n.request_id.empty())
        out += " — request=" + n.request_id;
    if (!n.action_link.empty())
        out += " — Open trace";
    return out;
}

// Format: "Deployment — staging — v2.1.0 — success — 2m — View Deploy"
inline std::string FormatDeployment(const Notification& n)
{
    std::string out = "Deployment — " + n.project + " — " + n.target;
    if (!n.first_error.empty())
        out += " — " + n.first_error;
    if (!n.action_link.empty())
        out += " — " + n.action;
    return out;
}

// Format: "CRITICAL: Security — repo/file — severity: HIGH — remediation: update package@>2.0.0"
inline std::string FormatSecurityAlert(const Notification& n)
{
    std::string out = "CRITICAL: Security — " + n.project;
    if (!n.target.empty())
        out += "/" + n.target;
    out += " — severity: " + SeverityToString(n.severity);
    if (!n.first_error.empty())
        out += " — " + n.first_error;
    if (!n.link.empty())
        out += " — View Report";
    return out;
}

// ============================================================================
// NOTIFICATION BUILDER
// ============================================================================

class NotificationBuilder
{
public:
    Notification n;

    NotificationBuilder(NotificationType type)
    {
        n.type = type;
        n.severity = NotificationSeverity::Medium;
        n.timestamp = std::chrono::system_clock::now();
    }

    NotificationBuilder& SetSummary(const std::string& s)
    {
        n.summary = s;
        return *this;
    }
    NotificationBuilder& SetID(const std::string& s)
    {
        n.id = s;
        return *this;
    }
    NotificationBuilder& SetProject(const std::string& s)
    {
        n.project = s;
        return *this;
    }
    NotificationBuilder& SetBranch(const std::string& s)
    {
        n.branch = s;
        return *this;
    }
    NotificationBuilder& SetCommit(const std::string& s)
    {
        n.commit = s;
        return *this;
    }
    NotificationBuilder& SetAuthor(const std::string& s)
    {
        n.author = s;
        return *this;
    }
    NotificationBuilder& SetTarget(const std::string& s)
    {
        n.target = s;
        return *this;
    }
    NotificationBuilder& SetError(const std::string& s)
    {
        n.first_error = s;
        return *this;
    }
    NotificationBuilder& SetErrorLocation(const std::string& s)
    {
        n.error_location = s;
        return *this;
    }
    NotificationBuilder& SetRequestID(const std::string& s)
    {
        n.request_id = s;
        return *this;
    }
    NotificationBuilder& SetLink(const std::string& s)
    {
        n.link = s;
        return *this;
    }
    NotificationBuilder& SetAction(const std::string& s)
    {
        n.action = s;
        return *this;
    }
    NotificationBuilder& SetActionLink(const std::string& s)
    {
        n.action_link = s;
        return *this;
    }
    NotificationBuilder& SetFile(const std::string& s, int line)
    {
        n.file_path = s;
        n.file_line = line;
        return *this;
    }

    NotificationBuilder& SetSeverity(NotificationSeverity s)
    {
        n.severity = s;
        return *this;
    }
    NotificationBuilder& AsCritical()
    {
        n.severity = NotificationSeverity::Critical;
        return *this;
    }
    NotificationBuilder& AsHigh()
    {
        n.severity = NotificationSeverity::High;
        return *this;
    }

    std::string Format() const
    {
        switch (n.type)
        {
        case NotificationType::BuildFailure:
            return FormatBuildFailure(n);
        case NotificationType::TestFailure:
            return FormatTestFailure(n);
        case NotificationType::RuntimeError:
            return FormatRuntimeError(n);
        case NotificationType::Deployment:
            return FormatDeployment(n);
        case NotificationType::SecurityAlert:
            return FormatSecurityAlert(n);
        default:
            return n.summary;
        }
    }

    Notification Build()
    {
        return n;
    }
};

// Shortcut builders
inline NotificationBuilder BuildFailed(const std::string& project,
    const std::string& target,
    const std::string& error,
    const std::string& location,
    const std::string& build_id,
    const std::string& log_link)
{
    return NotificationBuilder(NotificationType::BuildFailure)
        .SetSummary("Build failed: " + error)
        .SetProject(project)
        .SetTarget(target)
        .SetError(error)
        .SetErrorLocation(location)
        .SetID(build_id)
        .SetLink(log_link)
        .SetAction("Open log")
        .SetActionLink(log_link)
        .AsCritical();
}

inline NotificationBuilder TestFailed(const std::string& suite,
    const std::string& error_summary,
    const std::string& rerun_link)
{
    return NotificationBuilder(NotificationType::TestFailure)
        .SetTarget(suite)
        .SetError(error_summary)
        .SetAction("Re-run tests")
        .SetActionLink(rerun_link)
        .AsCritical();
}

inline NotificationBuilder RuntimeError(const std::string& service,
    const std::string& exception,
    const std::string& location,
    const std::string& req_id,
    const std::string& trace_link)
{
    return NotificationBuilder(NotificationType::RuntimeError)
        .SetProject(service)
        .SetError(exception)
        .SetErrorLocation(location)
        .SetRequestID(req_id)
        .SetActionLink(trace_link)
        .SetAction("Open trace")
        .AsCritical();
}

inline NotificationBuilder DeployComplete(const std::string& env,
    const std::string& version,
    bool success,
    const std::string& deploy_link)
{
    return NotificationBuilder(NotificationType::Deployment)
        .SetProject(env)
        .SetTarget(version)
        .SetError(success ? "success" : "failed")
        .SetLink(deploy_link)
        .SetAction(success ? "View Deploy" : "Revert")
        .SetSeverity(success ? NotificationSeverity::Medium : NotificationSeverity::Critical);
}

inline NotificationBuilder SecurityAlert(const std::string& repo_file,
    NotificationSeverity severity,
    const std::string& remediation,
    const std::string& report_link)
{
    return NotificationBuilder(NotificationType::SecurityAlert)
        .SetSummary("Security: " + remediation)
        .SetProject(repo_file)
        .SetSeverity(severity)
        .SetError("remediation: " + remediation)
        .SetLink(report_link)
        .SetAction("View Report");
}

// ============================================================================
// NOTIFICATION MANAGER
// ============================================================================

class NotificationManager
{
public:
    std::vector<Notification> notifications;

    void Add(const Notification& n)
    {
        notifications.push_back(n);
        // Keep last 100
        if (notifications.size() > 100)
        {
            notifications.erase(notifications.begin());
        }
    }

    std::vector<Notification> GetBySeverity(NotificationSeverity s) const
    {
        std::vector<Notification> result;
        for (const auto& n : notifications)
        {
            if (n.severity == s)
                result.push_back(n);
        }
        return result;
    }

    std::vector<Notification> GetCritical() const
    {
        return GetBySeverity(NotificationSeverity::Critical);
    }

    std::vector<Notification> GetByProject(const std::string& project)
    {
        std::vector<Notification> result;
        for (const auto& n : notifications)
        {
            if (n.project == project)
                result.push_back(n);
        }
        return result;
    }

    void Clear()
    {
        notifications.clear();
    }

    size_t Count() const
    {
        return notifications.size();
    }
};

} // namespace ZepNotifications