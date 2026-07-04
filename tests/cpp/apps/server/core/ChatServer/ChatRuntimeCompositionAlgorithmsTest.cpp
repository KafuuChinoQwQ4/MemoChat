#include <gtest/gtest.h>

#include <string>

bool MemoChatTestUseRabbitMqTaskBus(const std::string& backend, bool rabbitmq_available);
bool MemoChatTestWarnRabbitMqTaskBusUnavailable(const std::string& backend, bool rabbitmq_available);
bool MemoChatTestWarnUnsupportedTaskBusBackend(const std::string& backend);
bool MemoChatTestUseKafkaAsyncEventBus(const std::string& backend, bool kafka_available);
bool MemoChatTestWarnKafkaAsyncEventBusUnavailable(const std::string& backend, bool kafka_available);
bool MemoChatTestWarnUnsupportedAsyncEventBusBackend(const std::string& backend);
std::string MemoChatTestInlineTaskBusBackend();
std::string MemoChatTestRedisAsyncEventBusBackend();

TEST(ChatRuntimeCompositionAlgorithmsTest, SelectsRabbitMqOnlyWhenConfiguredAndAvailable)
{
    EXPECT_TRUE(MemoChatTestUseRabbitMqTaskBus("rabbitmq", true));
    EXPECT_FALSE(MemoChatTestUseRabbitMqTaskBus("rabbitmq", false));
    EXPECT_FALSE(MemoChatTestUseRabbitMqTaskBus("inline", true));

    EXPECT_TRUE(MemoChatTestWarnRabbitMqTaskBusUnavailable("rabbitmq", false));
    EXPECT_FALSE(MemoChatTestWarnRabbitMqTaskBusUnavailable("rabbitmq", true));
    EXPECT_FALSE(MemoChatTestWarnRabbitMqTaskBusUnavailable("inline", false));

    EXPECT_EQ(MemoChatTestInlineTaskBusBackend(), "inline");
}

TEST(ChatRuntimeCompositionAlgorithmsTest, WarnsOnlyForUnsupportedTaskBusBackends)
{
    EXPECT_FALSE(MemoChatTestWarnUnsupportedTaskBusBackend("inline"));
    EXPECT_FALSE(MemoChatTestWarnUnsupportedTaskBusBackend("rabbitmq"));
    EXPECT_TRUE(MemoChatTestWarnUnsupportedTaskBusBackend("redis"));
    EXPECT_TRUE(MemoChatTestWarnUnsupportedTaskBusBackend(""));
}

TEST(ChatRuntimeCompositionAlgorithmsTest, SelectsKafkaOnlyWhenConfiguredAndAvailable)
{
    EXPECT_TRUE(MemoChatTestUseKafkaAsyncEventBus("kafka", true));
    EXPECT_FALSE(MemoChatTestUseKafkaAsyncEventBus("kafka", false));
    EXPECT_FALSE(MemoChatTestUseKafkaAsyncEventBus("redis", true));

    EXPECT_TRUE(MemoChatTestWarnKafkaAsyncEventBusUnavailable("kafka", false));
    EXPECT_FALSE(MemoChatTestWarnKafkaAsyncEventBusUnavailable("kafka", true));
    EXPECT_FALSE(MemoChatTestWarnKafkaAsyncEventBusUnavailable("redis", false));

    EXPECT_EQ(MemoChatTestRedisAsyncEventBusBackend(), "redis");
}

TEST(ChatRuntimeCompositionAlgorithmsTest, WarnsOnlyForUnsupportedAsyncEventBusBackends)
{
    EXPECT_FALSE(MemoChatTestWarnUnsupportedAsyncEventBusBackend("redis"));
    EXPECT_FALSE(MemoChatTestWarnUnsupportedAsyncEventBusBackend("kafka"));
    EXPECT_TRUE(MemoChatTestWarnUnsupportedAsyncEventBusBackend("rabbitmq"));
    EXPECT_TRUE(MemoChatTestWarnUnsupportedAsyncEventBusBackend(""));
}
