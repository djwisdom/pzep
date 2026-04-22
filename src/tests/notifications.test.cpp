#include "config_app.h"
#include "zep/mcommon/logger.h"

#include "../../notifications.h"
#include <gtest/gtest.h>

using namespace ZepNotifications;

TEST(NotificationCore, SeverityEnum)
{
    EXPECT_EQ(SeverityToString(NotificationSeverity::Critical), "CRITICAL");
    EXPECT_EQ(SeverityToString(NotificationSeverity::High), "HIGH");
    EXPECT_EQ(SeverityToString(NotificationSeverity::Medium), "MEDIUM");
    EXPECT_EQ(SeverityToString(NotificationSeverity::Low), "LOW");
    EXPECT_EQ(SeverityToString(NotificationSeverity::Info), "INFO");
}

TEST(NotificationCore, TypeEnum)
{
    EXPECT_EQ(TypeToString(NotificationType::BuildFailure), "BUILD");
    EXPECT_EQ(TypeToString(NotificationType::TestFailure), "TESTS");
    EXPECT_EQ(TypeToString(NotificationType::MergeCIStatus), "CI");
    EXPECT_EQ(TypeToString(NotificationType::CodeReviewRequest), "REVIEW");
    EXPECT_EQ(TypeToString(NotificationType::RuntimeError), "RUNTIME");
    EXPECT_EQ(TypeToString(NotificationType::Deployment), "DEPLOY");
    EXPECT_EQ(TypeToString(NotificationType::SecurityAlert), "SECURITY");
    EXPECT_EQ(TypeToString(NotificationType::PerformanceRegression), "PERF");
    EXPECT_EQ(TypeToString(NotificationType::FlakyTest), "FLAKY");
    EXPECT_EQ(TypeToString(NotificationType::DependencyVuln), "VULN");
    EXPECT_EQ(TypeToString(NotificationType::LongRunningTask), "TASK");
    EXPECT_EQ(TypeToString(NotificationType::WorkspaceEvent), "WORKSPACE");
}

TEST(NotificationBuilder, BuildFailure)
{
    auto n = BuildFailed(
        "core-lib",
        "all",
        "undefined reference",
        "src/util.cpp:128",
        "1234",
        "http://jenkins/build/1234")
                 .Build();

    EXPECT_EQ(n.type, NotificationType::BuildFailure);
    EXPECT_EQ(n.project, "core-lib");
    EXPECT_EQ(n.target, "all");
    EXPECT_EQ(n.first_error, "undefined reference");
    EXPECT_EQ(n.error_location, "src/util.cpp:128");
    EXPECT_EQ(n.id, "1234");
    EXPECT_EQ(n.link, "http://jenkins/build/1234");
    EXPECT_EQ(n.severity, NotificationSeverity::Critical);
}

TEST(NotificationBuilder, TestFailure)
{
    auto n = TestFailed(
        "auth-suite",
        "test_login: expected 200 got 500",
        "http://jenkins/test/456")
                 .Build();

    EXPECT_EQ(n.type, NotificationType::TestFailure);
    EXPECT_EQ(n.target, "auth-suite");
    EXPECT_EQ(n.first_error, "test_login: expected 200 got 500");
    EXPECT_EQ(n.severity, NotificationSeverity::Critical);
}

TEST(NotificationBuilder, RuntimeError)
{
    auto n = RuntimeError(
        "auth-service",
        "NullPointerException",
        "AuthController.login()",
        "req-abc123",
        "http://trace/service/abc123")
                 .Build();

    EXPECT_EQ(n.type, NotificationType::RuntimeError);
    EXPECT_EQ(n.project, "auth-service");
    EXPECT_EQ(n.first_error, "NullPointerException");
    EXPECT_EQ(n.error_location, "AuthController.login()");
    EXPECT_EQ(n.request_id, "req-abc123");
    EXPECT_EQ(n.severity, NotificationSeverity::Critical);
}

TEST(NotificationBuilder, Deployment)
{
    auto n = DeployComplete(
        "staging",
        "v2.1.0",
        true,
        "http://deploy/staging/789")
                 .Build();

    EXPECT_EQ(n.type, NotificationType::Deployment);
    EXPECT_EQ(n.project, "staging");
    EXPECT_EQ(n.target, "v2.1.0");
    EXPECT_EQ(n.first_error, "success");
    EXPECT_EQ(n.severity, NotificationSeverity::Medium);
}

TEST(NotificationBuilder, DeploymentFailed)
{
    auto n = DeployComplete(
        "production",
        "v2.0.9",
        false,
        "http://deploy/prod/999")
                 .Build();

    EXPECT_EQ(n.type, NotificationType::Deployment);
    EXPECT_EQ(n.project, "production");
    EXPECT_EQ(n.first_error, "failed");
    EXPECT_EQ(n.severity, NotificationSeverity::Critical);
}

TEST(NotificationBuilder, SecurityAlert)
{
    auto n = SecurityAlert(
        "lib/utils.js",
        NotificationSeverity::High,
        "update lodash@>4.5.0",
        "http://gh/advisory/789")
                 .Build();

    EXPECT_EQ(n.type, NotificationType::SecurityAlert);
    EXPECT_EQ(n.project, "lib/utils.js");
    EXPECT_EQ(n.severity, NotificationSeverity::High);
}

TEST(NotificationBuilder, FluentAPI)
{
    auto n = NotificationBuilder(NotificationType::BuildFailure)
                 .SetSummary("Test build")
                 .SetProject("myproject")
                 .SetTarget("all")
                 .SetError("test error")
                 .SetID("999")
                 .SetLink("http://test")
                 .SetSeverity(NotificationSeverity::High)
                 .Build();

    EXPECT_EQ(n.summary, "Test build");
    EXPECT_EQ(n.project, "myproject");
    EXPECT_EQ(n.target, "all");
    EXPECT_EQ(n.first_error, "test error");
    EXPECT_EQ(n.id, "999");
    EXPECT_EQ(n.link, "http://test");
    EXPECT_EQ(n.severity, NotificationSeverity::High);
}

TEST(NotificationBuilder, FileLocation)
{
    auto n = NotificationBuilder(NotificationType::RuntimeError)
                 .SetFile("src/main.cpp", 42)
                 .Build();

    ASSERT_TRUE(n.file_path.has_value());
    EXPECT_EQ(*n.file_path, "src/main.cpp");
    ASSERT_TRUE(n.file_line.has_value());
    EXPECT_EQ(*n.file_line, 42);
}

TEST(Formatting, BuildFailure)
{
    Notification n;
    n.type = NotificationType::BuildFailure;
    n.project = "core-lib";
    n.target = "all";
    n.first_error = "undefined reference";
    n.error_location = "src/util.cpp:128";
    n.id = "1234";

    std::string formatted = FormatBuildFailure(n);
    EXPECT_NE(formatted.find("Build failed"), std::string::npos);
    EXPECT_NE(formatted.find("core-lib"), std::string::npos);
    EXPECT_NE(formatted.find("undefined reference"), std::string::npos);
    EXPECT_NE(formatted.find("src/util.cpp:128"), std::string::npos);
    EXPECT_NE(formatted.find("1234"), std::string::npos);
}

TEST(Formatting, TestFailure)
{
    Notification n;
    n.type = NotificationType::TestFailure;
    n.target = "auth-suite";
    n.first_error = "expected 200 got 500";

    std::string formatted = FormatTestFailure(n);
    EXPECT_NE(formatted.find("Tests broken"), std::string::npos);
    EXPECT_NE(formatted.find("auth-suite"), std::string::npos);
}

TEST(Formatting, RuntimeError)
{
    Notification n;
    n.type = NotificationType::RuntimeError;
    n.project = "auth-service";
    n.first_error = "NullPointerException";
    n.error_location = "AuthController.login()";
    n.request_id = "req-abc";

    std::string formatted = FormatRuntimeError(n);
    EXPECT_NE(formatted.find("Prod ERROR"), std::string::npos);
    EXPECT_NE(formatted.find("auth-service"), std::string::npos);
    EXPECT_NE(formatted.find("NullPointerException"), std::string::npos);
    EXPECT_NE(formatted.find("req-abc"), std::string::npos);
}

TEST(Formatting, Deployment)
{
    Notification n;
    n.type = NotificationType::Deployment;
    n.project = "staging";
    n.target = "v2.1.0";
    n.first_error = "success";

    std::string formatted = FormatDeployment(n);
    EXPECT_NE(formatted.find("Deployment"), std::string::npos);
    EXPECT_NE(formatted.find("staging"), std::string::npos);
    EXPECT_NE(formatted.find("v2.1.0"), std::string::npos);
}

TEST(Formatting, SecurityAlert)
{
    Notification n;
    n.type = NotificationType::SecurityAlert;
    n.project = "lib/utils.js";
    n.severity = NotificationSeverity::High;
    n.first_error = "update package";

    std::string formatted = FormatSecurityAlert(n);
    EXPECT_NE(formatted.find("CRITICAL: Security"), std::string::npos);
    EXPECT_NE(formatted.find("lib/utils.js"), std::string::npos);
    EXPECT_NE(formatted.find("HIGH"), std::string::npos);
}

TEST(NotificationManager, AddAndCount)
{
    NotificationManager m;
    EXPECT_EQ(m.Count(), 0);

    m.Add(BuildFailed("proj", "tgt", "err", "loc", "id", "url").Build());
    EXPECT_EQ(m.Count(), 1);

    m.Add(TestFailed("suite", "err", "url").Build());
    EXPECT_EQ(m.Count(), 2);
}

TEST(NotificationManager, Clear)
{
    NotificationManager m;
    m.Add(BuildFailed("proj", "tgt", "err", "loc", "id", "url").Build());
    EXPECT_EQ(m.Count(), 1);

    m.Clear();
    EXPECT_EQ(m.Count(), 0);
}

TEST(NotificationManager, GetCritical)
{
    NotificationManager m;
    m.Add(BuildFailed("proj", "tgt", "err", "loc", "id", "url").Build()); // Critical
    m.Add(TestFailed("suite", "err", "url").Build()); // Critical
    m.Add(DeployComplete("env", "v1", true, "url").Build()); // Medium

    auto critical = m.GetCritical();
    EXPECT_EQ(critical.size(), 2);
}

TEST(NotificationManager, GetByProject)
{
    NotificationManager m;
    m.Add(BuildFailed("proj-a", "tgt", "err", "loc", "id", "url").Build());
    m.Add(BuildFailed("proj-b", "tgt", "err", "loc", "id", "url").Build());
    m.Add(BuildFailed("proj-a", "tgt", "err", "loc", "id", "url").Build());

    auto projA = m.GetByProject("proj-a");
    EXPECT_EQ(projA.size(), 2);

    auto projB = m.GetByProject("proj-b");
    EXPECT_EQ(projB.size(), 1);
}

TEST(NotificationManager, CircularBuffer)
{
    NotificationManager m;

    // Add 101 notifications
    for (int i = 0; i < 101; i++)
    {
        Notification n;
        n.summary = "test " + std::to_string(i);
        m.Add(n);
    }

    // Should keep last 100
    EXPECT_EQ(m.Count(), 100);
}

TEST(NotificationBuilder, AsCriticalShortcut)
{
    auto n = NotificationBuilder(NotificationType::BuildFailure)
                 .AsCritical()
                 .Build();

    EXPECT_EQ(n.severity, NotificationSeverity::Critical);
}

TEST(NotificationBuilder, SetSeverityShortcut)
{
    auto n = NotificationBuilder(NotificationType::BuildFailure)
                 .AsHigh()
                 .Build();

    EXPECT_EQ(n.severity, NotificationSeverity::High);
}

TEST(NotificationTypes, AllTypesHaveBuilders)
{
    // Verify all notification type builders exist and compile
    auto n1 = BuildFailed("p", "t", "e", "l", "i", "u").Build();
    auto n2 = TestFailed("t", "e", "u").Build();
    auto n3 = RuntimeError("s", "e", "l", "r", "u").Build();
    auto n4 = DeployComplete("e", "v", true, "u").Build();
    auto n5 = SecurityAlert("f", NotificationSeverity::High, "r", "u").Build();

    EXPECT_EQ(n1.type, NotificationType::BuildFailure);
    EXPECT_EQ(n2.type, NotificationType::TestFailure);
    EXPECT_EQ(n3.type, NotificationType::RuntimeError);
    EXPECT_EQ(n4.type, NotificationType::Deployment);
    EXPECT_EQ(n5.type, NotificationType::SecurityAlert);
}

TEST(NotificationRequiredFields, BuildFailureComplete)
{
    auto n = BuildFailed(
        "core-lib",
        "all",
        "undefined reference",
        "src/util.cpp:128",
        "1234",
        "http://jenkins/build/1234")
                 .Build();

    // All required fields should be populated
    EXPECT_FALSE(n.summary.empty());
    EXPECT_FALSE(n.project.empty());
    EXPECT_FALSE(n.target.empty());
    EXPECT_FALSE(n.id.empty());
    EXPECT_FALSE(n.link.empty());
    EXPECT_NE(n.timestamp.time_since_epoch().count(), 0);
}