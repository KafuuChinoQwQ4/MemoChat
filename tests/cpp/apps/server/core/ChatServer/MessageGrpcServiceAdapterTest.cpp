#include <gtest/gtest.h>

#include "ChatMessageInternalGrpcService.hpp"
#include "MessageGrpcServiceAdapter.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"
#include "transport/CSession.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

namespace
{
class FakePrivateMessageCommandService final : public IPrivateMessageCommandService
{
public:
    MessageCommandResult TextChatMessage(const MessageCommandRequest& request) override
    {
        last_request = request;
        return {ID_TEXT_CHAT_MSG_RSP, R"({"error":0,"private":"adapter"})"};
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
        return Record(request, ID_GROUP_CHAT_MSG_RSP, R"({"error":0,"group":"adapter"})");
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

struct RunningGrpcServer
{
    int port = 0;
    std::unique_ptr<grpc::Server> server;

    std::string Endpoint() const
    {
        return std::string("127.0.0.1:") + std::to_string(port);
    }
};

class CapturingSession final : public CSession
{
public:
    struct SendRecord
    {
        short msg_id = 0;
        std::string payload;
    };

    explicit CapturingSession(boost::asio::io_context& io_context)
        : CSession(io_context, nullptr)
    {
    }

    void Send(char* msg, short max_length, short msgid) override
    {
        sends.push_back({msgid, std::string(msg, static_cast<std::size_t>(max_length))});
    }

    void Send(std::string msg, short msgid) override
    {
        sends.push_back({msgid, std::move(msg)});
    }

    std::vector<SendRecord> sends;
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

MessageCommandRequest BuildRequest(short msg_id, const std::string& payload)
{
    MessageCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = payload;
    request.session_uid = 42;
    request.session_id = "session-42";
    request.server_name = "ChatServer-1";
    request.trace_id = "trace-message-adapter";
    return request;
}

struct PrivateAdapterCase
{
    const char* name;
    short request_msg_id;
    short response_msg_id;
    std::string payload;
    std::string expected_payload;
    std::function<MessageCommandResult(MessageGrpcServiceAdapter&, const MessageCommandRequest&)> call;
};

struct GroupAdapterCase
{
    const char* name;
    short request_msg_id;
    short response_msg_id;
    std::string payload;
    std::string expected_payload;
    std::function<MessageCommandResult(MessageGrpcServiceAdapter&, const MessageCommandRequest&)> call;
};

struct GroupHandlerCase
{
    const char* name;
    short request_msg_id;
    short response_msg_id;
    std::string payload;
    std::string expected_payload;
    std::function<void(MessageGrpcServiceAdapter&, const std::shared_ptr<CSession>&, short, const std::string&)> call;
};
} // namespace

TEST(MessageGrpcServiceAdapterTest, TextChatMessageRoundTripsThroughRemoteAdapter)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    const auto request = BuildRequest(ID_TEXT_CHAT_MSG_REQ, R"({"fromuid":1,"touid":2})");

    const auto result = adapter.TextChatMessage(request);

    EXPECT_EQ(result.response_msg_id, ID_TEXT_CHAT_MSG_RSP);
    EXPECT_EQ(result.payload_json, R"({"error":0,"private":"adapter"})");
    EXPECT_EQ(private_service.last_request.request_msg_id, ID_TEXT_CHAT_MSG_REQ);
    EXPECT_EQ(private_service.last_request.session_uid, 42);
    EXPECT_EQ(private_service.last_request.session_id, "session-42");
    EXPECT_EQ(private_service.last_request.server_name, "ChatServer-1");
    EXPECT_EQ(private_service.last_request.trace_id, "trace-message-adapter");
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, GroupChatMessageRoundTripsThroughRemoteAdapter)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    const auto request = BuildRequest(ID_GROUP_CHAT_MSG_REQ, R"({"fromuid":1,"groupid":9})");

    const auto result = adapter.GroupChatMessage(request);

    EXPECT_EQ(result.response_msg_id, ID_GROUP_CHAT_MSG_RSP);
    EXPECT_EQ(result.payload_json, R"({"error":0,"group":"adapter"})");
    EXPECT_EQ(group_service.last_request.request_msg_id, ID_GROUP_CHAT_MSG_REQ);
    EXPECT_EQ(group_service.last_request.session_uid, 42);
    EXPECT_EQ(group_service.last_request.session_id, "session-42");
    EXPECT_EQ(group_service.last_request.server_name, "ChatServer-1");
    EXPECT_EQ(group_service.last_request.trace_id, "trace-message-adapter");
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, BuildGroupListMergesRemotePayload)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;

    adapter.BuildGroupListJson(42, out);

    EXPECT_EQ(group_service.last_build_group_list_uid, 42);
    EXPECT_EQ(out["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(out["group_uid"].asInt(), 42);
    const auto groups = out["groups"].get<memochat::json::JsonValue>();
    ASSERT_TRUE(groups.isArray());
    EXPECT_EQ(groups[0].asString(), "group-a");
    EXPECT_FALSE(out.isMember("message_remote_error"));
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, PrivateCommandMethodsRoundTripThroughRemoteAdapter)
{
    const PrivateAdapterCase cases[] = {
        {"ForwardPrivateMessage",
         ID_FORWARD_PRIVATE_MSG_REQ,
         ID_FORWARD_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1"})",
         R"({"error":0,"private":"forward"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.ForwardPrivateMessage(request);
         }},
        {"PrivateReadAck",
         ID_PRIVATE_READ_ACK_REQ,
         0,
         R"({"fromuid":1,"peer_uid":2,"read_ts":123})",
         R"({"error":0,"private":"read_ack"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.PrivateReadAck(request);
         }},
        {"EditPrivateMessage",
         ID_EDIT_PRIVATE_MSG_REQ,
         ID_EDIT_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1","content":"edited"})",
         R"({"error":0,"private":"edit"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.EditPrivateMessage(request);
         }},
        {"RevokePrivateMessage",
         ID_REVOKE_PRIVATE_MSG_REQ,
         ID_REVOKE_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1"})",
         R"({"error":0,"private":"revoke"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.RevokePrivateMessage(request);
         }},
        {"PrivateHistory",
         ID_PRIVATE_HISTORY_REQ,
         ID_PRIVATE_HISTORY_RSP,
         R"({"fromuid":1,"peer_uid":2,"limit":20})",
         R"({"error":0,"private":"history"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.PrivateHistory(request);
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

        MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
        const auto request = BuildRequest(one.request_msg_id, one.payload);

        const auto result = one.call(adapter, request);

        EXPECT_EQ(result.response_msg_id, one.response_msg_id);
        EXPECT_EQ(result.payload_json, one.expected_payload);
        EXPECT_EQ(private_service.last_request.request_msg_id, one.request_msg_id);
        EXPECT_EQ(private_service.last_request.payload_json, one.payload);
        EXPECT_EQ(private_service.last_request.session_uid, 42);
        EXPECT_EQ(private_service.last_request.session_id, "session-42");
        EXPECT_EQ(private_service.last_request.server_name, "ChatServer-1");
        EXPECT_EQ(private_service.last_request.trace_id, "trace-message-adapter");
        running.server->Shutdown();
    }
}

TEST(MessageGrpcServiceAdapterTest, GroupCommandMethodsRoundTripThroughRemoteAdapter)
{
    const GroupAdapterCase cases[] = {
        {"CreateGroup",
         ID_CREATE_GROUP_REQ,
         ID_CREATE_GROUP_RSP,
         R"({"fromuid":1,"name":"team"})",
         R"({"error":0,"group":"create"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.CreateGroup(request);
         }},
        {"GetGroupList",
         ID_GET_GROUP_LIST_REQ,
         ID_GET_GROUP_LIST_RSP,
         R"({"fromuid":1})",
         R"({"error":0,"group":"list"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.GetGroupList(request);
         }},
        {"InviteGroupMember",
         ID_INVITE_GROUP_MEMBER_REQ,
         ID_INVITE_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"touid":2})",
         R"({"error":0,"group":"invite"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.InviteGroupMember(request);
         }},
        {"ApplyJoinGroup",
         ID_APPLY_JOIN_GROUP_REQ,
         ID_APPLY_JOIN_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"apply"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.ApplyJoinGroup(request);
         }},
        {"ReviewGroupApply",
         ID_REVIEW_GROUP_APPLY_REQ,
         ID_REVIEW_GROUP_APPLY_RSP,
         R"({"fromuid":1,"apply_id":3,"agree":true})",
         R"({"error":0,"group":"review"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.ReviewGroupApply(request);
         }},
        {"GroupHistory",
         ID_GROUP_HISTORY_REQ,
         ID_GROUP_HISTORY_RSP,
         R"({"fromuid":1,"groupid":9,"limit":20})",
         R"({"error":0,"group":"history"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.GroupHistory(request);
         }},
        {"GroupReadAck",
         ID_GROUP_READ_ACK_REQ,
         0,
         R"({"fromuid":1,"groupid":9,"read_ts":123})",
         R"({"error":0,"group":"read_ack"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.GroupReadAck(request);
         }},
        {"EditGroupMessage",
         ID_EDIT_GROUP_MSG_REQ,
         ID_EDIT_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1","content":"edited"})",
         R"({"error":0,"group":"edit"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.EditGroupMessage(request);
         }},
        {"RevokeGroupMessage",
         ID_REVOKE_GROUP_MSG_REQ,
         ID_REVOKE_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1"})",
         R"({"error":0,"group":"revoke"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.RevokeGroupMessage(request);
         }},
        {"ForwardGroupMessage",
         ID_FORWARD_GROUP_MSG_REQ,
         ID_FORWARD_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1","target_groupid":10})",
         R"({"error":0,"group":"forward"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.ForwardGroupMessage(request);
         }},
        {"UpdateGroupAnnouncement",
         ID_UPDATE_GROUP_ANNOUNCEMENT_REQ,
         ID_UPDATE_GROUP_ANNOUNCEMENT_RSP,
         R"({"fromuid":1,"groupid":9,"announcement":"hello"})",
         R"({"error":0,"group":"announcement"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.UpdateGroupAnnouncement(request);
         }},
        {"UpdateGroupIcon",
         ID_UPDATE_GROUP_ICON_REQ,
         ID_UPDATE_GROUP_ICON_RSP,
         R"({"fromuid":1,"groupid":9,"icon":"icon.png"})",
         R"({"error":0,"group":"icon"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.UpdateGroupIcon(request);
         }},
        {"SetGroupAdmin",
         ID_SET_GROUP_ADMIN_REQ,
         ID_SET_GROUP_ADMIN_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"is_admin":true})",
         R"({"error":0,"group":"admin"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.SetGroupAdmin(request);
         }},
        {"MuteGroupMember",
         ID_MUTE_GROUP_MEMBER_REQ,
         ID_MUTE_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"mute_until":123})",
         R"({"error":0,"group":"mute"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.MuteGroupMember(request);
         }},
        {"KickGroupMember",
         ID_KICK_GROUP_MEMBER_REQ,
         ID_KICK_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2})",
         R"({"error":0,"group":"kick"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.KickGroupMember(request);
         }},
        {"QuitGroup",
         ID_QUIT_GROUP_REQ,
         ID_QUIT_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"quit"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.QuitGroup(request);
         }},
        {"DissolveGroup",
         ID_DISSOLVE_GROUP_REQ,
         ID_DISSOLVE_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"dissolve"})",
         [](MessageGrpcServiceAdapter& adapter, const MessageCommandRequest& request)
         {
             return adapter.DissolveGroup(request);
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

        MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
        const auto request = BuildRequest(one.request_msg_id, one.payload);

        const auto result = one.call(adapter, request);

        EXPECT_EQ(result.response_msg_id, one.response_msg_id);
        EXPECT_EQ(result.payload_json, one.expected_payload);
        EXPECT_EQ(group_service.last_request.request_msg_id, one.request_msg_id);
        EXPECT_EQ(group_service.last_request.payload_json, one.payload);
        EXPECT_EQ(group_service.last_request.session_uid, 42);
        EXPECT_EQ(group_service.last_request.session_id, "session-42");
        EXPECT_EQ(group_service.last_request.server_name, "ChatServer-1");
        EXPECT_EQ(group_service.last_request.trace_id, "trace-message-adapter");
        running.server->Shutdown();
    }
}

TEST(MessageGrpcServiceAdapterTest, PrivateReadAckCommandReturnsSilentSuccessMessageId)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    const auto request = BuildRequest(ID_PRIVATE_READ_ACK_REQ, R"({"fromuid":1,"peer_uid":2,"read_ts":123})");

    const auto result = adapter.PrivateReadAck(request);

    EXPECT_EQ(result.response_msg_id, 0);
    EXPECT_EQ(result.payload_json, R"({"error":0,"private":"read_ack"})");
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, GroupReadAckCommandReturnsSilentSuccessMessageId)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    const auto request = BuildRequest(ID_GROUP_READ_ACK_REQ, R"({"fromuid":1,"groupid":9,"read_ts":123})");

    const auto result = adapter.GroupReadAck(request);

    EXPECT_EQ(result.response_msg_id, 0);
    EXPECT_EQ(result.payload_json, R"({"error":0,"group":"read_ack"})");
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, PrivateHandlerSendsRemoteResultToSession)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    boost::asio::io_context io_context;
    auto session = std::make_shared<CapturingSession>(io_context);
    session->SetUserId(42);

    adapter.HandleEditPrivateMessage(
        session,
        ID_EDIT_PRIVATE_MSG_REQ,
        R"({"fromuid":1,"peer_uid":2,"msgid":"m1","content":"edited","trace_id":"trace-handler"})");

    ASSERT_EQ(session->sends.size(), 1U);
    EXPECT_EQ(session->sends.front().msg_id, ID_EDIT_PRIVATE_MSG_RSP);
    EXPECT_EQ(session->sends.front().payload, R"({"error":0,"private":"edit"})");
    EXPECT_EQ(private_service.last_request.request_msg_id, ID_EDIT_PRIVATE_MSG_REQ);
    EXPECT_EQ(private_service.last_request.session_uid, 42);
    EXPECT_FALSE(private_service.last_request.session_id.empty());
    EXPECT_EQ(private_service.last_request.trace_id, "trace-handler");
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, GroupHandlerSendsRemoteResultToSession)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    boost::asio::io_context io_context;
    auto session = std::make_shared<CapturingSession>(io_context);
    session->SetUserId(42);

    adapter.HandleEditGroupMessage(
        session,
        ID_EDIT_GROUP_MSG_REQ,
        R"({"fromuid":1,"groupid":9,"msgid":"m1","content":"edited","trace_id":"trace-group-handler"})");

    ASSERT_EQ(session->sends.size(), 1U);
    EXPECT_EQ(session->sends.front().msg_id, ID_EDIT_GROUP_MSG_RSP);
    EXPECT_EQ(session->sends.front().payload, R"({"error":0,"group":"edit"})");
    EXPECT_EQ(group_service.last_request.request_msg_id, ID_EDIT_GROUP_MSG_REQ);
    EXPECT_EQ(group_service.last_request.session_uid, 42);
    EXPECT_FALSE(group_service.last_request.session_id.empty());
    EXPECT_EQ(group_service.last_request.trace_id, "trace-group-handler");
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, GroupManagementHandlersSendRemoteResultsToSession)
{
    const GroupHandlerCase cases[] = {
        {"CreateGroup",
         ID_CREATE_GROUP_REQ,
         ID_CREATE_GROUP_RSP,
         R"({"fromuid":1,"name":"team","trace_id":"trace-create"})",
         R"({"error":0,"group":"create"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleCreateGroup(session, msg_id, payload);
         }},
        {"GetGroupList",
         ID_GET_GROUP_LIST_REQ,
         ID_GET_GROUP_LIST_RSP,
         R"({"fromuid":1,"trace_id":"trace-list"})",
         R"({"error":0,"group":"list"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleGetGroupList(session, msg_id, payload);
         }},
        {"InviteGroupMember",
         ID_INVITE_GROUP_MEMBER_REQ,
         ID_INVITE_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"touid":2,"trace_id":"trace-invite"})",
         R"({"error":0,"group":"invite"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleInviteGroupMember(session, msg_id, payload);
         }},
        {"ApplyJoinGroup",
         ID_APPLY_JOIN_GROUP_REQ,
         ID_APPLY_JOIN_GROUP_RSP,
         R"({"fromuid":1,"groupid":9,"trace_id":"trace-apply"})",
         R"({"error":0,"group":"apply"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleApplyJoinGroup(session, msg_id, payload);
         }},
        {"ReviewGroupApply",
         ID_REVIEW_GROUP_APPLY_REQ,
         ID_REVIEW_GROUP_APPLY_RSP,
         R"({"fromuid":1,"apply_id":3,"agree":true,"trace_id":"trace-review"})",
         R"({"error":0,"group":"review"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleReviewGroupApply(session, msg_id, payload);
         }},
        {"UpdateGroupAnnouncement",
         ID_UPDATE_GROUP_ANNOUNCEMENT_REQ,
         ID_UPDATE_GROUP_ANNOUNCEMENT_RSP,
         R"({"fromuid":1,"groupid":9,"announcement":"hello","trace_id":"trace-announcement"})",
         R"({"error":0,"group":"announcement"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleUpdateGroupAnnouncement(session, msg_id, payload);
         }},
        {"UpdateGroupIcon",
         ID_UPDATE_GROUP_ICON_REQ,
         ID_UPDATE_GROUP_ICON_RSP,
         R"({"fromuid":1,"groupid":9,"icon":"icon.png","trace_id":"trace-icon"})",
         R"({"error":0,"group":"icon"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleUpdateGroupIcon(session, msg_id, payload);
         }},
        {"SetGroupAdmin",
         ID_SET_GROUP_ADMIN_REQ,
         ID_SET_GROUP_ADMIN_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"is_admin":true,"trace_id":"trace-admin"})",
         R"({"error":0,"group":"admin"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleSetGroupAdmin(session, msg_id, payload);
         }},
        {"MuteGroupMember",
         ID_MUTE_GROUP_MEMBER_REQ,
         ID_MUTE_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"mute_until":123,"trace_id":"trace-mute"})",
         R"({"error":0,"group":"mute"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleMuteGroupMember(session, msg_id, payload);
         }},
        {"KickGroupMember",
         ID_KICK_GROUP_MEMBER_REQ,
         ID_KICK_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"trace_id":"trace-kick"})",
         R"({"error":0,"group":"kick"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleKickGroupMember(session, msg_id, payload);
         }},
        {"QuitGroup",
         ID_QUIT_GROUP_REQ,
         ID_QUIT_GROUP_RSP,
         R"({"fromuid":1,"groupid":9,"trace_id":"trace-quit"})",
         R"({"error":0,"group":"quit"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleQuitGroup(session, msg_id, payload);
         }},
        {"DissolveGroup",
         ID_DISSOLVE_GROUP_REQ,
         ID_DISSOLVE_GROUP_RSP,
         R"({"fromuid":1,"groupid":9,"trace_id":"trace-dissolve"})",
         R"({"error":0,"group":"dissolve"})",
         [](MessageGrpcServiceAdapter& adapter,
            const std::shared_ptr<CSession>& session,
            short msg_id,
            const std::string& payload)
         {
             adapter.HandleDissolveGroup(session, msg_id, payload);
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

        MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
        boost::asio::io_context io_context;
        auto session = std::make_shared<CapturingSession>(io_context);
        session->SetUserId(42);

        one.call(adapter, session, one.request_msg_id, one.payload);

        ASSERT_EQ(session->sends.size(), 1U);
        EXPECT_EQ(session->sends.front().msg_id, one.response_msg_id);
        EXPECT_EQ(session->sends.front().payload, one.expected_payload);
        EXPECT_EQ(group_service.last_request.request_msg_id, one.request_msg_id);
        EXPECT_EQ(group_service.last_request.payload_json, one.payload);
        EXPECT_EQ(group_service.last_request.session_uid, 42);
        EXPECT_FALSE(group_service.last_request.session_id.empty());
        EXPECT_NE(group_service.last_request.trace_id.find("trace-"), std::string::npos);
        running.server->Shutdown();
    }
}

TEST(MessageGrpcServiceAdapterTest, PrivateReadAckHandlerDoesNotSendSessionSuccess)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    boost::asio::io_context io_context;
    auto session = std::make_shared<CapturingSession>(io_context);
    session->SetUserId(42);

    adapter.HandlePrivateReadAck(session,
                                 ID_PRIVATE_READ_ACK_REQ,
                                 R"({"fromuid":1,"peer_uid":2,"read_ts":123,"trace_id":"trace-read-ack"})");

    EXPECT_TRUE(session->sends.empty());
    EXPECT_EQ(private_service.last_request.request_msg_id, ID_PRIVATE_READ_ACK_REQ);
    EXPECT_EQ(private_service.last_request.session_uid, 42);
    EXPECT_EQ(private_service.last_request.trace_id, "trace-read-ack");
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, GroupReadAckHandlerDoesNotSendSessionSuccess)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    ASSERT_GT(running.port, 0);

    MessageGrpcServiceAdapter adapter(running.Endpoint(), std::chrono::milliseconds(500));
    boost::asio::io_context io_context;
    auto session = std::make_shared<CapturingSession>(io_context);
    session->SetUserId(42);

    adapter.HandleGroupReadAck(session,
                               ID_GROUP_READ_ACK_REQ,
                               R"({"fromuid":1,"groupid":9,"read_ts":123,"trace_id":"trace-group-read-ack"})");

    EXPECT_TRUE(session->sends.empty());
    EXPECT_EQ(group_service.last_request.request_msg_id, ID_GROUP_READ_ACK_REQ);
    EXPECT_EQ(group_service.last_request.session_uid, 42);
    EXPECT_EQ(group_service.last_request.trace_id, "trace-group-read-ack");
    running.server->Shutdown();
}

TEST(MessageGrpcServiceAdapterTest, BuildGroupListMissingStubReturnsExplicitRemoteError)
{
    MessageGrpcServiceAdapter adapter(std::shared_ptr<grpc::Channel>(), std::chrono::milliseconds(10));
    memochat::json::JsonValue out(memochat::json::object_t{});

    adapter.BuildGroupListJson(42, out);

    EXPECT_EQ(out["message_remote_method"].asString(), "BuildGroupList");
    EXPECT_TRUE(out.isMember("message_remote_error"));
    EXPECT_EQ(out["message_remote_status_code"].asInt(), ErrorCodes::RPCFailed);
}
