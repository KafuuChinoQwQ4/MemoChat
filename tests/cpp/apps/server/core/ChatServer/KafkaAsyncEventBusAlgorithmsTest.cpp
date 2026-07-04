#include <gtest/gtest.h>

bool MemoChatTestKafkaEventShouldStartFromValidConfig(bool config_valid);
bool MemoChatTestKafkaEventShouldSkipOutboxRepairPublish(bool publish_callback_missing);
bool MemoChatTestKafkaEventShouldRejectPublish(bool producer_missing, bool config_invalid);
bool MemoChatTestKafkaEventShouldAssignGeneratedEventId(bool event_id_empty);
bool MemoChatTestKafkaEventShouldRejectSerializedPayload(bool serialized_empty);
bool MemoChatTestKafkaEventShouldSetParseError(bool parsed, bool error_sink_present);
bool MemoChatTestKafkaEventShouldCommitLastConsumed(bool consumer_present, bool last_message_present);
bool MemoChatTestKafkaEventShouldNackLastConsumed(bool consumer_present, bool last_message_present);
bool MemoChatTestKafkaEventShouldRouteToDlq(int retry_count, int consume_retry_max);
int MemoChatTestKafkaEventInitialOutboxStatus();
int MemoChatTestKafkaEventInitialRetryCount();
const char* MemoChatTestKafkaEventKafkaNotConfiguredError();
const char* MemoChatTestKafkaEventSerializeFailedError();
const char* MemoChatTestKafkaEventOutboxEnqueueFailedError();
const char* MemoChatTestKafkaEventKafkaBuildDisabledError();
const char* MemoChatTestKafkaEventParseFailedError();
const char* MemoChatTestKafkaEventProducerUnavailableError();

TEST(KafkaAsyncEventBusAlgorithmsTest, KeepsStartupAndPublishGuards)
{
    EXPECT_TRUE(MemoChatTestKafkaEventShouldStartFromValidConfig(true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldStartFromValidConfig(false));
    EXPECT_TRUE(MemoChatTestKafkaEventShouldSkipOutboxRepairPublish(true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldSkipOutboxRepairPublish(false));

    EXPECT_FALSE(MemoChatTestKafkaEventShouldRejectPublish(false, false));
    EXPECT_TRUE(MemoChatTestKafkaEventShouldRejectPublish(true, false));
    EXPECT_TRUE(MemoChatTestKafkaEventShouldRejectPublish(false, true));
    EXPECT_TRUE(MemoChatTestKafkaEventShouldRejectPublish(true, true));
}

TEST(KafkaAsyncEventBusAlgorithmsTest, KeepsPayloadAndParseGuards)
{
    EXPECT_TRUE(MemoChatTestKafkaEventShouldAssignGeneratedEventId(true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldAssignGeneratedEventId(false));
    EXPECT_TRUE(MemoChatTestKafkaEventShouldRejectSerializedPayload(true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldRejectSerializedPayload(false));

    EXPECT_TRUE(MemoChatTestKafkaEventShouldSetParseError(false, true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldSetParseError(false, false));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldSetParseError(true, true));
}

TEST(KafkaAsyncEventBusAlgorithmsTest, RequiresConsumerAndLastMessageForAckAndNack)
{
    EXPECT_TRUE(MemoChatTestKafkaEventShouldCommitLastConsumed(true, true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldCommitLastConsumed(false, true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldCommitLastConsumed(true, false));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldCommitLastConsumed(false, false));

    EXPECT_TRUE(MemoChatTestKafkaEventShouldNackLastConsumed(true, true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldNackLastConsumed(false, true));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldNackLastConsumed(true, false));
    EXPECT_FALSE(MemoChatTestKafkaEventShouldNackLastConsumed(false, false));
}

TEST(KafkaAsyncEventBusAlgorithmsTest, RoutesToDlqAtRetryLimit)
{
    EXPECT_FALSE(MemoChatTestKafkaEventShouldRouteToDlq(2, 3));
    EXPECT_TRUE(MemoChatTestKafkaEventShouldRouteToDlq(3, 3));
    EXPECT_TRUE(MemoChatTestKafkaEventShouldRouteToDlq(4, 3));
}

TEST(KafkaAsyncEventBusAlgorithmsTest, KeepsOutboxDefaultsAndErrorLiterals)
{
    EXPECT_EQ(MemoChatTestKafkaEventInitialOutboxStatus(), 0);
    EXPECT_EQ(MemoChatTestKafkaEventInitialRetryCount(), 0);
    EXPECT_STREQ(MemoChatTestKafkaEventKafkaNotConfiguredError(), "kafka_not_configured");
    EXPECT_STREQ(MemoChatTestKafkaEventSerializeFailedError(), "serialize_failed");
    EXPECT_STREQ(MemoChatTestKafkaEventOutboxEnqueueFailedError(), "outbox_enqueue_failed");
    EXPECT_STREQ(MemoChatTestKafkaEventKafkaBuildDisabledError(), "kafka_build_disabled");
    EXPECT_STREQ(MemoChatTestKafkaEventParseFailedError(), "parse_failed");
    EXPECT_STREQ(MemoChatTestKafkaEventProducerUnavailableError(), "producer_unavailable");
}
