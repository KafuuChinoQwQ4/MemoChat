#include <gtest/gtest.h>

#include "EmailDeliveryTaskCodec.hpp"

#include "json/GlazeCompat.hpp"

TEST(EmailDeliveryTaskCodecTest, SerializesEmailTaskAsJsonObject)
{
    varifyservice::EmailDeliveryTaskPayload task;
    task.email = "user@example.com";
    task.code = "123456";
    task.trace_id = "trace-1";
    task.retry_count = 2;

    const std::string body = varifyservice::SerializeEmailTask(task);

    EXPECT_EQ(body, R"({"email":"user@example.com","code":"123456","trace_id":"trace-1","retry_count":2})");

    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::reader_parse(body, root));
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "email", ""), "user@example.com");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "code", ""), "123456");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "trace_id", ""), "trace-1");
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(root, "retry_count", -1), 2);
}

TEST(EmailDeliveryTaskCodecTest, ParsesLegacyPayloadWithMissingRetryCountAsZero)
{
    varifyservice::EmailDeliveryTaskPayload parsed;

    ASSERT_TRUE(
        varifyservice::ParseEmailTask(R"({"email":"user@example.com","code":"123456","trace_id":"trace-3"})", &parsed));

    EXPECT_EQ(parsed.email, "user@example.com");
    EXPECT_EQ(parsed.code, "123456");
    EXPECT_EQ(parsed.trace_id, "trace-3");
    EXPECT_EQ(parsed.retry_count, 0);
}

TEST(EmailDeliveryTaskCodecTest, ParsesSerializedEmailTask)
{
    varifyservice::EmailDeliveryTaskPayload original;
    original.email = "user@example.com";
    original.code = "654321";
    original.trace_id = "trace-2";
    original.retry_count = 3;

    varifyservice::EmailDeliveryTaskPayload parsed;
    ASSERT_TRUE(varifyservice::ParseEmailTask(varifyservice::SerializeEmailTask(original), &parsed));

    EXPECT_EQ(parsed.email, original.email);
    EXPECT_EQ(parsed.code, original.code);
    EXPECT_EQ(parsed.trace_id, original.trace_id);
    EXPECT_EQ(parsed.retry_count, original.retry_count);
}

TEST(EmailDeliveryTaskCodecTest, RejectsMalformedOrIncompletePayloads)
{
    varifyservice::EmailDeliveryTaskPayload parsed;

    EXPECT_FALSE(varifyservice::ParseEmailTask("not-json", &parsed));
    EXPECT_FALSE(varifyservice::ParseEmailTask(R"({"email":"user@example.com","retry_count":1})", &parsed));
    EXPECT_FALSE(varifyservice::ParseEmailTask(R"({"code":"123456","retry_count":1})", &parsed));
    EXPECT_FALSE(varifyservice::ParseEmailTask(R"({"email":"","code":"123456","retry_count":1})", &parsed));
}

TEST(EmailDeliveryTaskCodecTest, RejectsNullOutputPointer)
{
    EXPECT_FALSE(varifyservice::ParseEmailTask(R"({"email":"user@example.com","code":"123456"})", nullptr));
}
