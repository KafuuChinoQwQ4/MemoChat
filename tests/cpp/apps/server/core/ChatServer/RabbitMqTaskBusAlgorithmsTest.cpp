#include <gtest/gtest.h>

bool MemoChatTestRabbitTaskBuildAvailable(bool compiled_with_rabbitmq);
bool MemoChatTestRabbitTaskShouldRejectInvalidConfig(bool config_valid);
bool MemoChatTestRabbitTaskShouldRejectRpcReply(bool normal_reply);
bool MemoChatTestRabbitTaskShouldReconnect(bool connection_missing);
bool MemoChatTestRabbitTaskShouldAckLastConsumed(bool connection_present, bool last_envelope_present);
bool MemoChatTestRabbitTaskShouldNackLastConsumed(bool connection_present, bool last_envelope_present);
bool MemoChatTestRabbitTaskShouldRouteToDeadLetter(int retry_count, int max_retries);
bool MemoChatTestRabbitTaskShouldRejectPublishResult(bool publish_ok);
int MemoChatTestRabbitTaskPersistentDeliveryMode();
const char* MemoChatTestRabbitTaskQueueSuffix();
const char* MemoChatTestRabbitTaskRetryQueueSuffix();
const char* MemoChatTestRabbitTaskDlqSuffix();
const char* MemoChatTestRabbitTaskContentTypeJson();
const char* MemoChatTestRabbitTaskAmqpRpcReplyFailedError();
const char* MemoChatTestRabbitTaskBuildDisabledError();
const char* MemoChatTestRabbitTaskInvalidConfigError();
const char* MemoChatTestRabbitTaskSocketCreateFailedError();
const char* MemoChatTestRabbitTaskSocketOpenFailedError();
const char* MemoChatTestRabbitTaskPublishFailedError();

TEST(RabbitMqTaskBusAlgorithmsTest, KeepsBuildConfigRpcAndReconnectGuards)
{
    EXPECT_TRUE(MemoChatTestRabbitTaskBuildAvailable(true));
    EXPECT_FALSE(MemoChatTestRabbitTaskBuildAvailable(false));

    EXPECT_FALSE(MemoChatTestRabbitTaskShouldRejectInvalidConfig(true));
    EXPECT_TRUE(MemoChatTestRabbitTaskShouldRejectInvalidConfig(false));

    EXPECT_FALSE(MemoChatTestRabbitTaskShouldRejectRpcReply(true));
    EXPECT_TRUE(MemoChatTestRabbitTaskShouldRejectRpcReply(false));

    EXPECT_TRUE(MemoChatTestRabbitTaskShouldReconnect(true));
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldReconnect(false));
}

TEST(RabbitMqTaskBusAlgorithmsTest, RequiresConnectionAndLastEnvelopeForAckAndNack)
{
    EXPECT_TRUE(MemoChatTestRabbitTaskShouldAckLastConsumed(true, true));
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldAckLastConsumed(false, true));
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldAckLastConsumed(true, false));
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldAckLastConsumed(false, false));

    EXPECT_TRUE(MemoChatTestRabbitTaskShouldNackLastConsumed(true, true));
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldNackLastConsumed(false, true));
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldNackLastConsumed(true, false));
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldNackLastConsumed(false, false));
}

TEST(RabbitMqTaskBusAlgorithmsTest, RoutesToDeadLetterAfterRetryLimit)
{
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldRouteToDeadLetter(2, 3));
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldRouteToDeadLetter(3, 3));
    EXPECT_TRUE(MemoChatTestRabbitTaskShouldRouteToDeadLetter(4, 3));
}

TEST(RabbitMqTaskBusAlgorithmsTest, KeepsPublishAndAmqpPropertyDecisions)
{
    EXPECT_FALSE(MemoChatTestRabbitTaskShouldRejectPublishResult(true));
    EXPECT_TRUE(MemoChatTestRabbitTaskShouldRejectPublishResult(false));

    EXPECT_EQ(MemoChatTestRabbitTaskPersistentDeliveryMode(), 2);
    EXPECT_STREQ(MemoChatTestRabbitTaskContentTypeJson(), "application/json");
}

TEST(RabbitMqTaskBusAlgorithmsTest, KeepsQueueSuffixesAndErrorLiterals)
{
    EXPECT_STREQ(MemoChatTestRabbitTaskQueueSuffix(), ".q");
    EXPECT_STREQ(MemoChatTestRabbitTaskRetryQueueSuffix(), ".retry");
    EXPECT_STREQ(MemoChatTestRabbitTaskDlqSuffix(), ".dlq");

    EXPECT_STREQ(MemoChatTestRabbitTaskAmqpRpcReplyFailedError(), "amqp_rpc_reply_failed");
    EXPECT_STREQ(MemoChatTestRabbitTaskBuildDisabledError(), "rabbitmq_build_disabled");
    EXPECT_STREQ(MemoChatTestRabbitTaskInvalidConfigError(), "rabbitmq_invalid_config");
    EXPECT_STREQ(MemoChatTestRabbitTaskSocketCreateFailedError(), "rabbitmq_socket_create_failed");
    EXPECT_STREQ(MemoChatTestRabbitTaskSocketOpenFailedError(), "rabbitmq_socket_open_failed");
    EXPECT_STREQ(MemoChatTestRabbitTaskPublishFailedError(), "rabbitmq_publish_failed");
}
