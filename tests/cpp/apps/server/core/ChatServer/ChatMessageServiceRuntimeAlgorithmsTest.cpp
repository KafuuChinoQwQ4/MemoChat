#include <gtest/gtest.h>

#include <string>

bool MemoChatTestMessageServiceIsConfigFlag(const std::string& value);
bool MemoChatTestMessageServiceShouldRejectMissingConfigValue(bool has_next_arg);
bool MemoChatTestMessageServiceShouldUseDefaultConfigValue(bool value_empty);
bool MemoChatTestMessageServiceShouldSetInstanceName(bool instance_name_empty);
std::string MemoChatTestMessageServiceDefaultName();
std::string MemoChatTestMessageServiceDefaultHost();
std::string MemoChatTestMessageServiceDefaultPort();
long long MemoChatTestMessageServiceDefaultDatacenterId();
long long MemoChatTestMessageServiceDefaultWorkerId();
std::string MemoChatTestMessageServiceDisabledEventPublishError();
std::string MemoChatTestMessageServiceLoggerName();

TEST(ChatMessageServiceRuntimeAlgorithmsTest, ClassifiesConfigArguments)
{
    EXPECT_TRUE(MemoChatTestMessageServiceIsConfigFlag("--config"));
    EXPECT_FALSE(MemoChatTestMessageServiceIsConfigFlag("--Config"));
    EXPECT_FALSE(MemoChatTestMessageServiceIsConfigFlag("--config-path"));
    EXPECT_TRUE(MemoChatTestMessageServiceShouldRejectMissingConfigValue(false));
    EXPECT_FALSE(MemoChatTestMessageServiceShouldRejectMissingConfigValue(true));
}

TEST(ChatMessageServiceRuntimeAlgorithmsTest, SelectsDefaultsAndInstanceNameGuard)
{
    EXPECT_TRUE(MemoChatTestMessageServiceShouldUseDefaultConfigValue(true));
    EXPECT_FALSE(MemoChatTestMessageServiceShouldUseDefaultConfigValue(false));
    EXPECT_FALSE(MemoChatTestMessageServiceShouldSetInstanceName(true));
    EXPECT_TRUE(MemoChatTestMessageServiceShouldSetInstanceName(false));
}

TEST(ChatMessageServiceRuntimeAlgorithmsTest, DefinesStartupDefaults)
{
    EXPECT_EQ(MemoChatTestMessageServiceDefaultName(), "chatmessageservice1");
    EXPECT_EQ(MemoChatTestMessageServiceDefaultHost(), "127.0.0.1");
    EXPECT_EQ(MemoChatTestMessageServiceDefaultPort(), "50092");
    EXPECT_EQ(MemoChatTestMessageServiceDefaultDatacenterId(), 1);
    EXPECT_EQ(MemoChatTestMessageServiceDefaultWorkerId(), 10);
}

TEST(ChatMessageServiceRuntimeAlgorithmsTest, DefinesServiceLiterals)
{
    EXPECT_EQ(MemoChatTestMessageServiceDisabledEventPublishError(),
              "ChatMessageService event publishing is disabled in this scaffold");
    EXPECT_EQ(MemoChatTestMessageServiceLoggerName(), "ChatMessageService");
}
