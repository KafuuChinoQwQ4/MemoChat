#include <gtest/gtest.h>

#include <string>

bool MemoChatTestDeliveryWorkerIsConfigFlag(const std::string& value);
bool MemoChatTestDeliveryWorkerShouldRejectMissingConfigValue(bool has_next_arg);
bool MemoChatTestDeliveryWorkerShouldSetInstanceName(bool instance_name_empty);
bool MemoChatTestDeliveryWorkerShouldRejectEmptyWorkerName(bool worker_name_empty);
std::string MemoChatTestDeliveryWorkerMissingConfigValueMessage();
std::string MemoChatTestDeliveryWorkerUnknownArgumentPrefix();
std::string MemoChatTestDeliveryWorkerEmptyWorkerNameMessage();
long long MemoChatTestDeliveryWorkerDefaultDatacenterId();
long long MemoChatTestDeliveryWorkerDefaultWorkerId();
std::string MemoChatTestDeliveryWorkerLoggerName();
std::string MemoChatTestDeliveryWorkerEnabledText(bool worker_enabled);

TEST(ChatDeliveryWorkerRuntimeAlgorithmsTest, ClassifiesConfigArguments)
{
    EXPECT_TRUE(MemoChatTestDeliveryWorkerIsConfigFlag("--config"));
    EXPECT_FALSE(MemoChatTestDeliveryWorkerIsConfigFlag("--Config"));
    EXPECT_FALSE(MemoChatTestDeliveryWorkerIsConfigFlag("--config-path"));
    EXPECT_TRUE(MemoChatTestDeliveryWorkerShouldRejectMissingConfigValue(false));
    EXPECT_FALSE(MemoChatTestDeliveryWorkerShouldRejectMissingConfigValue(true));
}

TEST(ChatDeliveryWorkerRuntimeAlgorithmsTest, GuardsInstanceAndWorkerNames)
{
    EXPECT_FALSE(MemoChatTestDeliveryWorkerShouldSetInstanceName(true));
    EXPECT_TRUE(MemoChatTestDeliveryWorkerShouldSetInstanceName(false));
    EXPECT_TRUE(MemoChatTestDeliveryWorkerShouldRejectEmptyWorkerName(true));
    EXPECT_FALSE(MemoChatTestDeliveryWorkerShouldRejectEmptyWorkerName(false));
}

TEST(ChatDeliveryWorkerRuntimeAlgorithmsTest, DefinesStartupDefaultsAndMessages)
{
    EXPECT_EQ(MemoChatTestDeliveryWorkerMissingConfigValueMessage(), "missing value for --config");
    EXPECT_EQ(MemoChatTestDeliveryWorkerUnknownArgumentPrefix(), "unknown argument: ");
    EXPECT_EQ(MemoChatTestDeliveryWorkerEmptyWorkerNameMessage(), "chat delivery worker node name is empty");
    EXPECT_EQ(MemoChatTestDeliveryWorkerDefaultDatacenterId(), 1);
    EXPECT_EQ(MemoChatTestDeliveryWorkerDefaultWorkerId(), 1);
    EXPECT_EQ(MemoChatTestDeliveryWorkerLoggerName(), "ChatDeliveryWorker");
}

TEST(ChatDeliveryWorkerRuntimeAlgorithmsTest, DefinesWorkerEnabledText)
{
    EXPECT_EQ(MemoChatTestDeliveryWorkerEnabledText(true), "true");
    EXPECT_EQ(MemoChatTestDeliveryWorkerEnabledText(false), "false");
}
