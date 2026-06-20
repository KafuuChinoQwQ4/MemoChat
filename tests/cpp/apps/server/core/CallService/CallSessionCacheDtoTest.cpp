#include <gtest/gtest.h>

#include "CallSessionCacheDto.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::call::CallSessionCacheDto>(
    std::array<std::string_view, 14>{"call_id",
                                     "room_name",
                                     "call_type",
                                     "caller_uid",
                                     "callee_uid",
                                     "state",
                                     "started_at_ms",
                                     "accepted_at_ms",
                                     "ended_at_ms",
                                     "expires_at_ms",
                                     "duration_sec",
                                     "reason",
                                     "trace_id",
                                     "updated_at_ms"}));
#endif

namespace
{

CallSessionInfo MakeSession()
{
    CallSessionInfo session;
    session.call_id = "call-1";
    session.room_name = "room-1";
    session.call_type = "video";
    session.caller_uid = 11;
    session.callee_uid = 22;
    session.state = "ringing";
    session.started_at_ms = 1000;
    session.accepted_at_ms = 2000;
    session.ended_at_ms = 3000;
    session.expires_at_ms = 4000;
    session.duration_sec = 77;
    session.reason = "normal";
    session.trace_id = "trace-1";
    session.updated_at_ms = 5000;
    return session;
}

} // namespace

TEST(CallSessionCacheDtoTest, EncodesCacheSessionWithExistingWireFieldNames)
{
    std::string body;
    ASSERT_TRUE(memochat::call::EncodeCallSessionCache(MakeSession(), &body));

    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::glaze_parse(root, body));
    EXPECT_EQ(root["call_id"].asString(), "call-1");
    EXPECT_EQ(root["room_name"].asString(), "room-1");
    EXPECT_EQ(root["call_type"].asString(), "video");
    EXPECT_EQ(root["caller_uid"].asInt(), 11);
    EXPECT_EQ(root["callee_uid"].asInt(), 22);
    EXPECT_EQ(root["state"].asString(), "ringing");
    EXPECT_EQ(root["started_at_ms"].asInt64(), 1000);
    EXPECT_EQ(root["accepted_at_ms"].asInt64(), 2000);
    EXPECT_EQ(root["ended_at_ms"].asInt64(), 3000);
    EXPECT_EQ(root["expires_at_ms"].asInt64(), 4000);
    EXPECT_EQ(root["duration_sec"].asInt(), 77);
    EXPECT_EQ(root["reason"].asString(), "normal");
    EXPECT_EQ(root["trace_id"].asString(), "trace-1");
    EXPECT_EQ(root["updated_at_ms"].asInt64(), 5000);
}

TEST(CallSessionCacheDtoTest, DecodesFullCacheSession)
{
    CallSessionInfo session;
    ASSERT_TRUE(memochat::call::DecodeCallSessionCache(
        R"({"call_id":"call-2","room_name":"room-2","call_type":"audio","caller_uid":33,"callee_uid":44,"state":"accepted","started_at_ms":10,"accepted_at_ms":20,"ended_at_ms":30,"expires_at_ms":40,"duration_sec":5,"reason":"ok","trace_id":"trace-2","updated_at_ms":50})",
        &session));

    EXPECT_EQ(session.call_id, "call-2");
    EXPECT_EQ(session.room_name, "room-2");
    EXPECT_EQ(session.call_type, "audio");
    EXPECT_EQ(session.caller_uid, 33);
    EXPECT_EQ(session.callee_uid, 44);
    EXPECT_EQ(session.state, "accepted");
    EXPECT_EQ(session.started_at_ms, 10);
    EXPECT_EQ(session.accepted_at_ms, 20);
    EXPECT_EQ(session.ended_at_ms, 30);
    EXPECT_EQ(session.expires_at_ms, 40);
    EXPECT_EQ(session.duration_sec, 5);
    EXPECT_EQ(session.reason, "ok");
    EXPECT_EQ(session.trace_id, "trace-2");
    EXPECT_EQ(session.updated_at_ms, 50);
}

TEST(CallSessionCacheDtoTest, DecodesMissingOptionalFieldsAsDefaults)
{
    CallSessionInfo session;
    ASSERT_TRUE(memochat::call::DecodeCallSessionCache(R"({"call_id":"call-3"})", &session));

    EXPECT_EQ(session.call_id, "call-3");
    EXPECT_TRUE(session.room_name.empty());
    EXPECT_TRUE(session.call_type.empty());
    EXPECT_EQ(session.caller_uid, 0);
    EXPECT_EQ(session.callee_uid, 0);
    EXPECT_TRUE(session.state.empty());
    EXPECT_EQ(session.started_at_ms, 0);
    EXPECT_EQ(session.accepted_at_ms, 0);
    EXPECT_EQ(session.ended_at_ms, 0);
    EXPECT_EQ(session.expires_at_ms, 0);
    EXPECT_EQ(session.duration_sec, 0);
    EXPECT_TRUE(session.reason.empty());
    EXPECT_TRUE(session.trace_id.empty());
    EXPECT_EQ(session.updated_at_ms, 0);
}

TEST(CallSessionCacheDtoTest, RejectsInvalidCacheSession)
{
    CallSessionInfo session;

    EXPECT_FALSE(memochat::call::DecodeCallSessionCache("not-json", &session));
    EXPECT_FALSE(memochat::call::DecodeCallSessionCache(R"({"call_id":""})", &session));
    EXPECT_FALSE(memochat::call::DecodeCallSessionCache(R"({"room_name":"room-only"})", &session));
    EXPECT_FALSE(memochat::call::DecodeCallSessionCache(R"({"call_id":"call-4"})",
                                                        static_cast<CallSessionInfo*>(nullptr)));
}

TEST(CallSessionCacheDtoTest, ReportsNullEncodeOutput)
{
    std::string error;

    EXPECT_FALSE(memochat::call::EncodeCallSessionCache(MakeSession(), nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");
}
