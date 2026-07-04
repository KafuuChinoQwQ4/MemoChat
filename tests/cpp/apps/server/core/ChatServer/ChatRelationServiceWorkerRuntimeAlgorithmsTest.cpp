#include <gtest/gtest.h>

#include <string>

bool MemoChatTestRelationServiceWorkerIsConfigFlag(const std::string& value);
bool MemoChatTestRelationServiceWorkerShouldRejectMissingConfigValue(bool has_next_arg);
bool MemoChatTestRelationServiceWorkerShouldUseDefaultConfigValue(bool value_empty);
bool MemoChatTestRelationServiceWorkerShouldSetInstanceName(bool instance_name_empty);
std::string MemoChatTestRelationServiceWorkerDefaultName();
std::string MemoChatTestRelationServiceWorkerDefaultHost();
std::string MemoChatTestRelationServiceWorkerDefaultPort();
long long MemoChatTestRelationServiceWorkerDefaultDatacenterId();
long long MemoChatTestRelationServiceWorkerDefaultWorkerId();
std::string MemoChatTestRelationServiceWorkerLoggerName();
bool MemoChatTestRelationServiceWorkerIsRabbitMqBackend(const std::string& value);
bool MemoChatTestRelationServiceWorkerIsKafkaBackend(const std::string& value);
std::string MemoChatTestRelationServiceWorkerEventBusUnavailableError();
std::string MemoChatTestRelationServiceWorkerRabbitMqUnavailableLogEvent();
std::string MemoChatTestRelationServiceWorkerRabbitMqUnavailableLogMessage();
std::string MemoChatTestRelationServiceWorkerKafkaUnavailableLogEvent();
std::string MemoChatTestRelationServiceWorkerKafkaUnavailableLogMessage();

TEST(ChatRelationServiceWorkerRuntimeAlgorithmsTest, ClassifiesConfigArguments)
{
    EXPECT_TRUE(MemoChatTestRelationServiceWorkerIsConfigFlag("--config"));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerIsConfigFlag("--Config"));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerIsConfigFlag("--config-path"));
    EXPECT_TRUE(MemoChatTestRelationServiceWorkerShouldRejectMissingConfigValue(false));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerShouldRejectMissingConfigValue(true));
}

TEST(ChatRelationServiceWorkerRuntimeAlgorithmsTest, SelectsDefaultsAndInstanceNameGuard)
{
    EXPECT_TRUE(MemoChatTestRelationServiceWorkerShouldUseDefaultConfigValue(true));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerShouldUseDefaultConfigValue(false));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerShouldSetInstanceName(true));
    EXPECT_TRUE(MemoChatTestRelationServiceWorkerShouldSetInstanceName(false));
}

TEST(ChatRelationServiceWorkerRuntimeAlgorithmsTest, DefinesStartupDefaults)
{
    EXPECT_EQ(MemoChatTestRelationServiceWorkerDefaultName(), "chatrelationservice1");
    EXPECT_EQ(MemoChatTestRelationServiceWorkerDefaultHost(), "127.0.0.1");
    EXPECT_EQ(MemoChatTestRelationServiceWorkerDefaultPort(), "50091");
    EXPECT_EQ(MemoChatTestRelationServiceWorkerDefaultDatacenterId(), 1);
    EXPECT_EQ(MemoChatTestRelationServiceWorkerDefaultWorkerId(), 9);
    EXPECT_EQ(MemoChatTestRelationServiceWorkerLoggerName(), "ChatRelationServiceWorker");
}

TEST(ChatRelationServiceWorkerRuntimeAlgorithmsTest, ClassifiesQueueBackends)
{
    EXPECT_TRUE(MemoChatTestRelationServiceWorkerIsRabbitMqBackend("rabbitmq"));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerIsRabbitMqBackend("RabbitMQ"));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerIsRabbitMqBackend("inline"));
    EXPECT_TRUE(MemoChatTestRelationServiceWorkerIsKafkaBackend("kafka"));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerIsKafkaBackend("Kafka"));
    EXPECT_FALSE(MemoChatTestRelationServiceWorkerIsKafkaBackend("redis"));
}

TEST(ChatRelationServiceWorkerRuntimeAlgorithmsTest, DefinesBusFallbackLiterals)
{
    EXPECT_EQ(MemoChatTestRelationServiceWorkerEventBusUnavailableError(), "event_bus_unavailable");
    EXPECT_EQ(MemoChatTestRelationServiceWorkerRabbitMqUnavailableLogEvent(),
              "relation_service.task_bus.rabbitmq_unavailable");
    EXPECT_EQ(MemoChatTestRelationServiceWorkerRabbitMqUnavailableLogMessage(),
              "rabbitmq task bus unavailable in this build, falling back to inline");
    EXPECT_EQ(MemoChatTestRelationServiceWorkerKafkaUnavailableLogEvent(),
              "relation_service.event_bus.kafka_unavailable");
    EXPECT_EQ(MemoChatTestRelationServiceWorkerKafkaUnavailableLogMessage(),
              "kafka async event bus unavailable in this build, falling back to redis");
}
