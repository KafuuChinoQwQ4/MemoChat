#include <gtest/gtest.h>

#include "delivery/MessageDeliveryTaskPayload.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::delivery::MessageDeliveryTaskPayload>(
    std::array<std::string_view, 5>{"recipient_uid", "msgid", "exclude_uid", "reason", "payload"}));
#endif

namespace
{

memochat::json::JsonValue MakeMessagePayload()
{
    memochat::json::JsonValue payload(memochat::json::object_t{});
    payload["fromuid"] = 10;
    payload["touid"] = 20;
    payload["content"] = "retry me";
    return payload;
}

} // namespace

TEST(MessageDeliveryTaskPayloadTest, BuildsTypedTaskPayloadJsonWithDynamicNestedPayload)
{
    const memochat::json::JsonValue task = memochat::chat::delivery::BuildDeliveryTaskPayloadJson(
        42, 1001, MakeMessagePayload(), 7, "rpc_failed");

    ASSERT_TRUE(task.isObject());
    EXPECT_EQ(task["recipient_uid"].asInt(), 42);
    EXPECT_EQ(task["msgid"].asInt(), 1001);
    EXPECT_EQ(task["exclude_uid"].asInt(), 7);
    EXPECT_EQ(task["reason"].asString(), "rpc_failed");
    ASSERT_TRUE(task["payload"].isObject());
    EXPECT_EQ(task["payload"]["content"].asString(), "retry me");
}

TEST(MessageDeliveryTaskPayloadTest, ParsesTypedTaskPayloadJson)
{
    const memochat::json::JsonValue task = memochat::chat::delivery::BuildDeliveryTaskPayloadJson(
        42, 1001, MakeMessagePayload(), 7, "offline");

    memochat::chat::delivery::MessageDeliveryTaskPayload parsed;
    ASSERT_TRUE(memochat::chat::delivery::ParseDeliveryTaskPayload(task, &parsed));

    EXPECT_EQ(parsed.recipient_uid, 42);
    EXPECT_EQ(parsed.msgid, 1001);
    EXPECT_EQ(parsed.exclude_uid, 7);
    EXPECT_EQ(parsed.reason, "offline");
    EXPECT_EQ(parsed.payload["fromuid"].asInt(), 10);
    EXPECT_EQ(parsed.payload["content"].asString(), "retry me");
}

TEST(MessageDeliveryTaskPayloadTest, OmitsEmptyReasonForRelationNotifyCompatibility)
{
    const memochat::json::JsonValue task =
        memochat::chat::delivery::BuildDeliveryTaskPayloadJson(42, 1001, MakeMessagePayload(), 0, "");

    ASSERT_TRUE(task.isObject());
    EXPECT_EQ(task["recipient_uid"].asInt(), 42);
    EXPECT_EQ(task["msgid"].asInt(), 1001);
    EXPECT_EQ(task["exclude_uid"].asInt(), 0);
    EXPECT_FALSE(task.isMember("reason")) << task.toStyledString();
    ASSERT_TRUE(task["payload"].isObject());

    memochat::chat::delivery::MessageDeliveryTaskPayload parsed;
    ASSERT_TRUE(memochat::chat::delivery::ParseDeliveryTaskPayload(task, &parsed));
    EXPECT_EQ(parsed.recipient_uid, 42);
    EXPECT_EQ(parsed.msgid, 1001);
    EXPECT_EQ(parsed.exclude_uid, 0);
    EXPECT_TRUE(parsed.reason.empty());
    EXPECT_EQ(parsed.payload["content"].asString(), "retry me");
}

TEST(MessageDeliveryTaskPayloadTest, RejectsIncompleteOrInvalidTaskPayload)
{
    memochat::chat::delivery::MessageDeliveryTaskPayload parsed;
    memochat::json::JsonValue empty(memochat::json::object_t{});

    EXPECT_FALSE(memochat::chat::delivery::ParseDeliveryTaskPayload(empty, &parsed));
    EXPECT_FALSE(memochat::chat::delivery::ParseDeliveryTaskPayload(MakeMessagePayload(), nullptr));

    memochat::json::JsonValue missing_nested(memochat::json::object_t{});
    missing_nested["recipient_uid"] = 42;
    missing_nested["msgid"] = 1001;
    EXPECT_FALSE(memochat::chat::delivery::ParseDeliveryTaskPayload(missing_nested, &parsed));
}
