#include <gtest/gtest.h>

namespace memochat::tests::ai::gateway_client
{
const char* DefaultAIServerHost();
const char* DefaultAIServerPort();
const char* DefaultAIServerInternalAuthHeader();
const char* DefaultAIServerInternalKeyEnv();
bool ShouldRejectAIServerAuthConfiguration(bool key_configured, bool local_environment, bool loopback_target);
bool ShouldUseDefaultHost(bool host_empty);
bool ShouldUseDefaultPort(bool port_empty);
const char* DefaultApiProviderAdapter();
bool ShouldUseDefaultAdapter(bool adapter_empty);
int UnavailableErrorCode();
const char* UnavailableMessage();
int StreamSuccessCode();
const char* StreamUnavailablePrefix();
const char* KbUploadFailedMessage();
const char* KbSearchFailedMessage();
const char* KbListFailedMessage();
const char* KbDeleteFailedMessage();
const char* MemoryListFailedMessage();
const char* MemoryCreateFailedMessage();
const char* MemoryDeleteFailedMessage();
const char* AgentTaskCreateFailedMessage();
const char* AgentTaskListFailedMessage();
const char* AgentTaskGetFailedMessage();
const char* AgentTaskCancelFailedMessage();
const char* AgentTaskResumeFailedMessage();
} // namespace memochat::tests::ai::gateway_client

namespace tc = memochat::tests::ai::gateway_client;

TEST(AIGatewayClientAlgorithmsTest, ExposesStableAIServerTargetDefaults)
{
    EXPECT_STREQ(tc::DefaultAIServerHost(), "127.0.0.1");
    EXPECT_STREQ(tc::DefaultAIServerPort(), "8095");
    EXPECT_STREQ(tc::DefaultAIServerInternalAuthHeader(), "X-MemoChat-AI-Internal-Key");
    EXPECT_STREQ(tc::DefaultAIServerInternalKeyEnv(), "MEMOCHAT_AI_INTERNAL_API_KEY");
}

TEST(AIGatewayClientAlgorithmsTest, FailsClosedUnlessKeylessTargetIsLocalLoopback)
{
    EXPECT_FALSE(tc::ShouldRejectAIServerAuthConfiguration(true, false, false));
    EXPECT_FALSE(tc::ShouldRejectAIServerAuthConfiguration(false, true, true));
    EXPECT_TRUE(tc::ShouldRejectAIServerAuthConfiguration(false, false, true));
    EXPECT_TRUE(tc::ShouldRejectAIServerAuthConfiguration(false, true, false));
    EXPECT_TRUE(tc::ShouldRejectAIServerAuthConfiguration(false, false, false));
}

TEST(AIGatewayClientAlgorithmsTest, PreservesEmptyFieldFallbackGuards)
{
    EXPECT_TRUE(tc::ShouldUseDefaultHost(true));
    EXPECT_FALSE(tc::ShouldUseDefaultHost(false));
    EXPECT_TRUE(tc::ShouldUseDefaultPort(true));
    EXPECT_FALSE(tc::ShouldUseDefaultPort(false));
    EXPECT_TRUE(tc::ShouldUseDefaultAdapter(true));
    EXPECT_FALSE(tc::ShouldUseDefaultAdapter(false));
}

TEST(AIGatewayClientAlgorithmsTest, ExposesDefaultApiProviderAdapter)
{
    EXPECT_STREQ(tc::DefaultApiProviderAdapter(), "openai_compatible");
}

TEST(AIGatewayClientAlgorithmsTest, PreservesSharedUnavailablePayload)
{
    EXPECT_EQ(tc::UnavailableErrorCode(), 500);
    EXPECT_STREQ(tc::UnavailableMessage(), "AIServer unavailable");
}

TEST(AIGatewayClientAlgorithmsTest, PreservesStreamFinalCodeAndPrefix)
{
    EXPECT_EQ(tc::StreamSuccessCode(), 0);
    EXPECT_STREQ(tc::StreamUnavailablePrefix(), "AIServer unavailable: ");
}

TEST(AIGatewayClientAlgorithmsTest, PreservesPerEndpointFailureMessages)
{
    EXPECT_STREQ(tc::KbUploadFailedMessage(), "upload failed");
    EXPECT_STREQ(tc::KbSearchFailedMessage(), "search failed");
    EXPECT_STREQ(tc::KbListFailedMessage(), "list failed");
    EXPECT_STREQ(tc::KbDeleteFailedMessage(), "delete failed");
    EXPECT_STREQ(tc::MemoryListFailedMessage(), "memory list failed");
    EXPECT_STREQ(tc::MemoryCreateFailedMessage(), "memory create failed");
    EXPECT_STREQ(tc::MemoryDeleteFailedMessage(), "memory delete failed");
    EXPECT_STREQ(tc::AgentTaskCreateFailedMessage(), "task create failed");
    EXPECT_STREQ(tc::AgentTaskListFailedMessage(), "task list failed");
    EXPECT_STREQ(tc::AgentTaskGetFailedMessage(), "task get failed");
    EXPECT_STREQ(tc::AgentTaskCancelFailedMessage(), "task cancel failed");
    EXPECT_STREQ(tc::AgentTaskResumeFailedMessage(), "task resume failed");
}
