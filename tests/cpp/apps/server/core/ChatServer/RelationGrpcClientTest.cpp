#include <gtest/gtest.h>

#include "ChatRelationInternalGrpcService.h"
#include "RelationGrpcClient.h"
#include "const.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

namespace
{
class FakeRelationService final : public IRelationService
{
public:
    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override
    {
        out["bootstrap_uid"] = uid;
        out["friend_list"] = memochat::json::array_t{};
    }

    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override
    {
        out["dialog_uid"] = uid;
        out["dialogs"] = memochat::json::array_t{};
    }

    RelationCommandResult SearchUser(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_SEARCH_USER_RSP, R"({"error":0,"user_id":"alice"})"};
    }

    RelationCommandResult AddFriendApply(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_ADD_FRIEND_RSP, R"({"error":0})"};
    }

    RelationCommandResult AuthFriendApply(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_AUTH_FRIEND_RSP, R"({"error":0})"};
    }

    RelationCommandResult DeleteFriend(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_DELETE_FRIEND_RSP, R"({"error":0})"};
    }

    RelationCommandResult GetDialogList(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_GET_DIALOG_LIST_RSP, R"({"error":0,"dialogs":[]})"};
    }

    RelationCommandResult SyncDraft(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_SYNC_DRAFT_RSP, R"({"error":0})"};
    }

    RelationCommandResult PinDialog(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_PIN_DIALOG_RSP, R"({"error":0})"};
    }

    void HandleSearchUser(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleAddFriendApply(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleAuthFriendApply(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleDeleteFriend(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleGetDialogList(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleSyncDraft(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandlePinDialog(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }

    RelationCommandRequest last_request;
};

struct RunningGrpcServer
{
    int port = 0;
    std::unique_ptr<grpc::Server> server;

    std::string Endpoint() const
    {
        return std::string("127.0.0.1:") + std::to_string(port);
    }
};

RunningGrpcServer StartServer(chatinternal::ChatRelationInternalService::Service* service)
{
    RunningGrpcServer running;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &running.port);
    builder.RegisterService(service);
    running.server = builder.BuildAndStart();
    return running;
}

const char* EnvValue(const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr ? "" : value;
}

void ExpectNoRemoteErrorPayload(const std::string& payload_json)
{
    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(payload_json, payload)) << payload_json;
    EXPECT_FALSE(payload.isMember("relation_remote_error"));
    EXPECT_FALSE(payload.isMember("relation_remote_status_code"));
    EXPECT_FALSE(payload.isMember("relation_remote_error_code"));
}

int EnvIntOrDefault(const char* name, int fallback)
{
    const std::string value = EnvValue(name);
    if (value.empty())
    {
        return fallback;
    }
    return std::stoi(value);
}

bool HasPrivateDialog(const memochat::json::JsonValue& dialogs, int peer_uid)
{
    for (const auto& dialog : dialogs)
    {
        if (dialog.isObject() && dialog["dialog_type"].asString() == "private" &&
            dialog["peer_uid"].asInt() == peer_uid)
        {
            return true;
        }
    }
    return false;
}

bool HasGroupDialog(const memochat::json::JsonValue& dialogs, int64_t group_id)
{
    for (const auto& dialog : dialogs)
    {
        if (dialog.isObject() && dialog["dialog_type"].asString() == "group" &&
            dialog["group_id"].asInt64() == group_id)
        {
            return true;
        }
    }
    return false;
}
} // namespace

TEST(RelationGrpcClientTest, SearchUserRoundTripsCommandPayload)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    RelationGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
    RelationCommandRequest request;
    request.request_msg_id = ID_SEARCH_USER_REQ;
    request.payload_json = R"({"user_id":"alice"})";
    request.session_uid = 42;
    request.session_id = "session-42";
    request.server_name = "ChatServer-1";
    request.trace_id = "trace-command";

    const auto result = client.SearchUser(request);

    EXPECT_EQ(result.response_msg_id, ID_SEARCH_USER_RSP);
    EXPECT_EQ(result.payload_json, R"({"error":0,"user_id":"alice"})");
    EXPECT_EQ(fake.last_request.request_msg_id, ID_SEARCH_USER_REQ);
    EXPECT_EQ(fake.last_request.payload_json, R"({"user_id":"alice"})");
    EXPECT_EQ(fake.last_request.session_uid, 42);
    EXPECT_EQ(fake.last_request.session_id, "session-42");
    EXPECT_EQ(fake.last_request.server_name, "ChatServer-1");
    EXPECT_EQ(fake.last_request.trace_id, "trace-command");
    running.server->Shutdown();
}

TEST(RelationGrpcClientTest, QueryMethodsMergeRemotePayload)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    RelationGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;
    client.AppendRelationBootstrapJson(7, out);

    EXPECT_EQ(out["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(out["uid"].asInt(), 7);
    EXPECT_EQ(out["bootstrap_uid"].asInt(), 7);
    EXPECT_FALSE(out.isMember("relation_remote_error"));
    running.server->Shutdown();
}

TEST(RelationGrpcClientTest, RuntimeSmokeSearchUserUsesRealRelationServiceWorker)
{
    const std::string endpoint = EnvValue("MEMOCHAT_RELATION_SERVICE_SMOKE_ENDPOINT");
    if (endpoint.empty())
    {
        GTEST_SKIP() << "MEMOCHAT_RELATION_SERVICE_SMOKE_ENDPOINT is not set";
    }

    RelationGrpcClient client(endpoint, std::chrono::seconds(2));
    RelationCommandRequest request;
    request.request_msg_id = ID_SEARCH_USER_REQ;
    request.payload_json = R"({})";
    request.session_uid = 1;
    request.session_id = "relation-service-smoke";
    request.server_name = "ChatServer-smoke";
    request.trace_id = "relation-service-smoke";

    const auto result = client.SearchUser(request);

    EXPECT_EQ(result.response_msg_id, ID_SEARCH_USER_RSP);
    ExpectNoRemoteErrorPayload(result.payload_json);

    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(result.payload_json, payload)) << result.payload_json;
    ASSERT_TRUE(payload.isMember("error")) << result.payload_json;
    EXPECT_EQ(payload["error"].asInt(), ErrorCodes::Error_Json);
}

TEST(RelationGrpcClientTest, RuntimeSmokeGetDialogListUsesRealRelationServiceWorker)
{
    const std::string endpoint = EnvValue("MEMOCHAT_RELATION_SERVICE_SMOKE_ENDPOINT");
    if (endpoint.empty())
    {
        GTEST_SKIP() << "MEMOCHAT_RELATION_SERVICE_SMOKE_ENDPOINT is not set";
    }

    const int uid = EnvIntOrDefault("MEMOCHAT_RELATION_SERVICE_SMOKE_UID", 910001);
    const int peer_uid = EnvIntOrDefault("MEMOCHAT_RELATION_SERVICE_SMOKE_PEER_UID", 910002);
    const int64_t group_id = EnvIntOrDefault("MEMOCHAT_RELATION_SERVICE_SMOKE_GROUP_ID", 0);

    RelationGrpcClient client(endpoint, std::chrono::seconds(2));
    RelationCommandRequest request;
    request.request_msg_id = ID_GET_DIALOG_LIST_REQ;
    request.payload_json = std::string(R"({"fromuid":)") + std::to_string(uid) + "}";
    request.session_uid = uid;
    request.session_id = "relation-service-smoke-dialogs";
    request.server_name = "ChatServer-smoke";
    request.trace_id = "relation-service-smoke-dialogs";

    const auto result = client.GetDialogList(request);

    EXPECT_EQ(result.response_msg_id, ID_GET_DIALOG_LIST_RSP);
    ExpectNoRemoteErrorPayload(result.payload_json);

    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(result.payload_json, payload)) << result.payload_json;
    ASSERT_TRUE(payload.isMember("error")) << result.payload_json;
    EXPECT_EQ(payload["error"].asInt(), ErrorCodes::Success) << result.payload_json;
    EXPECT_EQ(payload["uid"].asInt(), uid) << result.payload_json;
    ASSERT_TRUE(payload.isMember("dialogs")) << result.payload_json;
    ASSERT_TRUE(payload["dialogs"].isArray()) << result.payload_json;
    EXPECT_TRUE(HasPrivateDialog(payload["dialogs"], peer_uid)) << result.payload_json;
    if (group_id > 0)
    {
        EXPECT_TRUE(HasGroupDialog(payload["dialogs"], group_id)) << result.payload_json;
    }
}
