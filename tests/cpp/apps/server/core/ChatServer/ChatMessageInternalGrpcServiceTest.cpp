#include <gtest/gtest.h>

#include "ChatMessageInternalGrpcService.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"

#include <functional>
#include <string>

namespace
{
class FakePrivateMessageCommandService final : public IPrivateMessageCommandService
{
public:
    MessageCommandResult TextChatMessage(const MessageCommandRequest& request) override
    {
        last_request = request;
        return {ID_TEXT_CHAT_MSG_RSP, R"({"error":0,"private":"ok"})"};
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
        return Record(request, ID_GROUP_CHAT_MSG_RSP, R"({"error":0,"group":"ok"})");
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

chatinternal::JsonPayloadRequest BuildRequest(short msg_id, const std::string& payload)
{
    chatinternal::JsonPayloadRequest request;
    request.set_tcp_msg_id(msg_id);
    request.set_payload_json(payload);
    request.mutable_session()->set_uid(42);
    request.mutable_session()->set_session_id("session-42");
    request.mutable_session()->set_server_name("ChatServer-1");
    request.mutable_session()->set_trace_id("trace-message-command");
    return request;
}

struct PrivateRpcCase
{
    const char* name;
    short request_msg_id;
    short response_msg_id;
    std::string payload;
    std::string expected_payload;
    std::function<grpc::Status(ChatMessageInternalGrpcService&,
                               const chatinternal::JsonPayloadRequest*,
                               chatinternal::JsonPayloadResponse*)>
        call;
};

struct GroupRpcCase
{
    const char* name;
    short request_msg_id;
    short response_msg_id;
    std::string payload;
    std::string expected_payload;
    std::function<grpc::Status(ChatMessageInternalGrpcService&,
                               const chatinternal::JsonPayloadRequest*,
                               chatinternal::JsonPayloadResponse*)>
        call;
};
} // namespace

TEST(ChatMessageInternalGrpcServiceTest, BuildGroupListDelegatesToGroupCommandService)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);

    chatinternal::BootstrapRequest request;
    request.set_uid(42);
    request.set_trace_id("trace-group-list");
    chatinternal::BootstrapResponse response;

    const auto status = service.BuildGroupList(nullptr, &request, &response);

    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.error(), ErrorCodes::Success);
    EXPECT_EQ(group_service.last_build_group_list_uid, 42);
    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(response.payload_json(), payload)) << response.payload_json();
    EXPECT_EQ(payload["group_uid"].asInt(), 42);
    const auto groups = payload["groups"].get<memochat::json::JsonValue>();
    ASSERT_TRUE(groups.isArray());
    EXPECT_EQ(groups[0].asString(), "group-a");
}

TEST(ChatMessageInternalGrpcServiceTest, SendTextChatMessageDelegatesToPrivateCommandService)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);

    const auto request = BuildRequest(ID_TEXT_CHAT_MSG_REQ, R"({"fromuid":1,"touid":2})");
    chatinternal::JsonPayloadResponse response;

    const auto status = service.SendTextChatMessage(nullptr, &request, &response);

    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.error(), ErrorCodes::Success);
    EXPECT_EQ(response.tcp_msg_id(), ID_TEXT_CHAT_MSG_RSP);
    EXPECT_EQ(response.payload_json(), R"({"error":0,"private":"ok"})");
    EXPECT_EQ(private_service.last_request.request_msg_id, ID_TEXT_CHAT_MSG_REQ);
    EXPECT_EQ(private_service.last_request.payload_json, R"({"fromuid":1,"touid":2})");
    EXPECT_EQ(private_service.last_request.session_uid, 42);
    EXPECT_EQ(private_service.last_request.session_id, "session-42");
    EXPECT_EQ(private_service.last_request.server_name, "ChatServer-1");
    EXPECT_EQ(private_service.last_request.trace_id, "trace-message-command");
}

TEST(ChatMessageInternalGrpcServiceTest, GroupChatMessageDelegatesToGroupCommandService)
{
    FakePrivateMessageCommandService private_service;
    FakeGroupMessageCommandService group_service;
    ChatMessageInternalGrpcService service(&private_service, &group_service);

    const auto request = BuildRequest(ID_GROUP_CHAT_MSG_REQ, R"({"fromuid":1,"groupid":9})");
    chatinternal::JsonPayloadResponse response;

    const auto status = service.GroupChatMessage(nullptr, &request, &response);

    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.error(), ErrorCodes::Success);
    EXPECT_EQ(response.tcp_msg_id(), ID_GROUP_CHAT_MSG_RSP);
    EXPECT_EQ(response.payload_json(), R"({"error":0,"group":"ok"})");
    EXPECT_EQ(group_service.last_request.request_msg_id, ID_GROUP_CHAT_MSG_REQ);
    EXPECT_EQ(group_service.last_request.payload_json, R"({"fromuid":1,"groupid":9})");
    EXPECT_EQ(group_service.last_request.session_uid, 42);
    EXPECT_EQ(group_service.last_request.session_id, "session-42");
    EXPECT_EQ(group_service.last_request.server_name, "ChatServer-1");
    EXPECT_EQ(group_service.last_request.trace_id, "trace-message-command");
}

TEST(ChatMessageInternalGrpcServiceTest, PrivateCommandMethodsDelegateToPrivateCommandService)
{
    const PrivateRpcCase cases[] = {
        {"ForwardPrivateMessage",
         ID_FORWARD_PRIVATE_MSG_REQ,
         ID_FORWARD_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1"})",
         R"({"error":0,"private":"forward"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.ForwardPrivateMessage(nullptr, request, response);
         }},
        {"PrivateReadAck",
         ID_PRIVATE_READ_ACK_REQ,
         0,
         R"({"fromuid":1,"peer_uid":2,"read_ts":123})",
         R"({"error":0,"private":"read_ack"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.PrivateReadAck(nullptr, request, response);
         }},
        {"EditPrivateMessage",
         ID_EDIT_PRIVATE_MSG_REQ,
         ID_EDIT_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1","content":"edited"})",
         R"({"error":0,"private":"edit"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.EditPrivateMessage(nullptr, request, response);
         }},
        {"RevokePrivateMessage",
         ID_REVOKE_PRIVATE_MSG_REQ,
         ID_REVOKE_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"m1"})",
         R"({"error":0,"private":"revoke"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.RevokePrivateMessage(nullptr, request, response);
         }},
        {"PrivateHistory",
         ID_PRIVATE_HISTORY_REQ,
         ID_PRIVATE_HISTORY_RSP,
         R"({"fromuid":1,"peer_uid":2,"limit":20})",
         R"({"error":0,"private":"history"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.PrivateHistory(nullptr, request, response);
         }},
    };

    for (const auto& one : cases)
    {
        SCOPED_TRACE(one.name);
        FakePrivateMessageCommandService private_service;
        FakeGroupMessageCommandService group_service;
        ChatMessageInternalGrpcService service(&private_service, &group_service);

        const auto request = BuildRequest(one.request_msg_id, one.payload);
        chatinternal::JsonPayloadResponse response;

        const auto status = one.call(service, &request, &response);

        ASSERT_TRUE(status.ok()) << status.error_message();
        EXPECT_EQ(response.error(), ErrorCodes::Success);
        EXPECT_EQ(response.tcp_msg_id(), one.response_msg_id);
        EXPECT_EQ(response.payload_json(), one.expected_payload);
        EXPECT_EQ(private_service.last_request.request_msg_id, one.request_msg_id);
        EXPECT_EQ(private_service.last_request.payload_json, one.payload);
        EXPECT_EQ(private_service.last_request.session_uid, 42);
        EXPECT_EQ(private_service.last_request.session_id, "session-42");
        EXPECT_EQ(private_service.last_request.server_name, "ChatServer-1");
        EXPECT_EQ(private_service.last_request.trace_id, "trace-message-command");
    }
}

TEST(ChatMessageInternalGrpcServiceTest, GroupCommandMethodsDelegateToGroupCommandService)
{
    const GroupRpcCase cases[] = {
        {"CreateGroup",
         ID_CREATE_GROUP_REQ,
         ID_CREATE_GROUP_RSP,
         R"({"fromuid":1,"name":"team"})",
         R"({"error":0,"group":"create"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.CreateGroup(nullptr, request, response);
         }},
        {"GetGroupList",
         ID_GET_GROUP_LIST_REQ,
         ID_GET_GROUP_LIST_RSP,
         R"({"fromuid":1})",
         R"({"error":0,"group":"list"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.GetGroupList(nullptr, request, response);
         }},
        {"InviteGroupMember",
         ID_INVITE_GROUP_MEMBER_REQ,
         ID_INVITE_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"touid":2})",
         R"({"error":0,"group":"invite"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.InviteGroupMember(nullptr, request, response);
         }},
        {"ApplyJoinGroup",
         ID_APPLY_JOIN_GROUP_REQ,
         ID_APPLY_JOIN_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"apply"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.ApplyJoinGroup(nullptr, request, response);
         }},
        {"ReviewGroupApply",
         ID_REVIEW_GROUP_APPLY_REQ,
         ID_REVIEW_GROUP_APPLY_RSP,
         R"({"fromuid":1,"apply_id":3,"agree":true})",
         R"({"error":0,"group":"review"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.ReviewGroupApply(nullptr, request, response);
         }},
        {"GroupHistory",
         ID_GROUP_HISTORY_REQ,
         ID_GROUP_HISTORY_RSP,
         R"({"fromuid":1,"groupid":9,"limit":20})",
         R"({"error":0,"group":"history"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.GroupHistory(nullptr, request, response);
         }},
        {"GroupReadAck",
         ID_GROUP_READ_ACK_REQ,
         0,
         R"({"fromuid":1,"groupid":9,"read_ts":123})",
         R"({"error":0,"group":"read_ack"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.GroupReadAck(nullptr, request, response);
         }},
        {"EditGroupMessage",
         ID_EDIT_GROUP_MSG_REQ,
         ID_EDIT_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1","content":"edited"})",
         R"({"error":0,"group":"edit"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.EditGroupMessage(nullptr, request, response);
         }},
        {"RevokeGroupMessage",
         ID_REVOKE_GROUP_MSG_REQ,
         ID_REVOKE_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1"})",
         R"({"error":0,"group":"revoke"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.RevokeGroupMessage(nullptr, request, response);
         }},
        {"ForwardGroupMessage",
         ID_FORWARD_GROUP_MSG_REQ,
         ID_FORWARD_GROUP_MSG_RSP,
         R"({"fromuid":1,"groupid":9,"msgid":"m1","target_groupid":10})",
         R"({"error":0,"group":"forward"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.ForwardGroupMessage(nullptr, request, response);
         }},
        {"UpdateGroupAnnouncement",
         ID_UPDATE_GROUP_ANNOUNCEMENT_REQ,
         ID_UPDATE_GROUP_ANNOUNCEMENT_RSP,
         R"({"fromuid":1,"groupid":9,"announcement":"hello"})",
         R"({"error":0,"group":"announcement"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.UpdateGroupAnnouncement(nullptr, request, response);
         }},
        {"UpdateGroupIcon",
         ID_UPDATE_GROUP_ICON_REQ,
         ID_UPDATE_GROUP_ICON_RSP,
         R"({"fromuid":1,"groupid":9,"icon":"icon.png"})",
         R"({"error":0,"group":"icon"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.UpdateGroupIcon(nullptr, request, response);
         }},
        {"SetGroupAdmin",
         ID_SET_GROUP_ADMIN_REQ,
         ID_SET_GROUP_ADMIN_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"is_admin":true})",
         R"({"error":0,"group":"admin"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.SetGroupAdmin(nullptr, request, response);
         }},
        {"MuteGroupMember",
         ID_MUTE_GROUP_MEMBER_REQ,
         ID_MUTE_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2,"mute_until":123})",
         R"({"error":0,"group":"mute"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.MuteGroupMember(nullptr, request, response);
         }},
        {"KickGroupMember",
         ID_KICK_GROUP_MEMBER_REQ,
         ID_KICK_GROUP_MEMBER_RSP,
         R"({"fromuid":1,"groupid":9,"target_uid":2})",
         R"({"error":0,"group":"kick"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.KickGroupMember(nullptr, request, response);
         }},
        {"QuitGroup",
         ID_QUIT_GROUP_REQ,
         ID_QUIT_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"quit"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.QuitGroup(nullptr, request, response);
         }},
        {"DissolveGroup",
         ID_DISSOLVE_GROUP_REQ,
         ID_DISSOLVE_GROUP_RSP,
         R"({"fromuid":1,"groupid":9})",
         R"({"error":0,"group":"dissolve"})",
         [](ChatMessageInternalGrpcService& service,
            const chatinternal::JsonPayloadRequest* request,
            chatinternal::JsonPayloadResponse* response)
         {
             return service.DissolveGroup(nullptr, request, response);
         }},
    };

    for (const auto& one : cases)
    {
        SCOPED_TRACE(one.name);
        FakePrivateMessageCommandService private_service;
        FakeGroupMessageCommandService group_service;
        ChatMessageInternalGrpcService service(&private_service, &group_service);

        const auto request = BuildRequest(one.request_msg_id, one.payload);
        chatinternal::JsonPayloadResponse response;

        const auto status = one.call(service, &request, &response);

        ASSERT_TRUE(status.ok()) << status.error_message();
        EXPECT_EQ(response.error(), ErrorCodes::Success);
        EXPECT_EQ(response.tcp_msg_id(), one.response_msg_id);
        EXPECT_EQ(response.payload_json(), one.expected_payload);
        EXPECT_EQ(group_service.last_request.request_msg_id, one.request_msg_id);
        EXPECT_EQ(group_service.last_request.payload_json, one.payload);
        EXPECT_EQ(group_service.last_request.session_uid, 42);
        EXPECT_EQ(group_service.last_request.session_id, "session-42");
        EXPECT_EQ(group_service.last_request.server_name, "ChatServer-1");
        EXPECT_EQ(group_service.last_request.trace_id, "trace-message-command");
    }
}

TEST(ChatMessageInternalGrpcServiceTest, MissingCommandServicesReturnFailedPrecondition)
{
    ChatMessageInternalGrpcService service(nullptr, nullptr);

    const auto request = BuildRequest(ID_TEXT_CHAT_MSG_REQ, R"({"fromuid":1,"touid":2})");
    chatinternal::JsonPayloadResponse response;

    const auto status = service.SendTextChatMessage(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_EQ(response.error(), ErrorCodes::RPCFailed);
    EXPECT_EQ(response.payload_json(), "{}");
}
