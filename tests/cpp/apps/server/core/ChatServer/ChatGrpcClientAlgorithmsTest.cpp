#include <gtest/gtest.h>

#include <string>

bool MemoChatTestChatGrpcShouldSkipSelfNode(const std::string& node_name, const std::string& self_name);
int MemoChatTestChatGrpcDefaultRemotePoolSize();
bool MemoChatTestChatGrpcShouldSkipRemoteCall(bool pool_found);
bool MemoChatTestChatGrpcShouldReportStatusError(bool status_ok);

TEST(ChatGrpcClientAlgorithmsTest, SkipsOnlyExactSelfNode)
{
    EXPECT_TRUE(MemoChatTestChatGrpcShouldSkipSelfNode("chat-a", "chat-a"));
    EXPECT_FALSE(MemoChatTestChatGrpcShouldSkipSelfNode("chat-a", "chat-b"));
    EXPECT_FALSE(MemoChatTestChatGrpcShouldSkipSelfNode("chat-a", "CHAT-A"));
    EXPECT_FALSE(MemoChatTestChatGrpcShouldSkipSelfNode("", "chat-a"));
}

TEST(ChatGrpcClientAlgorithmsTest, KeepsLegacyPoolAndRemoteCallGuards)
{
    EXPECT_EQ(MemoChatTestChatGrpcDefaultRemotePoolSize(), 5);

    EXPECT_FALSE(MemoChatTestChatGrpcShouldSkipRemoteCall(true));
    EXPECT_TRUE(MemoChatTestChatGrpcShouldSkipRemoteCall(false));
}

TEST(ChatGrpcClientAlgorithmsTest, ReportsGrpcStatusErrorsOnlyWhenStatusFails)
{
    EXPECT_FALSE(MemoChatTestChatGrpcShouldReportStatusError(true));
    EXPECT_TRUE(MemoChatTestChatGrpcShouldReportStatusError(false));
}
