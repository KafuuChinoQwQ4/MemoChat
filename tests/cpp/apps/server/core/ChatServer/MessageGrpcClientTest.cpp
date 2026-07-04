#include <gtest/gtest.h>

#include "ChatMessageInternalGrpcService.hpp"
#include "MessageGrpcClient.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

namespace
{
class FakePrivateMessageCommandService final : public IPrivateMessageCommandService
{
public:
    MessageCommandResult TextChatMessage(const MessageCommandRequest& request) override
    {
        last_request = request;
        return {ID_TEXT_CHAT_MSG_RSP, R"({"error":0,"private":"roundtrip"})"};
    }

    MessageCommandResult ForwardPrivateMessage(const MessageCommandRequest& request) override
    {
        last_request = request;
        return {ID_FORWARD_PRIVATE_MSG_RSP, R"({"error":0,"private":"forward"})"};
    }

    MessageCommandResult PrivateReadAck(const MessageCommandRequest& request) override
    {
        last_request = request;
        return {0, R"({"error":0,"private":"read_ack"})"};
    }

    MessageCommandResult EditPrivateMessage(const MessageCommandRequest& request) override
    {
        last_request = request;
        return {ID_EDIT_PRIVATE_MSG_RSP, R"({"error":0,"private":"edit"})"};
    }

    MessageCommandResult RevokePrivateMessage(const MessageCommandRequest& request) override
    {
        last_request = request;
        return {ID_REVOKE_PRIVATE_MSG_RSP, R"({"error":0,"private":"revoke"})"};
    }

    MessageCommandResult PrivateHistory(const MessageCommandRequest& request) override
    {
        last_request = request;
        return {ID_PRIVATE_HISTORY_RSP, R"({"error":0,"private":"history"})"};
    }

    MessageCommandRequest last_request;
};

class FakeGroupMessageCommandService final : public IGroupMessageCommandService
{
public:
    void BuildGroupListJson(int uid, memochat::json::JsonValue& out) override
    {
        last_build_group_list_uid = uid;
        out["group_uid"] = uid;
        out["groups"] = memochat::json::array_t{};
        out["groups"].append("group-a");
    }

    MessageCommandResult CreateGroup(const MessageCommandRequest& request) override
    {
        return Record(request, ID_CREATE_GROUP_RSP, R"({"error":0,"group":"create"})");
    }

    MessageCommandResult GetGroupList(const MessageCommandRequest& request) override
    {
        return Record(request, ID_GET_GROUP_LIST_RSP, R"({"error":0,"group":"list"})");
    }

    MessageCommandResult InviteGroupMember(const MessageCommandRequest& request) override
    {
        return Record(request, ID_INVITE_GROUP_MEMBER_RSP, R"({"error":0,"group":"invite"})");
    }

    MessageCommandResult ApplyJoinGroup(const MessageCommandRequest& request) override
    {
        return Record(request, ID_APPLY_JOIN_GROUP_RSP, R"({"error":0,"group":"apply"})");
    }

    MessageCommandResult ReviewGroupApply(const MessageCommandRequest& request) override
    {
        return Record(request, ID_REVIEW_GROUP_APPLY_RSP, R"({"error":0,"group":"review"})");
    }

    MessageCommandResult GroupChatMessage(const MessageCommandRequest& request) override
    {
        return Record(request, ID_GROUP_CHAT_MSG_RSP, R"({"error":0,"group":"roundtrip"})");
    }

    MessageCommandResult GroupHistory(const MessageCommandRequest& request) override
    {
        return Record(request, ID_GROUP_HISTORY_RSP, R"({"error":0,"group":"history"})");
    }

    MessageCommandResult GroupReadAck(const MessageCommandRequest& request) override
    {
        return Record(request, 0, R"({"error":0,"group":"read_ack"})");
    }

    MessageCommandResult EditGroupMessage(const MessageCommandRequest& request) override
    {
        return Record(request, ID_EDIT_GROUP_MSG_RSP, R"({"error":0,"group":"edit"})");
    }

    MessageCommandResult RevokeGroupMessage(const MessageCommandRequest& request) override
    {
        return Record(request, ID_REVOKE_GROUP_MSG_RSP, R"({"error":0,"group":"revoke"})");
    }

    MessageCommandResult ForwardGroupMessage(const MessageCommandRequest& request) override
    {
        return Record(request, ID_FORWARD_GROUP_MSG_RSP, R"({"error":0,"group":"forward"})");
    }

    MessageCommandResult UpdateGroupAnnouncement(const MessageCommandRequest& request) override
    {
        return Record(request, ID_UPDATE_GROUP_ANNOUNCEMENT_RSP, R"({"error":0,"group":"announcement"})");
    }

    MessageCommandResult UpdateGroupIcon(const MessageCommandRequest& request) override
    {
        return Record(request, ID_UPDATE_GROUP_ICON_RSP, R"({"error":0,"group":"icon"})");
    }

    MessageCommandResult SetGroupAdmin(const MessageCommandRequest& request) override
    {
        return Record(request, ID_SET_GROUP_ADMIN_RSP, R"({"error":0,"group":"admin"})");
    }

    MessageCommandResult MuteGroupMember(const MessageCommandRequest& request) override
    {
        return Record(request, ID_MUTE_GROUP_MEMBER_RSP, R"({"error":0,"group":"mute"})");
    }

    MessageCommandResult KickGroupMember(const MessageCommandRequest& request) override
    {
        return Record(request, ID_KICK_GROUP_MEMBER_RSP, R"({"error":0,"group":"kick"})");
    }

    MessageCommandResult QuitGroup(const MessageCommandRequest& request) override
    {
        return Record(request, ID_QUIT_GROUP_RSP, R"({"error":0,"group":"quit"})");
    }

    MessageCommandResult DissolveGroup(const MessageCommandRequest& request) override
    {
        return Record(request, ID_DISSOLVE_GROUP_RSP, R"({"error":0,"group":"dissolve"})");
    }

    MessageCommandRequest last_request;
    int last_build_group_list_uid = 0;

private:
    MessageCommandResult Record(const MessageCommandRequest& request, short response_msg_id, const char* payload)
    {
        last_request = request;
        return {response_msg_id, payload};
    }
};

class NonObjectGroupListService final : public chatinternal::ChatGroupMessageInternalService::Service
{
public:
    grpc::Status BuildGroupList(grpc::ServerContext*,
                                const chatinternal::BootstrapRequest*,
                                chatinternal::BootstrapResponse* response) override
    {
        response->set_error(ErrorCodes::Success);
        response->set_payload_json(R"(["not-an-object"])");
        return grpc::Status::OK;
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

RunningGrpcServer StartServer(ChatMessageInternalGrpcService* service)
{
    RunningGrpcServer running;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &running.port);
    builder.RegisterService(static_cast<chatinternal::ChatPrivateMessageInternalService::Service*>(service));
    builder.RegisterService(static_cast<chatinternal::ChatGroupMessageInternalService::Service*>(service));
    running.server = builder.BuildAndStart();
    return running;
}

RunningGrpcServer StartGroupServer(chatinternal::ChatGroupMessageInternalService::Service* service)
{
    RunningGrpcServer running;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &running.port);
    builder.RegisterService(service);
    running.server = builder.BuildAndStart();
    return running;
}

MessageCommandRequest BuildRequest(short msg_id, const std::string& payload)
{
    MessageCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = payload;
    request.session_uid = 42;
    request.session_id = "session-42";
    request.server_name = "ChatServer-1";
    request.trace_id = "trace-message-client";
    return request;
}

struct PrivateClientCase
{
    const char* name;
    short request_msg_id;
    short response_msg_id;
    std::string payload;
    std::string expected_payload;
    std::function<MessageCommandResult(MessageGrpcClient&, const MessageCommandRequest&)> call;
};

struct GroupClientCase
{
    const char* name;
    short request_msg_id;
    short response_msg_id;
    std::string payload;
    std::string expected_payload;
    std::function<MessageCommandResult(MessageGrpcClient&, const MessageCommandRequest&)> call;
};
} // namespace

TEST(MessageGrpcClientTest, TextChatMessageRoundTripsCommandPayload)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
    const auto request = BuildRequest(ID_TEXT_CHAT_MSG_REQ, R"({"fromuid":1,"touid":2})");

    const auto result = client.TextChatMessage(request);

    EXPECT_EQ(result.response_msg_id, ID_TEXT_CHAT_MSG_RSP);
    EXPECT_EQ(result.payload_json, R"({"error":0,"private":"roundtrip"})");
    EXPECT_EQ(private_service.last_request.request_msg_id, ID_TEXT_CHAT_MSG_REQ);
    EXPECT_EQ(private_service.last_request.payload_json, R"({"fromuid":1,"touid":2})");
    EXPECT_EQ(private_service.last_request.session_uid, 42);
    EXPECT_EQ(private_service.last_request.session_id, "session-42");
    EXPECT_EQ(private_service.last_request.server_name, "ChatServer-1");
    EXPECT_EQ(private_service.last_request.trace_id, "trace-message-client");
    running.server->Shutdown();
}

TEST(MessageGrpcClientTest, GroupChatMessageRoundTripsCommandPayload)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
    const auto request = BuildRequest(ID_GROUP_CHAT_MSG_REQ, R"({"fromuid":1,"groupid":9})");

    const auto result = client.GroupChatMessage(request);

    EXPECT_EQ(result.response_msg_id, ID_GROUP_CHAT_MSG_RSP);
    EXPECT_EQ(result.payload_json, R"({"error":0,"group":"roundtrip"})");
    EXPECT_EQ(group_service.last_request.request_msg_id, ID_GROUP_CHAT_MSG_REQ);
    EXPECT_EQ(group_service.last_request.payload_json, R"({"fromuid":1,"groupid":9})");
    EXPECT_EQ(group_service.last_request.session_uid, 42);
    EXPECT_EQ(group_service.last_request.session_id, "session-42");
    EXPECT_EQ(group_service.last_request.server_name, "ChatServer-1");
    EXPECT_EQ(group_service.last_request.trace_id, "trace-message-client");
    running.server->Shutdown();
}

TEST(MessageGrpcClientTest, BuildGroupListMergesRemotePayload)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;
    out["existing"] = "keep";

    client.BuildGroupListJson(42, out);

    EXPECT_EQ(group_service.last_build_group_list_uid, 42);
    EXPECT_EQ(out["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(out["existing"].asString(), "keep");
    EXPECT_EQ(out["group_uid"].asInt(), 42);
    const auto groups = out["groups"].get<memochat::json::JsonValue>();
    ASSERT_TRUE(groups.isArray());
    EXPECT_EQ(groups[0].asString(), "group-a");
    EXPECT_FALSE(out.isMember("message_remote_error"));
    running.server->Shutdown();
}

TEST(MessageGrpcClientTest, BuildGroupListRejectsNonObjectRemotePayload)
{
    NonObjectGroupListService service;
    auto running = StartGroupServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;

    client.BuildGroupListJson(42, out);

    EXPECT_EQ(out["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(out["message_remote_method"].asString(), "BuildGroupList");
    EXPECT_EQ(out["message_remote_error"].asString(), "invalid message payload json");
    EXPECT_EQ(out["message_remote_status_code"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_FALSE(out.isMember("0"));
    running.server->Shutdown();
}

TEST(MessageGrpcClientTest, PrivateCommandMethodsRoundTripCommandPayload)
{
    const PrivateClientCase cases[] = {
        {"ForwardPrivateMessage",
         ID_FORWARD_PRIVATE_MSG_REQ,
         ID_FORWARD_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1"})",
         R"({"error":0,"private":"forward"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.ForwardPrivateMessage(request);
         }},
        {"PrivateReadAck",
         ID_PRIVATE_READ_ACK_REQ,
         0,
         R"({"fromuid":1,"peer_uid":2,"read_ts":123})",
         R"({"error":0,"private":"read_ack"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.PrivateReadAck(request);
         }},
        {"EditPrivateMessage",
         ID_EDIT_PRIVATE_MSG_REQ,
         ID_EDIT_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1","content":"edited"})",
         R"({"error":0,"private":"edit"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.EditPrivateMessage(request);
         }},
        {"RevokePrivateMessage",
         ID_REVOKE_PRIVATE_MSG_REQ,
         ID_REVOKE_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1"})",
         R"({"error":0,"private":"revoke"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.RevokePrivateMessage(request);
         }},
        {"PrivateHistory",
         ID_PRIVATE_HISTORY_REQ,
         ID_PRIVATE_HISTORY_RSP,
         R"({"fromuid":1,"peer_uid":2,"limit":20})",
         R"({"error":0,"private":"history"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.PrivateHistory(request);
         }},
    };

    for (const auto& one : cases)
    {
        SCOPED_TRACE(one.name);
        FakePrivateMessageCommandService private_service;
        FakeGroupMessageCommandService group_service;
        ChatMessageInternalGrpcService service(&private_service, &group_service);
        auto running = StartServer(&service);
        ASSERT_NE(running.server, nullptr);
        ASSERT_GT(running.port, 0);

        MessageGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
        const auto request = BuildRequest(one.request_msg_id, one.payload);

        const auto result = one.call(client, request);

        EXPECT_EQ(result.response_msg_id, one.response_msg_id);
        EXPECT_EQ(result.payload_json, one.expected_payload);
        EXPECT_EQ(private_service.last_request.request_msg_id, one.request_msg_id);
        EXPECT_EQ(private_service.last_request.payload_json, one.payload);
        EXPECT_EQ(private_service.last_request.session_uid, 42);
        EXPECT_EQ(private_service.last_request.session_id, "session-42");
        EXPECT_EQ(private_service.last_request.server_name, "ChatServer-1");
        EXPECT_EQ(private_service.last_request.trace_id, "trace-message-client");
        running.server->Shutdown();
    }
}

TEST(MessageGrpcClientTest, GroupCommandMethodsRoundTripCommandPayload)
{
    const GroupClientCase cases[] = {
        {"CreateGroup",
         ID_CREATE_GROUP_REQ,
         ID_CREATE_GROUP_RSP,
         R"({"fromuid":1,"name":"team"})",
         R"({"error":0,"group":"create"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.CreateGroup(request);
         }},
        {"GetGroupList",
         ID_GET_GROUP_LIST_REQ,
         ID_GET_GROUP_LIST_RSP,
         R"({"fromuid":1})",
         R"({"error":0,"group":"list"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.GetGroupList(request);
         }},
        {"InviteGroupMember",
         ID_INVITE_GROUP_MEMBER_REQ,
         ID_INVITE_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"touid":2})",
         R"({"error":0,"group":"invite"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.InviteGroupMember(request);
         }},
        {"ApplyJoinGroup",
         ID_APPLY_JOIN_GROUP_REQ,
         ID_APPLY_JOIN_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"apply"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.ApplyJoinGroup(request);
         }},
        {"ReviewGroupApply",
         ID_REVIEW_GROUP_APPLY_REQ,
         ID_REVIEW_GROUP_APPLY_RSP,
         R"({"fromuid":1,"apply_id":3,"agree":true})",
         R"({"error":0,"group":"review"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.ReviewGroupApply(request);
         }},
        {"GroupHistory",
         ID_GROUP_HISTORY_REQ,
         ID_GROUP_HISTORY_RSP,
         R"({"fromuid":1,"groupid":9,"limit":20})",
         R"({"error":0,"group":"history"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.GroupHistory(request);
         }},
        {"GroupReadAck",
         ID_GROUP_READ_ACK_REQ,
         0,
         R"({"fromuid":1,"groupid":9,"read_ts":123})",
         R"({"error":0,"group":"read_ack"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.GroupReadAck(request);
         }},
        {"EditGroupMessage",
         ID_EDIT_GROUP_MSG_REQ,
         ID_EDIT_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1","content":"edited"})",
         R"({"error":0,"group":"edit"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.EditGroupMessage(request);
         }},
        {"RevokeGroupMessage",
         ID_REVOKE_GROUP_MSG_REQ,
         ID_REVOKE_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1"})",
         R"({"error":0,"group":"revoke"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.RevokeGroupMessage(request);
         }},
        {"ForwardGroupMessage",
         ID_FORWARD_GROUP_MSG_REQ,
         ID_FORWARD_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1","target_groupid":10})",
         R"({"error":0,"group":"forward"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.ForwardGroupMessage(request);
         }},
        {"UpdateGroupAnnouncement",
         ID_UPDATE_GROUP_ANNOUNCEMENT_REQ,
         ID_UPDATE_GROUP_ANNOUNCEMENT_RSP,
         R"({"fromuid":1,"groupid":9,"announcement":"hello"})",
         R"({"error":0,"group":"announcement"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.UpdateGroupAnnouncement(request);
         }},
        {"UpdateGroupIcon",
         ID_UPDATE_GROUP_ICON_REQ,
         ID_UPDATE_GROUP_ICON_RSP,
         R"({"fromuid":1,"groupid":9,"icon":"icon.png"})",
         R"({"error":0,"group":"icon"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.UpdateGroupIcon(request);
         }},
        {"SetGroupAdmin",
         ID_SET_GROUP_ADMIN_REQ,
         ID_SET_GROUP_ADMIN_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"is_admin":true})",
         R"({"error":0,"group":"admin"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.SetGroupAdmin(request);
         }},
        {"MuteGroupMember",
         ID_MUTE_GROUP_MEMBER_REQ,
         ID_MUTE_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"mute_until":123})",
         R"({"error":0,"group":"mute"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.MuteGroupMember(request);
         }},
        {"KickGroupMember",
         ID_KICK_GROUP_MEMBER_REQ,
         ID_KICK_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2})",
         R"({"error":0,"group":"kick"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.KickGroupMember(request);
         }},
        {"QuitGroup",
         ID_QUIT_GROUP_REQ,
         ID_QUIT_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"quit"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.QuitGroup(request);
         }},
        {"DissolveGroup",
         ID_DISSOLVE_GROUP_REQ,
         ID_DISSOLVE_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"dissolve"})",
         [](MessageGrpcClient& client, const MessageCommandRequest& request)
         {
             return client.DissolveGroup(request);
         }},
    };

    for (const auto& one : cases)
    {
        SCOPED_TRACE(one.name);
        FakePrivateMessageCommandService private_service;
        FakeGroupMessageCommandService group_service;
        ChatMessageInternalGrpcService service(&private_service, &group_service);
        auto running = StartServer(&service);
        ASSERT_NE(running.server, nullptr);
        ASSERT_GT(running.port, 0);

        MessageGrpcClient client(running.Endpoint(), std::chrono::milliseconds(500));
        const auto request = BuildRequest(one.request_msg_id, one.payload);

        const auto result = one.call(client, request);

        EXPECT_EQ(result.response_msg_id, one.response_msg_id);
        EXPECT_EQ(result.payload_json, one.expected_payload);
        EXPECT_EQ(group_service.last_request.request_msg_id, one.request_msg_id);
        EXPECT_EQ(group_service.last_request.payload_json, one.payload);
        EXPECT_EQ(group_service.last_request.session_uid, 42);
        EXPECT_EQ(group_service.last_request.session_id, "session-42");
        EXPECT_EQ(group_service.last_request.server_name, "ChatServer-1");
        EXPECT_EQ(group_service.last_request.trace_id, "trace-message-client");
        running.server->Shutdown();
    }
}

TEST(MessageGrpcClientTest, MissingPrivateStubReturnsRemoteErrorPayload)
{
    MessageGrpcClient client(std::shared_ptr<grpc::Channel>(), std::chrono::milliseconds(10));
    const auto request = BuildRequest(ID_TEXT_CHAT_MSG_REQ, R"({"fromuid":1,"touid":2})");

    const auto result = client.TextChatMessage(request);

    EXPECT_EQ(result.response_msg_id, ID_TEXT_CHAT_MSG_REQ);
    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(result.payload_json, payload)) << result.payload_json;
    EXPECT_EQ(payload["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(payload["message_remote_method"].asString(), "SendTextChatMessage");
    EXPECT_TRUE(payload.isMember("message_remote_error"));
}

TEST(MessageGrpcClientTest, MissingPrivateStubNamesNewPrivateRpcInErrorPayload)
{
    MessageGrpcClient client(std::shared_ptr<grpc::Channel>(), std::chrono::milliseconds(10));
    const auto request = BuildRequest(ID_EDIT_PRIVATE_MSG_REQ, R"({"fromuid":1,"peer_uid":2,"msgid":"m1"})");

    const auto result = client.EditPrivateMessage(request);

    EXPECT_EQ(result.response_msg_id, ID_EDIT_PRIVATE_MSG_REQ);
    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(result.payload_json, payload)) << result.payload_json;
    EXPECT_EQ(payload["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(payload["message_remote_method"].asString(), "EditPrivateMessage");
    EXPECT_TRUE(payload.isMember("message_remote_error"));
}

TEST(MessageGrpcClientTest, MissingGroupStubNamesNewGroupRpcInErrorPayload)
{
    MessageGrpcClient client(std::shared_ptr<grpc::Channel>(), std::chrono::milliseconds(10));
    const auto request = BuildRequest(ID_EDIT_GROUP_MSG_REQ, R"({"fromuid":1,"groupid":9,"msgid":"m1"})");

    const auto result = client.EditGroupMessage(request);

    EXPECT_EQ(result.response_msg_id, ID_EDIT_GROUP_MSG_REQ);
    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(result.payload_json, payload)) << result.payload_json;
    EXPECT_EQ(payload["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(payload["message_remote_method"].asString(), "EditGroupMessage");
    EXPECT_TRUE(payload.isMember("message_remote_error"));
}

TEST(MessageGrpcClientTest, MissingGroupStubNamesGroupManagementRpcInErrorPayload)
{
    MessageGrpcClient client(std::shared_ptr<grpc::Channel>(), std::chrono::milliseconds(10));
    const auto request = BuildRequest(ID_DISSOLVE_GROUP_REQ, R"({"fromuid":1,"groupid":9})");

    const auto result = client.DissolveGroup(request);

    EXPECT_EQ(result.response_msg_id, ID_DISSOLVE_GROUP_REQ);
    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(result.payload_json, payload)) << result.payload_json;
    EXPECT_EQ(payload["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(payload["message_remote_method"].asString(), "DissolveGroup");
    EXPECT_TRUE(payload.isMember("message_remote_error"));
}

TEST(MessageGrpcClientTest, MissingGroupStubNamesBuildGroupListInErrorFields)
{
    MessageGrpcClient client(std::shared_ptr<grpc::Channel>(), std::chrono::milliseconds(10));
    memochat::json::JsonValue out(memochat::json::object_t{});

    client.BuildGroupListJson(42, out);

    EXPECT_EQ(out["message_remote_method"].asString(), "BuildGroupList");
    EXPECT_TRUE(out.isMember("message_remote_error"));
    EXPECT_EQ(out["message_remote_status_code"].asInt(), ErrorCodes::RPCFailed);
}
