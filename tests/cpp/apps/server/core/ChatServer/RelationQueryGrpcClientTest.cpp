#include <gtest/gtest.h>

#include "ChatRelationInternalGrpcService.hpp"
#include "RelationQueryGrpcClient.hpp"
#include "auth/RelationGrpcAuth.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

namespace
{
std::string TestRoleToken(char role)
{
    return std::string(32, role);
}

RelationGrpcAuthTokens TestQueryTokens()
{
    return RelationGrpcAuthTokens{
        .chat = TestRoleToken('c'),
        .call = TestRoleToken('a'),
        .moments = TestRoleToken('m'),
    };
}

class FakeRelationQueryService final : public IRelationService
{
public:
    bool AreUsersFriends(int uid, int peer_uid) override
    {
        return uid == 42 && peer_uid == 84;
    }

    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override
    {
        out["bootstrap_uid"] = uid;
        out["friend_list"] = memochat::json::array_t{};
        out["friend_list"].append("friend-a");
    }

    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override
    {
        out["dialog_uid"] = uid;
        out["dialogs"] = memochat::json::array_t{};
        out["dialogs"].append("dialog-a");
    }

    RelationCommandResult SearchUser(const RelationCommandRequest&) override
    {
        return {ID_SEARCH_USER_RSP, R"({"error":0})"};
    }

    RelationCommandResult AddFriendApply(const RelationCommandRequest&) override
    {
        return {ID_ADD_FRIEND_RSP, R"({"error":0})"};
    }

    RelationCommandResult AuthFriendApply(const RelationCommandRequest&) override
    {
        return {ID_AUTH_FRIEND_RSP, R"({"error":0})"};
    }

    RelationCommandResult DeleteFriend(const RelationCommandRequest&) override
    {
        return {ID_DELETE_FRIEND_RSP, R"({"error":0})"};
    }

    RelationCommandResult GetDialogList(const RelationCommandRequest&) override
    {
        return {ID_GET_DIALOG_LIST_RSP, R"({"error":0,"dialogs":[]})"};
    }

    RelationCommandResult SyncDraft(const RelationCommandRequest&) override
    {
        return {ID_SYNC_DRAFT_RSP, R"({"error":0})"};
    }

    RelationCommandResult PinDialog(const RelationCommandRequest&) override
    {
        return {ID_PIN_DIALOG_RSP, R"({"error":0})"};
    }

    RelationCommandResult FilterFriendUids(const RelationCommandRequest&) override
    {
        return {0, R"({"error":0,"friend_uids":[]})"};
    }
};

class FailingRelationInternalGrpcService final : public chatinternal::ChatRelationInternalService::Service
{
public:
    grpc::Status AppendRelationBootstrap(grpc::ServerContext*,
                                         const chatinternal::BootstrapRequest*,
                                         chatinternal::BootstrapResponse*) override
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "relation service unavailable");
    }

    grpc::Status BuildDialogList(grpc::ServerContext*,
                                 const chatinternal::BootstrapRequest*,
                                 chatinternal::BootstrapResponse*) override
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "relation service unavailable");
    }
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
} // namespace

TEST(RelationQueryGrpcClientTest, AppendRelationBootstrapMergesRemotePayload)
{
    FakeRelationQueryService fake;
    ChatRelationInternalGrpcService service(&fake, RelationGrpcAccessMode::QueryOnly, TestQueryTokens());
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    RelationQueryGrpcClient client(running.Endpoint(), TestRoleToken('c'), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;
    out["existing"] = "keep";

    client.AppendRelationBootstrapJson(42, out);

    EXPECT_EQ(out["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(out["existing"].asString(), "keep");
    EXPECT_EQ(out["uid"].asInt(), 42);
    EXPECT_EQ(out["bootstrap_uid"].asInt(), 42);
    const auto friend_list = out["friend_list"].get<memochat::json::JsonValue>();
    ASSERT_TRUE(friend_list.isArray());
    EXPECT_EQ(friend_list[0].asString(), "friend-a");
    EXPECT_FALSE(out.isMember("relation_query_remote_error"));
    running.server->Shutdown();
}

TEST(RelationQueryGrpcClientTest, BuildDialogListMergesRemotePayload)
{
    FakeRelationQueryService fake;
    ChatRelationInternalGrpcService service(&fake, RelationGrpcAccessMode::QueryOnly, TestQueryTokens());
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    RelationQueryGrpcClient client(running.Endpoint(), TestRoleToken('c'), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;

    client.BuildDialogListJson(7, out);

    EXPECT_EQ(out["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(out["uid"].asInt(), 7);
    EXPECT_EQ(out["dialog_uid"].asInt(), 7);
    const auto dialogs = out["dialogs"].get<memochat::json::JsonValue>();
    ASSERT_TRUE(dialogs.isArray());
    EXPECT_EQ(dialogs[0].asString(), "dialog-a");
    EXPECT_FALSE(out.isMember("relation_query_remote_error"));
    running.server->Shutdown();
}

TEST(RelationQueryGrpcClientTest, QueryOnlyServiceRejectsWrongRoleToken)
{
    FakeRelationQueryService fake;
    ChatRelationInternalGrpcService service(&fake, RelationGrpcAccessMode::QueryOnly, TestQueryTokens());
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);

    RelationQueryGrpcClient client(running.Endpoint(), TestRoleToken('a'), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;
    client.AppendRelationBootstrapJson(42, out);

    EXPECT_EQ(out["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(out["relation_query_remote_status_code"].asInt(), static_cast<int>(grpc::StatusCode::UNAUTHENTICATED));
    running.server->Shutdown();
}

TEST(RelationQueryGrpcClientTest, QueryOnlyServiceEnforcesPerRpcCallerAllowlist)
{
    FakeRelationQueryService fake;
    ChatRelationInternalGrpcService service(&fake, RelationGrpcAccessMode::QueryOnly, TestQueryTokens());
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    auto stub = chatinternal::ChatRelationInternalService::NewStub(
        grpc::CreateChannel(running.Endpoint(), grpc::InsecureChannelCredentials()));

    chatinternal::FriendshipRequest friendship_request;
    friendship_request.set_uid(42);
    friendship_request.set_peer_uid(84);
    chatinternal::FriendshipResponse friendship_response;
    grpc::ClientContext call_context;
    memochat::auth::InjectRelationGrpcAuth(call_context, TestRoleToken('a'));
    EXPECT_TRUE(stub->CheckFriendship(&call_context, friendship_request, &friendship_response).ok());

    chatinternal::BootstrapRequest bootstrap_request;
    bootstrap_request.set_uid(42);
    chatinternal::BootstrapResponse bootstrap_response;
    grpc::ClientContext cross_role_context;
    memochat::auth::InjectRelationGrpcAuth(cross_role_context, TestRoleToken('a'));
    EXPECT_EQ(stub->AppendRelationBootstrap(&cross_role_context, bootstrap_request, &bootstrap_response).error_code(),
              grpc::StatusCode::UNAUTHENTICATED);

    chatinternal::JsonPayloadRequest filter_request;
    filter_request.set_payload_json(R"({"viewer_uid":42,"author_uids":[84]})");
    filter_request.mutable_session()->set_uid(42);
    chatinternal::JsonPayloadResponse filter_response;
    grpc::ClientContext moments_context;
    memochat::auth::InjectRelationGrpcAuth(moments_context, TestRoleToken('m'));
    EXPECT_TRUE(stub->FilterFriendUids(&moments_context, filter_request, &filter_response).ok());

    chatinternal::JsonPayloadResponse restricted_response;
    grpc::ClientContext restricted_context;
    memochat::auth::InjectRelationGrpcAuth(restricted_context, TestRoleToken('c'));
    EXPECT_EQ(stub->SearchUser(&restricted_context, filter_request, &restricted_response).error_code(),
              grpc::StatusCode::PERMISSION_DENIED);
    running.server->Shutdown();
}

TEST(RelationQueryGrpcClientTest, RemoteFailureMarksBusinessError)
{
    FailingRelationInternalGrpcService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    RelationQueryGrpcClient client(running.Endpoint(), TestRoleToken('c'), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;

    client.AppendRelationBootstrapJson(42, out);

    EXPECT_EQ(out["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(out["relation_query_remote_method"].asString(), "AppendRelationBootstrap");
    EXPECT_EQ(out["relation_query_remote_status_code"].asInt(), static_cast<int>(grpc::StatusCode::UNAVAILABLE));
    EXPECT_NE(out["relation_query_remote_error"].asString().find("unavailable"), std::string::npos);
    running.server->Shutdown();
}
