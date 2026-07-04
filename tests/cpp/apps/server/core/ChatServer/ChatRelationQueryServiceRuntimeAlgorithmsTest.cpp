#include <gtest/gtest.h>

#include <string>

bool MemoChatTestRelationQueryServiceIsConfigFlag(const std::string& value);
bool MemoChatTestRelationQueryServiceShouldRejectMissingConfigValue(bool has_next_arg);
bool MemoChatTestRelationQueryServiceShouldUseDefaultConfigValue(bool value_empty);
bool MemoChatTestRelationQueryServiceShouldSetInstanceName(bool instance_name_empty);
std::string MemoChatTestRelationQueryServiceDefaultName();
std::string MemoChatTestRelationQueryServiceDefaultHost();
std::string MemoChatTestRelationQueryServiceDefaultPort();
long long MemoChatTestRelationQueryServiceDefaultDatacenterId();
long long MemoChatTestRelationQueryServiceDefaultWorkerId();
std::string MemoChatTestRelationQueryServiceLoggerName();

TEST(ChatRelationQueryServiceRuntimeAlgorithmsTest, ClassifiesConfigArguments)
{
    EXPECT_TRUE(MemoChatTestRelationQueryServiceIsConfigFlag("--config"));
    EXPECT_FALSE(MemoChatTestRelationQueryServiceIsConfigFlag("--Config"));
    EXPECT_FALSE(MemoChatTestRelationQueryServiceIsConfigFlag("--config-path"));
    EXPECT_TRUE(MemoChatTestRelationQueryServiceShouldRejectMissingConfigValue(false));
    EXPECT_FALSE(MemoChatTestRelationQueryServiceShouldRejectMissingConfigValue(true));
}

TEST(ChatRelationQueryServiceRuntimeAlgorithmsTest, SelectsDefaultsAndInstanceNameGuard)
{
    EXPECT_TRUE(MemoChatTestRelationQueryServiceShouldUseDefaultConfigValue(true));
    EXPECT_FALSE(MemoChatTestRelationQueryServiceShouldUseDefaultConfigValue(false));
    EXPECT_FALSE(MemoChatTestRelationQueryServiceShouldSetInstanceName(true));
    EXPECT_TRUE(MemoChatTestRelationQueryServiceShouldSetInstanceName(false));
}

TEST(ChatRelationQueryServiceRuntimeAlgorithmsTest, DefinesStartupDefaults)
{
    EXPECT_EQ(MemoChatTestRelationQueryServiceDefaultName(), "chatrelationquery1");
    EXPECT_EQ(MemoChatTestRelationQueryServiceDefaultHost(), "127.0.0.1");
    EXPECT_EQ(MemoChatTestRelationQueryServiceDefaultPort(), "50090");
    EXPECT_EQ(MemoChatTestRelationQueryServiceDefaultDatacenterId(), 1);
    EXPECT_EQ(MemoChatTestRelationQueryServiceDefaultWorkerId(), 8);
}

TEST(ChatRelationQueryServiceRuntimeAlgorithmsTest, DefinesLoggerName)
{
    EXPECT_EQ(MemoChatTestRelationQueryServiceLoggerName(), "ChatRelationQueryService");
}
