#include <gtest/gtest.h>

#include "ChatAsyncEvent.h"
#include "ChatRuntime.h"
#include "ChatTaskEnvelope.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<TaskEnvelope>(std::array<std::string_view, 10>{"task_id",
                                                                                                   "task_type",
                                                                                                   "trace_id",
                                                                                                   "request_id",
                                                                                                   "created_at_ms",
                                                                                                   "available_at_ms",
                                                                                                   "retry_count",
                                                                                                   "max_retries",
                                                                                                   "routing_key",
                                                                                                   "payload"}));

static_assert(memochat::reflection::FieldNamesEqual<AsyncEventEnvelope>(
    std::array<std::string_view,
               7>{"event_id", "topic", "partition_key", "trace_id", "request_id", "retry_count", "payload"}));
#endif

namespace
{

memochat::json::JsonValue MakeNestedPayload()
{
    memochat::json::JsonValue payload(memochat::json::object_t{});
    payload["fromuid"] = 10;
    payload["touid"] = 20;
    payload["content"] = "hello";
    payload["tags"] = memochat::json::array_t{};
    payload["tags"].append("urgent");
    return payload;
}

} // namespace

TEST(ChatEnvelopeCodecTest, TaskEnvelopeRoundTripUsesTypedJsonAndKeepsDynamicPayload)
{
    TaskEnvelope original;
    original.task_id = "task-1";
    original.task_type = "delivery.retry";
    original.trace_id = "trace-1";
    original.request_id = "request-1";
    original.created_at_ms = 1000;
    original.available_at_ms = 1500;
    original.retry_count = 2;
    original.max_retries = 5;
    original.routing_key = "delivery";
    original.payload = MakeNestedPayload();

    const std::string serialized = SerializeTaskEnvelope(original);

    TaskEnvelope parsed;
    ASSERT_TRUE(ParseTaskEnvelope(serialized, parsed)) << serialized;
    EXPECT_EQ(parsed.task_id, "task-1");
    EXPECT_EQ(parsed.task_type, "delivery.retry");
    EXPECT_EQ(parsed.trace_id, "trace-1");
    EXPECT_EQ(parsed.request_id, "request-1");
    EXPECT_EQ(parsed.created_at_ms, int64_t{1000});
    EXPECT_EQ(parsed.available_at_ms, int64_t{1500});
    EXPECT_EQ(parsed.retry_count, 2);
    EXPECT_EQ(parsed.max_retries, 5);
    EXPECT_EQ(parsed.routing_key, "delivery");
    EXPECT_EQ(parsed.payload["fromuid"].asInt(), 10);
    EXPECT_EQ(parsed.payload["touid"].asInt(), 20);
    EXPECT_EQ(parsed.payload["content"].asString(), "hello");
    const memochat::json::JsonValue tags = static_cast<const memochat::json::JsonValue&>(parsed.payload)["tags"];
    ASSERT_TRUE(tags.isArray());
    ASSERT_EQ(tags.size(), 1U);
    EXPECT_EQ(tags[0].asString(), "urgent");
}

TEST(ChatEnvelopeCodecTest, TaskEnvelopeMissingFieldsKeepDefaults)
{
    TaskEnvelope parsed;

    ASSERT_TRUE(ParseTaskEnvelope(R"({"task_id":"task-min","payload":{"ok":true}})", parsed));

    EXPECT_EQ(parsed.task_id, "task-min");
    EXPECT_TRUE(parsed.task_type.empty());
    EXPECT_TRUE(parsed.trace_id.empty());
    EXPECT_EQ(parsed.created_at_ms, int64_t{0});
    EXPECT_EQ(parsed.available_at_ms, int64_t{0});
    EXPECT_EQ(parsed.retry_count, 0);
    EXPECT_EQ(parsed.max_retries, 0);
    EXPECT_TRUE(parsed.routing_key.empty());
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(parsed.payload, "ok", false));

    TaskEnvelope no_payload;
    ASSERT_TRUE(ParseTaskEnvelope(R"({"task_id":"task-empty"})", no_payload));
    EXPECT_TRUE(no_payload.payload.isObject());
    EXPECT_EQ(no_payload.payload.size(), 0U);
}

TEST(ChatEnvelopeCodecTest, TaskEnvelopeRejectsMalformedJson)
{
    TaskEnvelope parsed;

    EXPECT_FALSE(ParseTaskEnvelope("not-json", parsed));
}

TEST(ChatEnvelopeCodecTest, AsyncEventEnvelopeRoundTripUsesTypedJsonAndKeepsDynamicPayload)
{
    AsyncEventEnvelope original;
    original.event_id = "event-1";
    original.topic = memochat::chatruntime::TopicPrivate();
    original.partition_key = "10:20";
    original.trace_id = "trace-2";
    original.request_id = "request-2";
    original.retry_count = 3;
    original.payload = MakeNestedPayload();

    const std::string serialized = SerializeAsyncEventEnvelope(original);

    AsyncEventEnvelope parsed;
    ASSERT_TRUE(ParseAsyncEventEnvelope(serialized, parsed)) << serialized;
    EXPECT_EQ(parsed.event_id, "event-1");
    EXPECT_EQ(parsed.topic, memochat::chatruntime::TopicPrivate());
    EXPECT_EQ(parsed.partition_key, "10:20");
    EXPECT_EQ(parsed.trace_id, "trace-2");
    EXPECT_EQ(parsed.request_id, "request-2");
    EXPECT_EQ(parsed.retry_count, 3);
    EXPECT_EQ(parsed.payload["fromuid"].asInt(), 10);
    EXPECT_EQ(parsed.payload["touid"].asInt(), 20);
    EXPECT_EQ(parsed.payload["content"].asString(), "hello");
}

TEST(ChatEnvelopeCodecTest, BuildAsyncEventEnvelopeDerivesPrivatePartitionKey)
{
    const AsyncEventEnvelope envelope =
        BuildAsyncEventEnvelope(memochat::chatruntime::TopicPrivate(), MakeNestedPayload());

    EXPECT_EQ(envelope.topic, memochat::chatruntime::TopicPrivate());
    EXPECT_EQ(envelope.partition_key, "10:20");
    EXPECT_EQ(envelope.payload["content"].asString(), "hello");
}

TEST(ChatEnvelopeCodecTest, AsyncEventEnvelopeMissingFieldsKeepDefaults)
{
    AsyncEventEnvelope parsed;

    ASSERT_TRUE(ParseAsyncEventEnvelope(R"({"event_id":"event-min","payload":{"ok":false}})", parsed));

    EXPECT_EQ(parsed.event_id, "event-min");
    EXPECT_TRUE(parsed.topic.empty());
    EXPECT_TRUE(parsed.partition_key.empty());
    EXPECT_TRUE(parsed.trace_id.empty());
    EXPECT_TRUE(parsed.request_id.empty());
    EXPECT_EQ(parsed.retry_count, 0);
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(parsed.payload, "ok", true));

    AsyncEventEnvelope no_payload;
    ASSERT_TRUE(ParseAsyncEventEnvelope(R"({"event_id":"event-empty"})", no_payload));
    EXPECT_TRUE(no_payload.payload.isObject());
    EXPECT_EQ(no_payload.payload.size(), 0U);
}

TEST(ChatEnvelopeCodecTest, AsyncEventEnvelopeRejectsMalformedJson)
{
    AsyncEventEnvelope parsed;

    EXPECT_FALSE(ParseAsyncEventEnvelope("not-json", parsed));
}
