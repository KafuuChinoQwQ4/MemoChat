#include <gtest/gtest.h>

#include "CallPublicDtos.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::call::CallAuthRequestDto>(
    std::array<std::string_view, 3>{"uid", "token", "call_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::call::CallStartRequestDto>(
    std::array<std::string_view, 4>{"uid", "token", "peer_uid", "call_type"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::call::CallTokenRequestDto>(
    std::array<std::string_view, 4>{"uid", "token", "call_id", "role"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::call::CallEventResponseDto>(
    std::array<std::string_view, 19>{"error",
                                     "event_type",
                                     "call_id",
                                     "room_name",
                                     "call_type",
                                     "caller_uid",
                                     "callee_uid",
                                     "caller_name",
                                     "callee_name",
                                     "caller_icon",
                                     "callee_icon",
                                     "started_at",
                                     "accepted_at",
                                     "ended_at",
                                     "expires_at",
                                     "state",
                                     "reason",
                                     "trace_id",
                                     "livekit_url"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::call::CallStartResponseDto>(
    std::array<std::string_view, 22>{"error",       "event_type", "call_id",     "room_name",   "call_type",
                                     "caller_uid",  "callee_uid", "caller_name", "callee_name", "caller_icon",
                                     "callee_icon", "started_at", "accepted_at", "ended_at",    "expires_at",
                                     "state",       "reason",     "trace_id",    "livekit_url", "peer_uid",
                                     "peer_name",   "peer_icon"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::call::CallTokenResponseDto>(
    std::array<std::string_view, 7>{"error", "call_id", "room_name", "role", "livekit_url", "token", "trace_id"}));
#endif

TEST(CallPublicDtosTest, DecodesStartRequestWithExistingWireFieldNames)
{
    memochat::call::CallStartRequestDto request;
    ASSERT_TRUE(
        memochat::call::DecodeCallStartRequest(R"({"uid":42,"token":"token-1","peer_uid":77,"call_type":"video"})",
                                               &request));

    EXPECT_EQ(request.uid, 42);
    EXPECT_EQ(request.token, "token-1");
    EXPECT_EQ(request.peer_uid, 77);
    EXPECT_EQ(request.call_type, "video");
}

TEST(CallPublicDtosTest, KeepsLegacyStartDefaultsForWrongTypes)
{
    memochat::call::CallStartRequestDto request;
    ASSERT_TRUE(memochat::call::DecodeCallStartRequest(R"({"uid":"bad","token":7,"peer_uid":"bad","call_type":false})",
                                                       &request));

    EXPECT_EQ(request.uid, 0);
    EXPECT_TRUE(request.token.empty());
    EXPECT_EQ(request.peer_uid, 0);
    EXPECT_TRUE(request.call_type.empty());
}

TEST(CallPublicDtosTest, DecodesAuthRequestWithExistingWireFieldNames)
{
    memochat::call::CallAuthRequestDto request;
    ASSERT_TRUE(memochat::call::DecodeCallAuthRequest(R"({"uid":42,"token":"token-1","call_id":"call-1"})", &request));

    EXPECT_EQ(request.uid, 42);
    EXPECT_EQ(request.token, "token-1");
    EXPECT_EQ(request.call_id, "call-1");
}

TEST(CallPublicDtosTest, DefaultsAuthRequestMissingFields)
{
    memochat::call::CallAuthRequestDto request;
    ASSERT_TRUE(memochat::call::DecodeCallAuthRequest(R"({})", &request));

    EXPECT_EQ(request.uid, 0);
    EXPECT_TRUE(request.token.empty());
    EXPECT_TRUE(request.call_id.empty());
}

TEST(CallPublicDtosTest, DecodesTokenRequestWithExistingWireFieldNames)
{
    memochat::call::CallTokenRequestDto request;
    ASSERT_TRUE(
        memochat::call::DecodeCallTokenRequest(R"({"uid":42,"token":"token-1","call_id":"call-1","role":"caller"})",
                                               &request));

    EXPECT_EQ(request.uid, 42);
    EXPECT_EQ(request.token, "token-1");
    EXPECT_EQ(request.call_id, "call-1");
    EXPECT_EQ(request.role, "caller");
}

TEST(CallPublicDtosTest, DefaultsTokenRoleToEmptyString)
{
    memochat::call::CallTokenRequestDto request;
    ASSERT_TRUE(memochat::call::DecodeCallTokenRequest(R"({"uid":42,"token":"token-1","call_id":"call-1"})", &request));

    EXPECT_EQ(request.uid, 42);
    EXPECT_EQ(request.token, "token-1");
    EXPECT_EQ(request.call_id, "call-1");
    EXPECT_TRUE(request.role.empty());
}

TEST(CallPublicDtosTest, RejectsMalformedJsonAndNullOutputs)
{
    memochat::call::CallAuthRequestDto auth;
    EXPECT_FALSE(memochat::call::DecodeCallAuthRequest("not-json", &auth));
    EXPECT_FALSE(
        memochat::call::DecodeCallAuthRequest(R"({})", static_cast<memochat::call::CallAuthRequestDto*>(nullptr)));

    memochat::call::CallStartRequestDto start;
    EXPECT_FALSE(memochat::call::DecodeCallStartRequest("not-json", &start));
    EXPECT_FALSE(
        memochat::call::DecodeCallStartRequest(R"({})", static_cast<memochat::call::CallStartRequestDto*>(nullptr)));

    memochat::call::CallTokenRequestDto token;
    EXPECT_FALSE(memochat::call::DecodeCallTokenRequest("not-json", &token));
    EXPECT_FALSE(
        memochat::call::DecodeCallTokenRequest(R"({})", static_cast<memochat::call::CallTokenRequestDto*>(nullptr)));
}

TEST(CallPublicDtosTest, WritesEventResponseWithoutStartOnlyPeerFields)
{
    memochat::call::CallEventResponseDto response;
    response.error = 0;
    response.event_type = "call.accepted";
    response.call_id = "call-1";
    response.room_name = "memochat-room";
    response.call_type = "video";
    response.caller_uid = 42;
    response.callee_uid = 77;
    response.caller_name = "caller";
    response.callee_name = "callee";
    response.caller_icon = "caller.png";
    response.callee_icon = "callee.png";
    response.started_at = 1000;
    response.accepted_at = 2000;
    response.ended_at = 0;
    response.expires_at = 3000;
    response.state = "accepted";
    response.reason = "";
    response.trace_id = "trace-1";
    response.livekit_url = "ws://livekit";

    const memochat::json::JsonValue root = memochat::call::CallEventResponseToJsonValue(response);

    EXPECT_EQ(root["error"].asInt(), 0);
    EXPECT_EQ(root["event_type"].asString(), "call.accepted");
    EXPECT_EQ(root["call_id"].asString(), "call-1");
    EXPECT_EQ(root["room_name"].asString(), "memochat-room");
    EXPECT_EQ(root["call_type"].asString(), "video");
    EXPECT_EQ(root["caller_uid"].asInt(), 42);
    EXPECT_EQ(root["callee_uid"].asInt(), 77);
    EXPECT_EQ(root["caller_name"].asString(), "caller");
    EXPECT_EQ(root["callee_name"].asString(), "callee");
    EXPECT_EQ(root["caller_icon"].asString(), "caller.png");
    EXPECT_EQ(root["callee_icon"].asString(), "callee.png");
    EXPECT_EQ(root["started_at"].asInt64(), 1000);
    EXPECT_EQ(root["accepted_at"].asInt64(), 2000);
    EXPECT_EQ(root["ended_at"].asInt64(), 0);
    EXPECT_EQ(root["expires_at"].asInt64(), 3000);
    EXPECT_EQ(root["state"].asString(), "accepted");
    EXPECT_TRUE(root["reason"].asString().empty());
    EXPECT_EQ(root["trace_id"].asString(), "trace-1");
    EXPECT_EQ(root["livekit_url"].asString(), "ws://livekit");
    EXPECT_FALSE(root.isMember("peer_uid"));
    EXPECT_FALSE(root.isMember("peer_name"));
    EXPECT_FALSE(root.isMember("peer_icon"));
}

TEST(CallPublicDtosTest, WritesStartResponseWithPeerFields)
{
    memochat::call::CallStartResponseDto response;
    response.error = 0;
    response.event_type = "call.state_sync";
    response.call_id = "call-1";
    response.room_name = "memochat-room";
    response.call_type = "voice";
    response.caller_uid = 42;
    response.callee_uid = 77;
    response.caller_name = "caller";
    response.callee_name = "callee";
    response.caller_icon = "caller.png";
    response.callee_icon = "callee.png";
    response.started_at = 1000;
    response.expires_at = 3000;
    response.state = "ringing";
    response.trace_id = "trace-1";
    response.livekit_url = "ws://livekit";
    response.peer_uid = 77;
    response.peer_name = "callee";
    response.peer_icon = "callee.png";

    const memochat::json::JsonValue root = memochat::call::CallStartResponseToJsonValue(response);

    EXPECT_EQ(root["error"].asInt(), 0);
    EXPECT_EQ(root["event_type"].asString(), "call.state_sync");
    EXPECT_EQ(root["call_id"].asString(), "call-1");
    EXPECT_EQ(root["room_name"].asString(), "memochat-room");
    EXPECT_EQ(root["call_type"].asString(), "voice");
    EXPECT_EQ(root["peer_uid"].asInt(), 77);
    EXPECT_EQ(root["peer_name"].asString(), "callee");
    EXPECT_EQ(root["peer_icon"].asString(), "callee.png");
}

TEST(CallPublicDtosTest, WritesTokenResponseWithExistingWireFields)
{
    memochat::call::CallTokenResponseDto response;
    response.error = 0;
    response.call_id = "call-1";
    response.room_name = "memochat-room";
    response.role = "caller";
    response.livekit_url = "ws://livekit";
    response.token = "jwt-token";
    response.trace_id = "trace-1";

    const memochat::json::JsonValue root = memochat::call::CallTokenResponseToJsonValue(response);

    EXPECT_EQ(root["error"].asInt(), 0);
    EXPECT_EQ(root["call_id"].asString(), "call-1");
    EXPECT_EQ(root["room_name"].asString(), "memochat-room");
    EXPECT_EQ(root["role"].asString(), "caller");
    EXPECT_EQ(root["livekit_url"].asString(), "ws://livekit");
    EXPECT_EQ(root["token"].asString(), "jwt-token");
    EXPECT_EQ(root["trace_id"].asString(), "trace-1");
}
