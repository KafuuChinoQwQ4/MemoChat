#include <gtest/gtest.h>

bool MemoChatTestRedisEventShouldRejectSerializedPayload(bool serialized_empty);
bool MemoChatTestRedisEventShouldRequeueLastConsumed(bool last_topic_empty, bool last_serialized_empty);
const char* MemoChatTestRedisEventSerializeFailedError();
const char* MemoChatTestRedisEventQueuePublishFailedError();
const char* MemoChatTestRedisEventParseFailedError();

TEST(RedisAsyncEventBusAlgorithmsTest, RejectsOnlyEmptySerializedPayload)
{
    EXPECT_TRUE(MemoChatTestRedisEventShouldRejectSerializedPayload(true));
    EXPECT_FALSE(MemoChatTestRedisEventShouldRejectSerializedPayload(false));
}

TEST(RedisAsyncEventBusAlgorithmsTest, RequeuesOnlyWhenLastTopicAndPayloadExist)
{
    EXPECT_TRUE(MemoChatTestRedisEventShouldRequeueLastConsumed(false, false));
    EXPECT_FALSE(MemoChatTestRedisEventShouldRequeueLastConsumed(true, false));
    EXPECT_FALSE(MemoChatTestRedisEventShouldRequeueLastConsumed(false, true));
    EXPECT_FALSE(MemoChatTestRedisEventShouldRequeueLastConsumed(true, true));
}

TEST(RedisAsyncEventBusAlgorithmsTest, KeepsStableErrorLiterals)
{
    EXPECT_STREQ(MemoChatTestRedisEventSerializeFailedError(), "serialize_failed");
    EXPECT_STREQ(MemoChatTestRedisEventQueuePublishFailedError(), "queue_publish_failed");
    EXPECT_STREQ(MemoChatTestRedisEventParseFailedError(), "parse_failed");
}
