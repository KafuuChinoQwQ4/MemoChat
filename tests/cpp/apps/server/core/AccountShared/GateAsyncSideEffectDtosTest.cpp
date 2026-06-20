#include <gtest/gtest.h>

#include "GateAsyncSideEffectDtos.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<gateasync::GateUserProfileChangedPayloadDto>(
    std::array<std::string_view, 7>{"uid", "user_id", "email", "name", "nick", "icon", "sex"}));
static_assert(memochat::reflection::FieldNamesEqual<gateasync::GateLoginAuditPayloadDto>(
    std::array<std::string_view, 7>{
        "uid", "user_id", "email", "chat_server", "chat_host", "chat_port", "login_cache_hit"}));
static_assert(memochat::reflection::FieldNamesEqual<gateasync::GateCacheInvalidatePayloadDto>(
    std::array<std::string_view, 4>{"email", "user_name", "reason", "cache_domain"}));
static_assert(memochat::reflection::FieldNamesEqual<gateasync::GateKafkaEventEnvelopeDto>(
    std::array<std::string_view, 9>{"event_id",
                                    "topic",
                                    "partition_key",
                                    "event_type",
                                    "trace_id",
                                    "request_id",
                                    "created_at_ms",
                                    "retry_count",
                                    "payload"}));
static_assert(memochat::reflection::FieldNamesEqual<gateasync::GateRabbitTaskEnvelopeDto>(
    std::array<std::string_view, 9>{"task_id",
                                    "task_type",
                                    "trace_id",
                                    "request_id",
                                    "created_at_ms",
                                    "retry_count",
                                    "max_retries",
                                    "routing_key",
                                    "payload"}));
#endif

namespace
{

memochat::json::JsonValue MakePayload()
{
    memochat::json::JsonValue payload(memochat::json::object_t{});
    payload["email"] = "alice@example.com";
    payload["kind"] = "test";
    return payload;
}

} // namespace

TEST(GateAsyncSideEffectDtosTest, BuildsUserProfilePayloadWithExistingWireFieldNames)
{
    const auto payload =
        gateasync::ToJsonValue(gateasync::BuildUserProfileChangedPayload(42,
                                                                         "u-42",
                                                                         "alice@example.com",
                                                                         "alice",
                                                                         "Alice",
                                                                         "icon.png",
                                                                         2));

    EXPECT_EQ(payload["uid"].asInt(), 42);
    EXPECT_EQ(payload["user_id"].asString(), "u-42");
    EXPECT_EQ(payload["email"].asString(), "alice@example.com");
    EXPECT_EQ(payload["name"].asString(), "alice");
    EXPECT_EQ(payload["nick"].asString(), "Alice");
    EXPECT_EQ(payload["icon"].asString(), "icon.png");
    EXPECT_EQ(payload["sex"].asInt(), 2);
}

TEST(GateAsyncSideEffectDtosTest, BuildsLoginAuditPayloadWithExistingWireFieldNames)
{
    const auto payload = gateasync::ToJsonValue(gateasync::BuildLoginAuditPayload(42,
                                                                                  "u-42",
                                                                                  "alice@example.com",
                                                                                  "chat-1",
                                                                                  "127.0.0.1",
                                                                                  "50055",
                                                                                  true));

    EXPECT_EQ(payload["uid"].asInt(), 42);
    EXPECT_EQ(payload["user_id"].asString(), "u-42");
    EXPECT_EQ(payload["email"].asString(), "alice@example.com");
    EXPECT_EQ(payload["chat_server"].asString(), "chat-1");
    EXPECT_EQ(payload["chat_host"].asString(), "127.0.0.1");
    EXPECT_EQ(payload["chat_port"].asString(), "50055");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(payload, "login_cache_hit", false));
}

TEST(GateAsyncSideEffectDtosTest, BuildsAndDecodesCacheInvalidatePayload)
{
    const auto dto = gateasync::BuildCacheInvalidatePayload("alice@example.com", "alice", "reset_pwd");
    const auto payload = gateasync::ToJsonValue(dto);

    EXPECT_EQ(payload["email"].asString(), "alice@example.com");
    EXPECT_EQ(payload["user_name"].asString(), "alice");
    EXPECT_EQ(payload["reason"].asString(), "reset_pwd");
    EXPECT_EQ(payload["cache_domain"].asString(), "login_profile");

    gateasync::GateCacheInvalidatePayloadDto decoded;
    ASSERT_TRUE(gateasync::DecodeCacheInvalidatePayload(payload, &decoded));
    EXPECT_EQ(decoded.email, "alice@example.com");
    EXPECT_EQ(decoded.user_name, "alice");
    EXPECT_EQ(decoded.reason, "reset_pwd");
    EXPECT_EQ(decoded.cache_domain, "login_profile");
}

TEST(GateAsyncSideEffectDtosTest, RejectsInvalidCacheInvalidatePayload)
{
    gateasync::GateCacheInvalidatePayloadDto decoded;

    EXPECT_FALSE(gateasync::DecodeCacheInvalidatePayload(std::string_view("not-json"), &decoded));
    EXPECT_FALSE(gateasync::DecodeCacheInvalidatePayload(std::string_view(R"({"reason":"missing-email"})"), &decoded));
    EXPECT_FALSE(gateasync::DecodeCacheInvalidatePayload(std::string_view(R"({"email":""})"), &decoded));
    EXPECT_FALSE(gateasync::DecodeCacheInvalidatePayload(std::string_view(R"({"email":"alice@example.com"})"),
                                                         static_cast<gateasync::GateCacheInvalidatePayloadDto*>(
                                                             nullptr)));
}

TEST(GateAsyncSideEffectDtosTest, EncodesKafkaEventEnvelopeWithDynamicPayload)
{
    std::string body;
    ASSERT_TRUE(gateasync::EncodeGateKafkaEventEnvelope(gateasync::BuildKafkaEventEnvelope("event-1",
                                                                                          "audit.login.v1",
                                                                                          "42",
                                                                                          "gate_login_succeeded",
                                                                                          "trace-1",
                                                                                          "request-1",
                                                                                          1000,
                                                                                          0,
                                                                                          MakePayload()),
                                                        &body));

    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::glaze_parse(root, body));
    EXPECT_EQ(root["event_id"].asString(), "event-1");
    EXPECT_EQ(root["topic"].asString(), "audit.login.v1");
    EXPECT_EQ(root["partition_key"].asString(), "42");
    EXPECT_EQ(root["event_type"].asString(), "gate_login_succeeded");
    EXPECT_EQ(root["trace_id"].asString(), "trace-1");
    EXPECT_EQ(root["request_id"].asString(), "request-1");
    EXPECT_EQ(root["created_at_ms"].asInt64(), 1000);
    EXPECT_EQ(root["retry_count"].asInt(), 0);
    const memochat::json::JsonValue nested_payload = memochat::json::glaze_get(root, "payload");
    EXPECT_EQ(nested_payload["email"].asString(), "alice@example.com");
    EXPECT_EQ(nested_payload["kind"].asString(), "test");
}

TEST(GateAsyncSideEffectDtosTest, EncodesRabbitTaskEnvelopeWithDynamicPayload)
{
    std::string body;
    ASSERT_TRUE(gateasync::EncodeGateRabbitTaskEnvelope(gateasync::BuildRabbitTaskEnvelope("task-1",
                                                                                          "gate_cache_invalidate",
                                                                                          "trace-1",
                                                                                          "request-1",
                                                                                          1000,
                                                                                          0,
                                                                                          5,
                                                                                          "gate.cache.invalidate",
                                                                                          MakePayload()),
                                                        &body));

    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::glaze_parse(root, body));
    EXPECT_EQ(root["task_id"].asString(), "task-1");
    EXPECT_EQ(root["task_type"].asString(), "gate_cache_invalidate");
    EXPECT_EQ(root["trace_id"].asString(), "trace-1");
    EXPECT_EQ(root["request_id"].asString(), "request-1");
    EXPECT_EQ(root["created_at_ms"].asInt64(), 1000);
    EXPECT_EQ(root["retry_count"].asInt(), 0);
    EXPECT_EQ(root["max_retries"].asInt(), 5);
    EXPECT_EQ(root["routing_key"].asString(), "gate.cache.invalidate");
    const memochat::json::JsonValue nested_payload = memochat::json::glaze_get(root, "payload");
    EXPECT_EQ(nested_payload["email"].asString(), "alice@example.com");
    EXPECT_EQ(nested_payload["kind"].asString(), "test");
}

TEST(GateAsyncSideEffectDtosTest, ReportsNullEncodeOutput)
{
    std::string error;

    EXPECT_FALSE(gateasync::EncodeGateKafkaEventEnvelope(gateasync::BuildKafkaEventEnvelope("event-1",
                                                                                           "topic",
                                                                                           "key",
                                                                                           "type",
                                                                                           "trace",
                                                                                           "request",
                                                                                           1,
                                                                                           0,
                                                                                           MakePayload()),
                                                         nullptr,
                                                         &error));
    EXPECT_EQ(error, "output pointer is null");

    error.clear();
    EXPECT_FALSE(gateasync::EncodeGateRabbitTaskEnvelope(gateasync::BuildRabbitTaskEnvelope("task-1",
                                                                                           "task",
                                                                                           "trace",
                                                                                           "request",
                                                                                           1,
                                                                                           0,
                                                                                           5,
                                                                                           "routing",
                                                                                           MakePayload()),
                                                         nullptr,
                                                         &error));
    EXPECT_EQ(error, "output pointer is null");
}
