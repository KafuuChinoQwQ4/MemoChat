#include "modules/call/CallRouteModule.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{

const std::vector<std::string>& AuthRequestFields()
{
    static const std::vector<std::string> fields = {"uid", "token", "call_id"};
    return fields;
}

const std::vector<std::string>& EventResponseFields()
{
    static const std::vector<std::string> fields = {"error",
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
                                                    "livekit_url"};
    return fields;
}

void ExpectFields(const memochat::gate::routing::RouteBodySchema& schema, const std::vector<std::string>& names)
{
    ASSERT_EQ(schema.fields.size(), names.size());
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        EXPECT_EQ(schema.fields[i].name, names[i]);
    }
}

const char* ExpectedCallRouteSchemaSnapshot()
{
    return "route: call.start\n"
           "method: POST\n"
           "path: /api/call/start\n"
           "request: CallStartRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - peer_uid\n"
           "  - call_type\n"
           "response: CallStartResponseDto\n"
           "  - error\n"
           "  - event_type\n"
           "  - call_id\n"
           "  - room_name\n"
           "  - call_type\n"
           "  - caller_uid\n"
           "  - callee_uid\n"
           "  - caller_name\n"
           "  - callee_name\n"
           "  - caller_icon\n"
           "  - callee_icon\n"
           "  - started_at\n"
           "  - accepted_at\n"
           "  - ended_at\n"
           "  - expires_at\n"
           "  - state\n"
           "  - reason\n"
           "  - trace_id\n"
           "  - livekit_url\n"
           "  - peer_uid\n"
           "  - peer_name\n"
           "  - peer_icon\n"
           "\n"
           "route: call.accept\n"
           "method: POST\n"
           "path: /api/call/accept\n"
           "request: CallAuthRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - call_id\n"
           "response: CallEventResponseDto\n"
           "  - error\n"
           "  - event_type\n"
           "  - call_id\n"
           "  - room_name\n"
           "  - call_type\n"
           "  - caller_uid\n"
           "  - callee_uid\n"
           "  - caller_name\n"
           "  - callee_name\n"
           "  - caller_icon\n"
           "  - callee_icon\n"
           "  - started_at\n"
           "  - accepted_at\n"
           "  - ended_at\n"
           "  - expires_at\n"
           "  - state\n"
           "  - reason\n"
           "  - trace_id\n"
           "  - livekit_url\n"
           "\n"
           "route: call.reject\n"
           "method: POST\n"
           "path: /api/call/reject\n"
           "request: CallAuthRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - call_id\n"
           "response: CallEventResponseDto\n"
           "  - error\n"
           "  - event_type\n"
           "  - call_id\n"
           "  - room_name\n"
           "  - call_type\n"
           "  - caller_uid\n"
           "  - callee_uid\n"
           "  - caller_name\n"
           "  - callee_name\n"
           "  - caller_icon\n"
           "  - callee_icon\n"
           "  - started_at\n"
           "  - accepted_at\n"
           "  - ended_at\n"
           "  - expires_at\n"
           "  - state\n"
           "  - reason\n"
           "  - trace_id\n"
           "  - livekit_url\n"
           "\n"
           "route: call.cancel\n"
           "method: POST\n"
           "path: /api/call/cancel\n"
           "request: CallAuthRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - call_id\n"
           "response: CallEventResponseDto\n"
           "  - error\n"
           "  - event_type\n"
           "  - call_id\n"
           "  - room_name\n"
           "  - call_type\n"
           "  - caller_uid\n"
           "  - callee_uid\n"
           "  - caller_name\n"
           "  - callee_name\n"
           "  - caller_icon\n"
           "  - callee_icon\n"
           "  - started_at\n"
           "  - accepted_at\n"
           "  - ended_at\n"
           "  - expires_at\n"
           "  - state\n"
           "  - reason\n"
           "  - trace_id\n"
           "  - livekit_url\n"
           "\n"
           "route: call.hangup\n"
           "method: POST\n"
           "path: /api/call/hangup\n"
           "request: CallAuthRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - call_id\n"
           "response: CallEventResponseDto\n"
           "  - error\n"
           "  - event_type\n"
           "  - call_id\n"
           "  - room_name\n"
           "  - call_type\n"
           "  - caller_uid\n"
           "  - callee_uid\n"
           "  - caller_name\n"
           "  - callee_name\n"
           "  - caller_icon\n"
           "  - callee_icon\n"
           "  - started_at\n"
           "  - accepted_at\n"
           "  - ended_at\n"
           "  - expires_at\n"
           "  - state\n"
           "  - reason\n"
           "  - trace_id\n"
           "  - livekit_url\n"
           "\n"
           "route: call.token.post\n"
           "method: POST\n"
           "path: /api/call/token\n"
           "request: CallTokenRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - call_id\n"
           "  - role\n"
           "response: CallTokenResponseDto\n"
           "  - error\n"
           "  - call_id\n"
           "  - room_name\n"
           "  - role\n"
           "  - livekit_url\n"
           "  - token\n"
           "  - trace_id\n"
           "\n";
}

} // namespace

TEST(CallRouteSchemaTest, ListsOnlySchemaEligibleCallJsonBodyRoutes)
{
    const auto schemas = memochat::gate::modules::call::CallRouteModule::RouteSchemas();

    ASSERT_EQ(schemas.size(), 6U);
    EXPECT_EQ(schemas[0].name, "call.start");
    EXPECT_EQ(schemas[0].method, "POST");
    EXPECT_EQ(schemas[0].path, "/api/call/start");
    EXPECT_EQ(schemas[0].request.type_name, "CallStartRequestDto");
    EXPECT_EQ(schemas[0].response.type_name, "CallStartResponseDto");

    EXPECT_EQ(schemas[1].name, "call.accept");
    EXPECT_EQ(schemas[1].method, "POST");
    EXPECT_EQ(schemas[1].path, "/api/call/accept");
    EXPECT_EQ(schemas[1].request.type_name, "CallAuthRequestDto");
    EXPECT_EQ(schemas[1].response.type_name, "CallEventResponseDto");

    EXPECT_EQ(schemas[2].name, "call.reject");
    EXPECT_EQ(schemas[2].method, "POST");
    EXPECT_EQ(schemas[2].path, "/api/call/reject");

    EXPECT_EQ(schemas[3].name, "call.cancel");
    EXPECT_EQ(schemas[3].method, "POST");
    EXPECT_EQ(schemas[3].path, "/api/call/cancel");

    EXPECT_EQ(schemas[4].name, "call.hangup");
    EXPECT_EQ(schemas[4].method, "POST");
    EXPECT_EQ(schemas[4].path, "/api/call/hangup");

    EXPECT_EQ(schemas[5].name, "call.token.post");
    EXPECT_EQ(schemas[5].method, "POST");
    EXPECT_EQ(schemas[5].path, "/api/call/token");
    EXPECT_EQ(schemas[5].request.type_name, "CallTokenRequestDto");
    EXPECT_EQ(schemas[5].response.type_name, "CallTokenResponseDto");
}

TEST(CallRouteSchemaTest, BuildsFieldInventoriesFromCallDtos)
{
    const auto schemas = memochat::gate::modules::call::CallRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 6U);

    ExpectFields(schemas[0].request, {"uid", "token", "peer_uid", "call_type"});
    ExpectFields(schemas[0].response,
                 {"error",       "event_type",  "call_id",     "room_name",   "call_type",   "caller_uid",
                  "callee_uid",  "caller_name", "callee_name", "caller_icon", "callee_icon", "started_at",
                  "accepted_at", "ended_at",    "expires_at",  "state",       "reason",      "trace_id",
                  "livekit_url", "peer_uid",    "peer_name",   "peer_icon"});

    for (std::size_t i = 1; i <= 4; ++i)
    {
        ExpectFields(schemas[i].request, AuthRequestFields());
        ExpectFields(schemas[i].response, EventResponseFields());
    }

    ExpectFields(schemas[5].request, {"uid", "token", "call_id", "role"});
    ExpectFields(schemas[5].response, {"error", "call_id", "room_name", "role", "livekit_url", "token", "trace_id"});
}

TEST(CallRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::call::CallRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedCallRouteSchemaSnapshot());
}
