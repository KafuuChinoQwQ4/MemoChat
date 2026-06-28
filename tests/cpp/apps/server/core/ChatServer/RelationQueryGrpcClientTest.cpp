#include <gtest/gtest.h>

#include "ChatRelationInternalGrpcService.h"
#include "RelationQueryGrpcClient.h"
#include "const.h"
#include "json/GlazeCompat.h"

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

namespace
{
class FakeRelationQueryService final : public IRelationService
{
public:
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
    ChatRelationInternalGrpcService service(&fake);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    RelationQueryGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
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
    ChatRelationInternalGrpcService service(&fake);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    RelationQueryGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
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

TEST(RelationQueryGrpcClientTest, RemoteFailureMarksBusinessError)
{
    FailingRelationInternalGrpcService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    RelationQueryGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;

    client.AppendRelationBootstrapJson(42, out);

    EXPECT_EQ(out["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(out["relation_query_remote_method"].asString(), "AppendRelationBootstrap");
    EXPECT_EQ(out["relation_query_remote_status_code"].asInt(), static_cast<int>(grpc::StatusCode::UNAVAILABLE));
    EXPECT_NE(out["relation_query_remote_error"].asString().find("unavailable"), std::string::npos);
    running.server->Shutdown();
}
