#include <gtest/gtest.h>

#include "ConfigMgr.h"
#include "MessageGrpcClient.h"
#include "MessageGrpcServiceAdapter.h"
#include "MessageServiceConfig.h"
#include "MessageServiceFactory.h"
#include "const.h"
#include "json/GlazeCompat.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

namespace
{
const char* EnvValue(const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr ? "" : value;
}

MessageCommandRequest
BuildPrivateProbeRequest(short request_msg_id, const std::string& payload_json, const char* trace_suffix)
{
    MessageCommandRequest request;
    request.request_msg_id = request_msg_id;
    request.payload_json = payload_json;
    request.session_uid = 1;
    request.session_id = "message-remote-smoke-private";
    request.server_name = "ChatServer-smoke";
    request.trace_id = std::string("message-remote-smoke-private-") + trace_suffix;
    return request;
}

MessageCommandRequest
BuildGroupProbeRequest(short request_msg_id, const std::string& payload_json, const char* trace_suffix)
{
    MessageCommandRequest request;
    request.request_msg_id = request_msg_id;
    request.payload_json = payload_json;
    request.session_uid = 1;
    request.session_id = "message-remote-smoke-group";
    request.server_name = "ChatServer-smoke";
    request.trace_id = std::string("message-remote-smoke-group-") + trace_suffix;
    return request;
}

void ExpectNoRemoteErrorPayload(const std::string& payload_json)
{
    memochat::json::JsonValue payload(memochat::json::object_t{});
    ASSERT_TRUE(memochat::json::reader_parse(payload_json, payload)) << payload_json;
    EXPECT_FALSE(payload.isMember("message_remote_method")) << payload_json;
    EXPECT_FALSE(payload.isMember("message_remote_error")) << payload_json;
    EXPECT_FALSE(payload.isMember("message_remote_status_code")) << payload_json;
}

struct PrivateGrpcProbeCase
{
    const char* name;
    short request_msg_id;
    short expected_response_msg_id;
    const char* payload_json;
    std::function<MessageCommandResult(MessageGrpcClient&, const MessageCommandRequest&)> call;
};

struct GroupGrpcProbeCase
{
    const char* name;
    short request_msg_id;
    short expected_response_msg_id;
    const char* payload_json;
    std::function<MessageCommandResult(MessageGrpcClient&, const MessageCommandRequest&)> call;
};

class FakePrivateMessageService final : public IPrivateMessageService
{
public:
    MessageCommandResult TextChatMessage(const MessageCommandRequest&) override
    {
        return {ID_TEXT_CHAT_MSG_RSP, R"({"error":0})"};
    }

    MessageCommandResult ForwardPrivateMessage(const MessageCommandRequest&) override
    {
        return {ID_FORWARD_PRIVATE_MSG_RSP, R"({"error":0})"};
    }

    MessageCommandResult PrivateReadAck(const MessageCommandRequest&) override
    {
        return {0, R"({"error":0})"};
    }

    MessageCommandResult EditPrivateMessage(const MessageCommandRequest&) override
    {
        return {ID_EDIT_PRIVATE_MSG_RSP, R"({"error":0})"};
    }

    MessageCommandResult RevokePrivateMessage(const MessageCommandRequest&) override
    {
        return {ID_REVOKE_PRIVATE_MSG_RSP, R"({"error":0})"};
    }

    MessageCommandResult PrivateHistory(const MessageCommandRequest&) override
    {
        return {ID_PRIVATE_HISTORY_RSP, R"({"error":0})"};
    }

    void HandleTextChatMessage(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleForwardPrivateMessage(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandlePrivateReadAck(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleEditPrivateMessage(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleRevokePrivateMessage(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandlePrivateHistory(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
};

class FakeGroupMessageService final : public IGroupMessageService
{
public:
    void BuildGroupListJson(int, memochat::json::JsonValue&) override
    {
    }

    MessageCommandResult CreateGroup(const MessageCommandRequest&) override
    {
        return {ID_CREATE_GROUP_RSP, R"({"error":0})"};
    }

    MessageCommandResult GetGroupList(const MessageCommandRequest&) override
    {
        return {ID_GET_GROUP_LIST_RSP, R"({"error":0})"};
    }

    MessageCommandResult InviteGroupMember(const MessageCommandRequest&) override
    {
        return {ID_INVITE_GROUP_MEMBER_RSP, R"({"error":0})"};
    }

    MessageCommandResult ApplyJoinGroup(const MessageCommandRequest&) override
    {
        return {ID_APPLY_JOIN_GROUP_RSP, R"({"error":0})"};
    }

    MessageCommandResult ReviewGroupApply(const MessageCommandRequest&) override
    {
        return {ID_REVIEW_GROUP_APPLY_RSP, R"({"error":0})"};
    }

    MessageCommandResult GroupChatMessage(const MessageCommandRequest&) override
    {
        return {ID_GROUP_CHAT_MSG_RSP, R"({"error":0})"};
    }

    MessageCommandResult GroupHistory(const MessageCommandRequest&) override
    {
        return {ID_GROUP_HISTORY_RSP, R"({"error":0})"};
    }

    MessageCommandResult GroupReadAck(const MessageCommandRequest&) override
    {
        return {0, R"({"error":0})"};
    }

    MessageCommandResult EditGroupMessage(const MessageCommandRequest&) override
    {
        return {ID_EDIT_GROUP_MSG_RSP, R"({"error":0})"};
    }

    MessageCommandResult RevokeGroupMessage(const MessageCommandRequest&) override
    {
        return {ID_REVOKE_GROUP_MSG_RSP, R"({"error":0})"};
    }

    MessageCommandResult ForwardGroupMessage(const MessageCommandRequest&) override
    {
        return {ID_FORWARD_GROUP_MSG_RSP, R"({"error":0})"};
    }

    MessageCommandResult UpdateGroupAnnouncement(const MessageCommandRequest&) override
    {
        return {ID_UPDATE_GROUP_ANNOUNCEMENT_RSP, R"({"error":0})"};
    }

    MessageCommandResult UpdateGroupIcon(const MessageCommandRequest&) override
    {
        return {ID_UPDATE_GROUP_ICON_RSP, R"({"error":0})"};
    }

    MessageCommandResult SetGroupAdmin(const MessageCommandRequest&) override
    {
        return {ID_SET_GROUP_ADMIN_RSP, R"({"error":0})"};
    }

    MessageCommandResult MuteGroupMember(const MessageCommandRequest&) override
    {
        return {ID_MUTE_GROUP_MEMBER_RSP, R"({"error":0})"};
    }

    MessageCommandResult KickGroupMember(const MessageCommandRequest&) override
    {
        return {ID_KICK_GROUP_MEMBER_RSP, R"({"error":0})"};
    }

    MessageCommandResult QuitGroup(const MessageCommandRequest&) override
    {
        return {ID_QUIT_GROUP_RSP, R"({"error":0})"};
    }

    MessageCommandResult DissolveGroup(const MessageCommandRequest&) override
    {
        return {ID_DISSOLVE_GROUP_RSP, R"({"error":0})"};
    }

    void HandleCreateGroup(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleGetGroupList(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleInviteGroupMember(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleApplyJoinGroup(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleReviewGroupApply(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleGroupChatMessage(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleGroupHistory(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleEditGroupMessage(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleRevokeGroupMessage(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleForwardGroupMessage(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleGroupReadAck(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleUpdateGroupAnnouncement(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleUpdateGroupIcon(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleSetGroupAdmin(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleMuteGroupMember(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleKickGroupMember(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleQuitGroup(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
    void HandleDissolveGroup(const std::shared_ptr<CSession>&, short, const std::string&) override
    {
    }
};
} // namespace

TEST(MessageRemoteSmokeTest, ConfiguredGrpcBackendSelectsRemoteAdapters)
{
    const std::string config_path = EnvValue("MEMOCHAT_MESSAGE_SERVICE_SMOKE_CONFIG");
    if (config_path.empty())
    {
        GTEST_SKIP() << "MEMOCHAT_MESSAGE_SERVICE_SMOKE_CONFIG is not set";
    }

    ConfigMgr::InitConfigPath(config_path);
    MessageServiceConfig config;
    EXPECT_EQ(config.MessageServiceBackend(), "grpc");

    const auto endpoint = config.MessageServiceEndpoint();
    ASSERT_FALSE(endpoint.empty());
    const std::string expected_endpoint = EnvValue("MEMOCHAT_MESSAGE_SERVICE_SMOKE_ENDPOINT");
    if (!expected_endpoint.empty())
    {
        EXPECT_EQ(endpoint, expected_endpoint);
    }

    auto private_service = CreatePrivateMessageService(config, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    auto group_service = CreateGroupMessageService(config, nullptr, nullptr, nullptr, nullptr);

    ASSERT_NE(private_service, nullptr);
    ASSERT_NE(group_service, nullptr);
    ASSERT_NE(dynamic_cast<MessageGrpcServiceAdapter*>(private_service.get()), nullptr);
    ASSERT_NE(dynamic_cast<MessageGrpcServiceAdapter*>(group_service.get()), nullptr);
}

TEST(MessageRemoteSmokeTest, RealMessageServiceWorkerAcceptsPrivateAndGroupGrpcCommands)
{
    const std::string endpoint = EnvValue("MEMOCHAT_MESSAGE_SERVICE_SMOKE_ENDPOINT");
    if (endpoint.empty())
    {
        GTEST_SKIP() << "MEMOCHAT_MESSAGE_SERVICE_SMOKE_ENDPOINT is not set";
    }

    MessageGrpcClient client(endpoint, std::chrono::seconds(2));

    const PrivateGrpcProbeCase private_cases[] = {
        {"TextChatMessage",
         ID_TEXT_CHAT_MSG_REQ,
         ID_TEXT_CHAT_MSG_RSP,
         R"({"fromuid":1,"touid":2})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.TextChatMessage(request);
         }},
        {"ForwardPrivateMessage",
         ID_FORWARD_PRIVATE_MSG_REQ,
         ID_FORWARD_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.ForwardPrivateMessage(request);
         }},
        {"PrivateReadAck",
         ID_PRIVATE_READ_ACK_REQ,
         0,
         R"({"fromuid":1,"peer_uid":0,"read_ts":123})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.PrivateReadAck(request);
         }},
        {"EditPrivateMessage",
         ID_EDIT_PRIVATE_MSG_REQ,
         ID_EDIT_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2,"msgid":"message-remote-smoke-edit"})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.EditPrivateMessage(request);
         }},
        {"RevokePrivateMessage",
         ID_REVOKE_PRIVATE_MSG_REQ,
         ID_REVOKE_PRIVATE_MSG_RSP,
         R"({"fromuid":1,"peer_uid":2})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.RevokePrivateMessage(request);
         }},
        {"PrivateHistory",
         ID_PRIVATE_HISTORY_REQ,
         ID_PRIVATE_HISTORY_RSP,
         R"({"fromuid":1,"peer_uid":2,"limit":0})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.PrivateHistory(request);
         }},
    };

    for (const auto& one : private_cases)
    {
        SCOPED_TRACE(one.name);
        const auto request = BuildPrivateProbeRequest(one.request_msg_id, one.payload_json, one.name);
        const auto result = one.call(client, request);
        EXPECT_EQ(result.response_msg_id, one.expected_response_msg_id);
        ExpectNoRemoteErrorPayload(result.payload_json);
    }

    const GroupGrpcProbeCase group_cases[] = {
        {"GroupChatMessage",
         ID_GROUP_CHAT_MSG_REQ,
         ID_GROUP_CHAT_MSG_RSP,
         R"({"fromuid":1,"groupid":1})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.GroupChatMessage(request);
         }},
        {"GroupHistory",
         ID_GROUP_HISTORY_REQ,
         ID_GROUP_HISTORY_RSP,
         R"({"fromuid":0,"groupid":0,"limit":0})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.GroupHistory(request);
         }},
        {"GroupReadAck",
         ID_GROUP_READ_ACK_REQ,
         0,
         R"({"fromuid":0,"groupid":0,"read_ts":123})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.GroupReadAck(request);
         }},
        {"EditGroupMessage",
         ID_EDIT_GROUP_MSG_REQ,
         ID_EDIT_GROUP_MSG_RSP,
         R"({"fromuid":0,"groupid":0,"msgid":"","content":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.EditGroupMessage(request);
         }},
        {"RevokeGroupMessage",
         ID_REVOKE_GROUP_MSG_REQ,
         ID_REVOKE_GROUP_MSG_RSP,
         R"({"fromuid":0,"groupid":0,"msgid":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.RevokeGroupMessage(request);
         }},
        {"ForwardGroupMessage",
         ID_FORWARD_GROUP_MSG_REQ,
         ID_FORWARD_GROUP_MSG_RSP,
         R"({"fromuid":0,"groupid":0,"msgid":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.ForwardGroupMessage(request);
         }},
        {"CreateGroup",
         ID_CREATE_GROUP_REQ,
         ID_CREATE_GROUP_RSP,
         R"({"fromuid":0,"name":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.CreateGroup(request);
         }},
        {"GetGroupList",
         ID_GET_GROUP_LIST_REQ,
         ID_GET_GROUP_LIST_RSP,
         R"({"fromuid":0})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.GetGroupList(request);
         }},
        {"InviteGroupMember",
         ID_INVITE_GROUP_MEMBER_REQ,
         ID_INVITE_GROUP_MEMBER_RSP,
         R"({"fromuid":0,"groupid":0,"target_user_id":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.InviteGroupMember(request);
         }},
        {"ApplyJoinGroup",
         ID_APPLY_JOIN_GROUP_REQ,
         ID_APPLY_JOIN_GROUP_RSP,
         R"({"fromuid":0,"group_code":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.ApplyJoinGroup(request);
         }},
        {"ReviewGroupApply",
         ID_REVIEW_GROUP_APPLY_REQ,
         ID_REVIEW_GROUP_APPLY_RSP,
         R"({"fromuid":0,"apply_id":0,"agree":false})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.ReviewGroupApply(request);
         }},
        {"UpdateGroupAnnouncement",
         ID_UPDATE_GROUP_ANNOUNCEMENT_REQ,
         ID_UPDATE_GROUP_ANNOUNCEMENT_RSP,
         R"({"fromuid":0,"groupid":0,"announcement":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.UpdateGroupAnnouncement(request);
         }},
        {"UpdateGroupIcon",
         ID_UPDATE_GROUP_ICON_REQ,
         ID_UPDATE_GROUP_ICON_RSP,
         R"({"fromuid":0,"groupid":0,"icon":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.UpdateGroupIcon(request);
         }},
        {"SetGroupAdmin",
         ID_SET_GROUP_ADMIN_REQ,
         ID_SET_GROUP_ADMIN_RSP,
         R"({"fromuid":0,"groupid":0,"target_user_id":"","is_admin":true})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.SetGroupAdmin(request);
         }},
        {"MuteGroupMember",
         ID_MUTE_GROUP_MEMBER_REQ,
         ID_MUTE_GROUP_MEMBER_RSP,
         R"({"fromuid":0,"groupid":0,"target_user_id":"","mute_seconds":0})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.MuteGroupMember(request);
         }},
        {"KickGroupMember",
         ID_KICK_GROUP_MEMBER_REQ,
         ID_KICK_GROUP_MEMBER_RSP,
         R"({"fromuid":0,"groupid":0,"target_user_id":""})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.KickGroupMember(request);
         }},
        {"QuitGroup",
         ID_QUIT_GROUP_REQ,
         ID_QUIT_GROUP_RSP,
         R"({"fromuid":0,"groupid":0})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.QuitGroup(request);
         }},
        {"DissolveGroup",
         ID_DISSOLVE_GROUP_REQ,
         ID_DISSOLVE_GROUP_RSP,
         R"({"fromuid":0,"groupid":0})",
         [](MessageGrpcClient& grpc_client, const MessageCommandRequest& request)
         {
             return grpc_client.DissolveGroup(request);
         }},
    };

    for (const auto& one : group_cases)
    {
        SCOPED_TRACE(one.name);
        const auto request = BuildGroupProbeRequest(one.request_msg_id, one.payload_json, one.name);
        const auto result = one.call(client, request);
        EXPECT_EQ(result.response_msg_id, one.expected_response_msg_id);
        ExpectNoRemoteErrorPayload(result.payload_json);
    }

    memochat::json::JsonValue group_list_payload(memochat::json::object_t{});
    client.BuildGroupListJson(1, group_list_payload);
    EXPECT_FALSE(group_list_payload.isMember("message_remote_method"));
    EXPECT_FALSE(group_list_payload.isMember("message_remote_error"));
    EXPECT_FALSE(group_list_payload.isMember("message_remote_status_code"));
}
