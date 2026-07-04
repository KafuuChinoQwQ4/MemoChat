#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::ai::route_registration
{
const char* GetMethod();
const char* PostMethod();
const char* ChatPath();
const char* SmartPath();
const char* HistoryPath();
const char* SessionPath();
const char* SessionListPath();
const char* SessionDeletePath();
const char* SessionUpdatePath();
const char* ModelListPath();
const char* ModelApiRegisterPath();
const char* ModelApiDeletePath();
const char* KbUploadPath();
const char* KbSearchPath();
const char* KbListPath();
const char* KbDeletePath();
const char* MemoryListPath();
const char* MemoryPath();
const char* MemoryDeletePath();
const char* TasksPath();
const char* TaskDetailPath();
const char* TaskCancelPath();
const char* TaskResumePath();
} // namespace memochat::tests::ai::route_registration

TEST(AIRouteRegistrationAlgorithmsTest, ExposesOnlyStableHttpMethods)
{
    EXPECT_STREQ(memochat::tests::ai::route_registration::GetMethod(), "GET");
    EXPECT_STREQ(memochat::tests::ai::route_registration::PostMethod(), "POST");
}

TEST(AIRouteRegistrationAlgorithmsTest, ExposesAIGatewayRouteRegistrationPaths)
{
    using memochat::tests::ai::route_registration::ChatPath;
    using memochat::tests::ai::route_registration::HistoryPath;
    using memochat::tests::ai::route_registration::KbDeletePath;
    using memochat::tests::ai::route_registration::KbListPath;
    using memochat::tests::ai::route_registration::KbSearchPath;
    using memochat::tests::ai::route_registration::KbUploadPath;
    using memochat::tests::ai::route_registration::MemoryDeletePath;
    using memochat::tests::ai::route_registration::MemoryListPath;
    using memochat::tests::ai::route_registration::MemoryPath;
    using memochat::tests::ai::route_registration::ModelApiDeletePath;
    using memochat::tests::ai::route_registration::ModelApiRegisterPath;
    using memochat::tests::ai::route_registration::ModelListPath;
    using memochat::tests::ai::route_registration::SessionDeletePath;
    using memochat::tests::ai::route_registration::SessionListPath;
    using memochat::tests::ai::route_registration::SessionPath;
    using memochat::tests::ai::route_registration::SessionUpdatePath;
    using memochat::tests::ai::route_registration::SmartPath;
    using memochat::tests::ai::route_registration::TaskCancelPath;
    using memochat::tests::ai::route_registration::TaskDetailPath;
    using memochat::tests::ai::route_registration::TaskResumePath;
    using memochat::tests::ai::route_registration::TasksPath;

    constexpr std::array<std::string_view, 21> expected = {
        "/ai/chat",
        "/ai/smart",
        "/ai/history",
        "/ai/session",
        "/ai/session/list",
        "/ai/session/delete",
        "/ai/session/update",
        "/ai/model/list",
        "/ai/model/api/register",
        "/ai/model/api/delete",
        "/ai/kb/upload",
        "/ai/kb/search",
        "/ai/kb/list",
        "/ai/kb/delete",
        "/ai/memory/list",
        "/ai/memory",
        "/ai/memory/delete",
        "/ai/tasks",
        "/ai/tasks/detail",
        "/ai/tasks/cancel",
        "/ai/tasks/resume",
    };
    const std::array<std::string_view, 21> actual = {
        ChatPath(),          SmartPath(),         HistoryPath(),   SessionPath(),          SessionListPath(),
        SessionDeletePath(), SessionUpdatePath(), ModelListPath(), ModelApiRegisterPath(), ModelApiDeletePath(),
        KbUploadPath(),      KbSearchPath(),      KbListPath(),    KbDeletePath(),         MemoryListPath(),
        MemoryPath(),        MemoryDeletePath(),  TasksPath(),     TaskDetailPath(),       TaskCancelPath(),
        TaskResumePath(),
    };

    EXPECT_EQ(actual, expected);
}
