#include "GroupMessageService.h"

#include "ChatRuntime.h"
#include "CSession.h"
#include "logging/Logger.h"

#include <chrono>
#include <iostream>
#include "json/GlazeCompat.h"
#include <unordered_set>

namespace
{
constexpr int64_t kPermChangeGroupInfoLocal = 1LL << 0;
constexpr int64_t kPermDeleteMessagesLocal = 1LL << 1;
constexpr int64_t kPermInviteUsersLocal = 1LL << 2;
constexpr int64_t kPermManageAdminsLocal = 1LL << 3;
constexpr int64_t kPermPinMessagesLocal = 1LL << 4;
constexpr int64_t kPermBanUsersLocal = 1LL << 5;
constexpr int64_t kPermManageTopicsLocal = 1LL << 6;
constexpr int64_t kDefaultAdminPermBitsLocal = kPermChangeGroupInfoLocal | kPermDeleteMessagesLocal |
                                               kPermInviteUsersLocal | kPermPinMessagesLocal | kPermBanUsersLocal;

int64_t NowMsGroupLocal()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

int64_t NowSecGroupLocal()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

bool ParseJsonObjectGroupLocal(const std::string& payload, memochat::json::JsonValue& root)
{
    memochat::json::JsonCharReaderBuilder builder;
    std::unique_ptr<memochat::json::JsonCharReader> reader(builder.newCharReader());
    std::string errors;
    return reader->parse(payload.data(), payload.data() + payload.size(), &root, &errors) && root.is_object();
}

// Compact wire JSON for TCP/QUIC transport (Qt QJsonDocument is strict).
std::string JsonToWireString(const memochat::json::JsonValue& v)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, v);
}

bool KafkaBackendEnabledGroupLocal()
{
    return memochat::chatruntime::AsyncEventBusBackend() == "kafka";
}

MessageCommandRequest BuildGroupMessageCommandRequestLocal(const std::shared_ptr<CSession>& session,
                                                           short msg_id,
                                                           const std::string& msg_data)
{
    MessageCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = msg_data;
    request.server_name = memochat::chatruntime::SelfServerName();
    if (session)
    {
        request.session_uid = session->GetUserId();
        request.session_id = session->GetSessionId();
    }
    return request;
}

void SendGroupMessageCommandResultLocal(const std::shared_ptr<CSession>& session, const MessageCommandResult& result)
{
    if (!session || result.response_msg_id == 0)
    {
        return;
    }
    session->Send(result.payload_json, result.response_msg_id);
}
} // namespace

GroupMessageService::GroupMessageService(IMessageRepository* message_repository,
                                         IRelationRepository* relation_repository,
                                         IDeliveryGateway* delivery_gateway,
                                         IEventPublisher* event_publisher)
    : _message_repository(message_repository)
    , _relation_repository(relation_repository)
    , _delivery_gateway(delivery_gateway)
    , _event_publisher(event_publisher)
{
}

void GroupMessageService::BuildGroupListJson(int uid, memochat::json::JsonValue& out)
{
    out["group_list"] = memochat::json::arrayValue;
    out["pending_group_apply_list"] = memochat::json::arrayValue;

    std::vector<std::shared_ptr<GroupInfo>> groups;
    _relation_repository->GetUserGroupList(uid, groups);
    for (const auto& group : groups)
    {
        if (!group)
        {
            continue;
        }
        memochat::json::JsonValue one;
        one["groupid"] = static_cast<int64_t>(group->group_id);
        one["group_code"] = group->group_code;
        one["name"] = group->name;
        one["icon"] = group->icon;
        one["owner_uid"] = group->owner_uid;
        one["announcement"] = group->announcement;
        one["member_limit"] = group->member_limit;
        one["member_count"] = group->member_count;
        one["is_all_muted"] = group->is_all_muted;
        one["role"] = group->role;
        one["permission_bits"] = static_cast<int64_t>(group->permission_bits);
        one["can_change_group_info"] = (group->permission_bits & kPermChangeGroupInfoLocal) != 0;
        one["can_delete_messages"] = (group->permission_bits & kPermDeleteMessagesLocal) != 0;
        one["can_invite_users"] = (group->permission_bits & kPermInviteUsersLocal) != 0;
        one["can_manage_admins"] = (group->permission_bits & kPermManageAdminsLocal) != 0;
        one["can_pin_messages"] = (group->permission_bits & kPermPinMessagesLocal) != 0;
        one["can_ban_users"] = (group->permission_bits & kPermBanUsersLocal) != 0;
        one["can_manage_topics"] = (group->permission_bits & kPermManageTopicsLocal) != 0;
        out["group_list"].append(one);
    }

    std::vector<std::shared_ptr<GroupApplyInfo>> applies;
    _relation_repository->GetPendingGroupApplyForReviewer(uid, applies, 30);
    for (const auto& apply : applies)
    {
        if (!apply)
        {
            continue;
        }
        memochat::json::JsonValue one;
        one["apply_id"] = static_cast<int64_t>(apply->apply_id);
        one["groupid"] = static_cast<int64_t>(apply->group_id);
        std::shared_ptr<GroupInfo> group_info;
        if (_relation_repository->GetGroupById(apply->group_id, group_info) && group_info)
        {
            one["group_code"] = group_info->group_code;
        }
        one["applicant_uid"] = apply->applicant_uid;
        one["inviter_uid"] = apply->inviter_uid;
        auto applicant = _relation_repository->GetUserByUid(apply->applicant_uid);
        if (applicant)
        {
            one["applicant_user_id"] = applicant->user_id;
        }
        if (apply->inviter_uid > 0)
        {
            auto inviter = _relation_repository->GetUserByUid(apply->inviter_uid);
            if (inviter)
            {
                one["inviter_user_id"] = inviter->user_id;
            }
        }
        one["type"] = apply->type;
        one["status"] = apply->status;
        one["reason"] = apply->reason;
        out["pending_group_apply_list"].append(one);
    }
}

MessageCommandResult GroupMessageService::CreateGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);

    const int owner_uid = root["fromuid"].asInt();
    const std::string group_name = root["name"].asString();
    const std::string announcement = root.get("announcement", "").asString();
    const int member_limit = root.get("member_limit", 200).asInt();
    std::vector<int> members;
    std::unordered_set<int> member_set;
    bool invalid_member_user_id = false;
    if (isMember(root, "member_user_ids") && root["member_user_ids"].is_array())
    {
        for (const auto& one : root["member_user_ids"])
        {
            const std::string member_user_id = one.asString();
            int uid = 0;
            if (!_relation_repository->GetUidByUserId(member_user_id, uid) || uid <= 0)
            {
                invalid_member_user_id = true;
                break;
            }
            if (uid == owner_uid)
            {
                continue;
            }
            member_set.insert(uid);
        }
    }
    for (int uid : member_set)
    {
        members.push_back(uid);
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_CREATE_GROUP_RSP, JsonToWireString(rtvalue)};
    };

    if (owner_uid <= 0 || group_name.empty() || group_name.size() > 64 || invalid_member_user_id)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    for (int uid : members)
    {
        if (!_relation_repository->IsPrivateFriend(owner_uid, uid))
        {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return result();
        }
    }

    int64_t group_id = 0;
    std::string group_code;
    if (!_relation_repository
             ->CreateGroup(owner_uid, group_name, announcement, member_limit, members, group_id, group_code) ||
        group_id <= 0)
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return result();
    }

    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["group_code"] = group_code;
    rtvalue["name"] = group_name;
    rtvalue["announcement"] = announcement;
    BuildGroupListJson(owner_uid, rtvalue);

    if (!members.empty())
    {
        memochat::json::JsonValue notify;
        notify["error"] = ErrorCodes::Success;
        notify["event"] = "group_invite";
        notify["groupid"] = static_cast<int64_t>(group_id);
        notify["group_code"] = group_code;
        notify["name"] = group_name;
        notify["operator_uid"] = owner_uid;
        _delivery_gateway->PushPayload(members, ID_NOTIFY_GROUP_INVITE_REQ, notify);
    }
    return result();
}

void GroupMessageService::HandleCreateGroup(const std::shared_ptr<CSession>& session,
                                            short msg_id,
                                            const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       CreateGroup(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::GetGroupList(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_GET_GROUP_LIST_RSP, JsonToWireString(rtvalue)};
    };
    if (uid <= 0)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }

    BuildGroupListJson(uid, rtvalue);
    return result();
}

void GroupMessageService::HandleGetGroupList(const std::shared_ptr<CSession>& session,
                                             short msg_id,
                                             const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       GetGroupList(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::InviteGroupMember(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int from_uid = root["fromuid"].asInt();
    const std::string target_user_id = root.get("target_user_id", "").asString();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string reason = root.get("reason", "").asString();
    int to_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, to_uid))
    {
        to_uid = 0;
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["touid"] = to_uid;
    rtvalue["target_user_id"] = target_user_id;
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_INVITE_GROUP_MEMBER_RSP, JsonToWireString(rtvalue)};
    };

    if (from_uid <= 0 || to_uid <= 0 || group_id <= 0 || target_user_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_relation_repository->InviteGroupMember(group_id, from_uid, to_uid, reason))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_invite";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["name"] = group_info ? group_info->name : "";
    notify["operator_uid"] = from_uid;
    notify["target_user_id"] = target_user_id;
    notify["reason"] = reason;
    _delivery_gateway->PushPayload({to_uid}, ID_NOTIFY_GROUP_INVITE_REQ, notify);
    return result();
}

void GroupMessageService::HandleInviteGroupMember(const std::shared_ptr<CSession>& session,
                                                  short msg_id,
                                                  const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        InviteGroupMember(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::ApplyJoinGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int from_uid = root["fromuid"].asInt();
    const std::string group_code = root.get("group_code", "").asString();
    int64_t group_id = 0;
    if (!_relation_repository->GetGroupIdByCode(group_code, group_id))
    {
        group_id = 0;
    }
    const std::string reason = root.get("reason", "").asString();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["group_code"] = group_code;
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_APPLY_JOIN_GROUP_RSP, JsonToWireString(rtvalue)};
    };

    if (from_uid <= 0 || group_id <= 0 || group_code.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_relation_repository->ApplyJoinGroup(group_id, from_uid, reason))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> admins;
    for (const auto& one : members)
    {
        if (one && one->role >= 2)
        {
            admins.push_back(one->uid);
        }
    }
    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_apply";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_code;
    notify["applicant_uid"] = from_uid;
    auto applicant = _relation_repository->GetUserByUid(from_uid);
    if (applicant)
    {
        notify["applicant_user_id"] = applicant->user_id;
    }
    notify["reason"] = reason;
    _delivery_gateway->PushPayload(admins, ID_NOTIFY_GROUP_APPLY_REQ, notify);
    return result();
}

void GroupMessageService::HandleApplyJoinGroup(const std::shared_ptr<CSession>& session,
                                               short msg_id,
                                               const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       ApplyJoinGroup(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::ReviewGroupApply(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int reviewer_uid = root["fromuid"].asInt();
    const int64_t apply_id = root["apply_id"].asInt64();
    const bool agree = memochat::json::glaze_safe_get<bool>(root, "agree", false);

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["apply_id"] = static_cast<int64_t>(apply_id);
    rtvalue["agree"] = agree;
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_REVIEW_GROUP_APPLY_RSP, JsonToWireString(rtvalue)};
    };

    if (reviewer_uid <= 0 || apply_id <= 0)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }

    std::shared_ptr<GroupApplyInfo> apply_info;
    if (!_relation_repository->ReviewGroupApply(apply_id, reviewer_uid, agree, apply_info) || !apply_info)
    {
        rtvalue["error"] = ErrorCodes::GroupApplyNotFound;
        return result();
    }

    rtvalue["groupid"] = static_cast<int64_t>(apply_info->group_id);
    rtvalue["applicant_uid"] = apply_info->applicant_uid;
    std::shared_ptr<GroupInfo> apply_group;
    if (_relation_repository->GetGroupById(apply_info->group_id, apply_group) && apply_group)
    {
        rtvalue["group_code"] = apply_group->group_code;
    }
    auto applicant = _relation_repository->GetUserByUid(apply_info->applicant_uid);
    if (applicant)
    {
        rtvalue["applicant_user_id"] = applicant->user_id;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(apply_info->group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one)
        {
            recipients.push_back(one->uid);
        }
    }
    recipients.push_back(apply_info->applicant_uid);

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_member_changed";
    notify["groupid"] = static_cast<int64_t>(apply_info->group_id);
    notify["group_code"] = apply_group ? apply_group->group_code : "";
    notify["applicant_uid"] = apply_info->applicant_uid;
    if (applicant)
    {
        notify["applicant_user_id"] = applicant->user_id;
    }
    notify["agree"] = agree;
    notify["operator_uid"] = reviewer_uid;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

void GroupMessageService::HandleReviewGroupApply(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        ReviewGroupApply(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::GroupChatMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonValue root;
    ParseJsonObjectGroupLocal(request.payload_json, root);
    const int from_uid = memochat::json::glaze_safe_get<int>(root, "fromuid", 0);
    const int64_t group_id = memochat::json::glaze_safe_get<int64_t>(root, "groupid", 0);
    const auto msg = root.get("msg");
    const std::string client_msg_id = memochat::json::glaze_safe_get<std::string>(msg, "msgid", "");
    const bool kafka_backend = KafkaBackendEnabledGroupLocal();
    const bool kafka_primary = kafka_backend && memochat::chatruntime::FeatureEnabled("chat_group_kafka_primary");
    const bool kafka_shadow =
        kafka_backend && !kafka_primary && memochat::chatruntime::FeatureEnabled("chat_group_kafka_shadow");

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = from_uid;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    if (!client_msg_id.empty())
    {
        rtvalue["client_msg_id"] = client_msg_id;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_GROUP_CHAT_MSG_RSP, JsonToWireString(rtvalue)};
    };

    if (from_uid <= 0 || group_id <= 0 || !msg.isObject())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }

    int role = 0;
    if (!_relation_repository->GetUserRoleInGroup(group_id, from_uid, role))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    const auto now_sec = NowSecGroupLocal();
    const auto now_ms = NowMsGroupLocal();
    for (const auto& member : members)
    {
        if (member && member->uid == from_uid && member->mute_until > now_sec)
        {
            rtvalue["error"] = ErrorCodes::GroupMuted;
            return result();
        }
    }

    const bool mention_all = memochat::json::glaze_safe_get<bool>(msg, "mention_all", false);
    if (mention_all && role < 2)
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    GroupMessageInfo info;
    info.msg_id = client_msg_id;
    info.group_id = group_id;
    info.from_uid = from_uid;
    info.msg_type = memochat::json::glaze_safe_get<std::string>(msg, "msgtype", "text");
    info.content = memochat::json::glaze_safe_get<std::string>(msg, "content", "");
    if (memochat::json::isMember(msg, "mentions"))
    {
        info.mentions_json = msg["mentions"].toStyledString();
    }
    else
    {
        info.mentions_json = "[]";
    }
    info.file_name = memochat::json::glaze_safe_get<std::string>(msg, "file_name", "");
    info.mime = memochat::json::glaze_safe_get<std::string>(msg, "mime", "");
    info.size = memochat::json::glaze_safe_get<int>(msg, "size", 0);
    info.reply_to_server_msg_id = memochat::json::glaze_safe_get<int64_t>(msg, "reply_to_server_msg_id", 0);
    if (memochat::json::isMember(msg, "forward_meta"))
    {
        info.forward_meta_json = msg["forward_meta"].toStyledString();
    }
    info.edited_at_ms = memochat::json::glaze_safe_get<int64_t>(msg, "edited_at_ms", 0);
    info.deleted_at_ms = memochat::json::glaze_safe_get<int64_t>(msg, "deleted_at_ms", 0);
    info.created_at = now_ms;
    if (info.msg_id.empty() || info.content.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }

    const auto accept_ts = NowMsGroupLocal();
    rtvalue["accept_node"] = memochat::chatruntime::SelfServerName();
    rtvalue["accept_ts"] = static_cast<int64_t>(accept_ts);
    rtvalue["status"] = kafka_primary ? "accepted" : "persisted";

    auto sender_info = _relation_repository->GetUserByUid(from_uid);
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (sender_info)
    {
        rtvalue["from_name"] = sender_info->name;
        rtvalue["from_nick"] = sender_info->nick;
        rtvalue["from_icon"] = sender_info->icon;
        rtvalue["from_user_id"] = sender_info->user_id;
    }
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }

    memochat::json::JsonValue event_payload;
    event_payload["fromuid"] = from_uid;
    event_payload["groupid"] = static_cast<int64_t>(group_id);
    event_payload["trace_id"] = root.get("trace_id", "").asString();
    event_payload["request_id"] = root.get("request_id", "").asString();
    event_payload["span_id"] = root.get("span_id", "").asString();
    event_payload["event_id"] = client_msg_id;
    event_payload["accept_node"] = memochat::chatruntime::SelfServerName();
    event_payload["accept_ts"] = static_cast<int64_t>(accept_ts);
    event_payload["msg"] = msg;
    if (sender_info)
    {
        event_payload["from_name"] = sender_info->name;
        event_payload["from_nick"] = sender_info->nick;
        event_payload["from_icon"] = sender_info->icon;
        event_payload["from_user_id"] = sender_info->user_id;
    }
    if (group_info)
    {
        event_payload["group_code"] = group_info->group_code;
    }

    if (kafka_primary || kafka_shadow)
    {
        std::string publish_error;
        if (!_event_publisher ||
            !_event_publisher->PublishEvent(memochat::chatruntime::TopicGroup(), event_payload, &publish_error))
        {
            if (kafka_primary)
            {
                rtvalue["error"] = ErrorCodes::RPCFailed;
                rtvalue["status"] = "failed";
                return result();
            }
            memolog::LogWarn("chat.group.shadow_publish_failed",
                             "group shadow publish failed",
                             {{"error", publish_error}, {"client_msg_id", client_msg_id}});
        }
    }

    if (kafka_primary)
    {
        return result();
    }

    if (sender_info)
    {
        info.from_name = sender_info->name;
        info.from_nick = sender_info->nick;
        info.from_icon = sender_info->icon;
    }
    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    if (!_message_repository->SaveGroupMessage(info, &server_msg_id, &group_seq))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        rtvalue["status"] = "failed";
        return result();
    }
    info.server_msg_id = server_msg_id;
    info.group_seq = group_seq;

    memochat::json::JsonValue msg_out = msg;
    msg_out["created_at"] = static_cast<int64_t>(now_ms);
    msg_out["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    msg_out["group_seq"] = static_cast<int64_t>(group_seq);
    rtvalue["msg"] = msg_out;
    rtvalue["created_at"] = static_cast<int64_t>(now_ms);
    rtvalue["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    rtvalue["group_seq"] = static_cast<int64_t>(group_seq);

    std::vector<int> recipients;
    for (const auto& member : members)
    {
        if (member && member->status == 1)
        {
            recipients.push_back(member->uid);
        }
    }
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_CHAT_MSG_REQ, rtvalue, from_uid);
    return result();
}

void GroupMessageService::HandleGroupChatMessage(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        GroupChatMessage(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::GroupHistory(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const int64_t before_ts = root.get("before_ts", 0).asInt64();
    const int64_t before_seq = root.get("before_seq", 0).asInt64();
    const int limit = root.get("limit", 20).asInt();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["has_more"] = false;
    rtvalue["next_before_seq"] = static_cast<int64_t>(0);
    rtvalue["messages"] = memochat::json::arrayValue;
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_GROUP_HISTORY_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || group_id <= 0 || !_relation_repository->IsGroupMember(group_id, uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMessageInfo>> msgs;
    bool has_more = false;
    const bool repository_success =
        _message_repository->GetGroupHistory(group_id, before_ts, before_seq, limit, msgs, has_more);
    memolog::LogInfo("chat.group.history.query",
                     "group history query completed",
                     {{"uid", std::to_string(uid)},
                      {"group_id", std::to_string(group_id)},
                      {"before_ts", std::to_string(before_ts)},
                      {"before_seq", std::to_string(before_seq)},
                      {"limit", std::to_string(limit)},
                      {"repository_success", repository_success ? "true" : "false"},
                      {"final_count", std::to_string(msgs.size())},
                      {"has_more", has_more ? "true" : "false"}});
    if (!repository_success)
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return result();
    }
    rtvalue["has_more"] = has_more;
    std::shared_ptr<GroupInfo> group_info;
    if (_relation_repository->GetGroupById(group_id, group_info) && group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }

    for (const auto& one : msgs)
    {
        if (!one)
        {
            continue;
        }
        memochat::json::JsonValue item;
        item["msgid"] = one->msg_id;
        item["groupid"] = static_cast<int64_t>(one->group_id);
        item["fromuid"] = one->from_uid;
        item["msgtype"] = one->msg_type;
        item["content"] = one->content;
        memochat::json::JsonValue mentions(memochat::json::array_t{});
        if (!one->mentions_json.empty())
        {
            memochat::json::JsonReader mentions_reader;
            memochat::json::JsonValue parsed_mentions;
            if (mentions_reader.parse(one->mentions_json, parsed_mentions) && parsed_mentions.is_array())
            {
                mentions = parsed_mentions;
            }
        }
        item["mentions"] = mentions;
        if (!one->file_name.empty())
        {
            item["file_name"] = one->file_name;
        }
        if (!one->mime.empty())
        {
            item["mime"] = one->mime;
        }
        if (one->size > 0)
        {
            item["size"] = one->size;
        }
        item["created_at"] = static_cast<int64_t>(one->created_at);
        item["server_msg_id"] = static_cast<int64_t>(one->server_msg_id);
        item["group_seq"] = static_cast<int64_t>(one->group_seq);
        if (one->reply_to_server_msg_id > 0)
        {
            item["reply_to_server_msg_id"] = static_cast<int64_t>(one->reply_to_server_msg_id);
        }
        if (!one->forward_meta_json.empty())
        {
            memochat::json::JsonReader forward_reader;
            memochat::json::JsonValue forward_meta;
            if (forward_reader.parse(one->forward_meta_json, forward_meta))
            {
                item["forward_meta"] = forward_meta;
            }
        }
        if (one->edited_at_ms > 0)
        {
            item["edited_at_ms"] = static_cast<int64_t>(one->edited_at_ms);
        }
        if (one->deleted_at_ms > 0)
        {
            item["deleted_at_ms"] = static_cast<int64_t>(one->deleted_at_ms);
        }
        item["from_name"] = one->from_name;
        item["from_nick"] = one->from_nick;
        item["from_icon"] = one->from_icon;
        if ((one->from_name.empty() || one->from_nick.empty()) && one->from_uid > 0)
        {
            auto from_user = _relation_repository->GetUserByUid(one->from_uid);
            if (from_user)
            {
                item["from_name"] = from_user->name;
                item["from_nick"] = from_user->nick;
                item["from_icon"] = from_user->icon;
            }
        }
        append(rtvalue["messages"], item);
    }
    if (!msgs.empty() && msgs.back())
    {
        rtvalue["next_before_seq"] = static_cast<int64_t>(msgs.back()->group_seq);
    }
    int64_t read_ts = 0;
    if (!msgs.empty() && msgs.front())
    {
        read_ts = msgs.front()->created_at;
    }
    if (read_ts <= 0)
    {
        read_ts = NowMsGroupLocal();
    }
    _message_repository->UpsertGroupReadState(uid, group_id, read_ts);
    return result();
}

void GroupMessageService::HandleGroupHistory(const std::shared_ptr<CSession>& session,
                                             short msg_id,
                                             const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       GroupHistory(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::EditGroupMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string target_msg_id = root.get("msgid", "").asString();
    const std::string content = root.get("content", "").asString();
    const int64_t now_ms = NowMsGroupLocal();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["msgid"] = target_msg_id;
    rtvalue["content"] = content;
    rtvalue["edited_at_ms"] = static_cast<int64_t>(now_ms);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_EDIT_GROUP_MSG_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || group_id <= 0 || target_msg_id.empty() || content.empty() || content.size() > 4096)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_relation_repository->IsGroupMember(group_id, uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }
    if (!_message_repository->UpdateGroupMessageContent(group_id, uid, target_msg_id, content, now_ms))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members)
    {
        if (member && member->status == 1)
        {
            recipients.push_back(member->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_msg_edited";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["msgid"] = target_msg_id;
    notify["content"] = content;
    notify["edited_at_ms"] = static_cast<int64_t>(now_ms);
    notify["operator_uid"] = uid;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, 0);
    return result();
}

void GroupMessageService::HandleEditGroupMessage(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        EditGroupMessage(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::RevokeGroupMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string target_msg_id = root.get("msgid", "").asString();
    const int64_t now_ms = NowMsGroupLocal();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["msgid"] = target_msg_id;
    rtvalue["content"] = "[消息已撤回]";
    rtvalue["deleted_at_ms"] = static_cast<int64_t>(now_ms);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_REVOKE_GROUP_MSG_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || group_id <= 0 || target_msg_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_relation_repository->IsGroupMember(group_id, uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }
    if (!_message_repository->RevokeGroupMessage(group_id, uid, target_msg_id, now_ms))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members)
    {
        if (member && member->status == 1)
        {
            recipients.push_back(member->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_msg_revoked";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["msgid"] = target_msg_id;
    notify["content"] = "[消息已撤回]";
    notify["deleted_at_ms"] = static_cast<int64_t>(now_ms);
    notify["operator_uid"] = uid;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, 0);
    return result();
}

void GroupMessageService::HandleRevokeGroupMessage(const std::shared_ptr<CSession>& session,
                                                   short msg_id,
                                                   const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        RevokeGroupMessage(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::ForwardGroupMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int from_uid = root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();
    const int64_t group_id = root.get("groupid", 0).asInt64();
    const std::string source_msg_id = root.get("msgid", "").asString();
    std::string client_msg_id = root.get("client_msg_id", "").asString();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = from_uid;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    if (!client_msg_id.empty())
    {
        rtvalue["client_msg_id"] = client_msg_id;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_FORWARD_GROUP_MSG_RSP, JsonToWireString(rtvalue)};
    };

    if (from_uid <= 0 || group_id <= 0 || source_msg_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_relation_repository->IsGroupMember(group_id, from_uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::shared_ptr<GroupMessageInfo> source_msg;
    if (!_message_repository->FindGroupMessageByClientId(group_id, source_msg_id, source_msg))
    {
        rtvalue["error"] = ErrorCodes::GroupNotFound;
        return result();
    }

    const int64_t now_ms = NowMsGroupLocal();
    if (client_msg_id.empty())
    {
        client_msg_id = std::to_string(from_uid) + "_" + std::to_string(now_ms);
        rtvalue["client_msg_id"] = client_msg_id;
    }

    GroupMessageInfo info;
    info.msg_id = client_msg_id;
    info.group_id = group_id;
    info.from_uid = from_uid;
    info.msg_type = source_msg->msg_type;
    info.content = source_msg->content;
    info.mentions_json = source_msg->mentions_json;
    info.file_name = source_msg->file_name;
    info.mime = source_msg->mime;
    info.size = source_msg->size;
    info.reply_to_server_msg_id = source_msg->reply_to_server_msg_id;
    memochat::json::JsonValue forward_meta;
    forward_meta["forwarded_from_msgid"] = source_msg_id;
    forward_meta["source_server_msg_id"] = static_cast<int64_t>(source_msg->server_msg_id);
    forward_meta["source_group_seq"] = static_cast<int64_t>(source_msg->group_seq);
    forward_meta["source_from_uid"] = source_msg->from_uid;
    if (!source_msg->forward_meta_json.empty())
    {
        memochat::json::JsonReader prev_forward_reader;
        memochat::json::JsonValue prev_forward_meta;
        if (prev_forward_reader.parse(source_msg->forward_meta_json, prev_forward_meta))
        {
            forward_meta["prev_forward_meta"] = prev_forward_meta;
        }
    }
    info.forward_meta_json = forward_meta
                                 .and_then(
                                     [](auto&& v)
                                     {
                                         return glz::write_json(v);
                                     })
                                 .value_or("{}");
    info.created_at = now_ms;

    auto sender_info = _relation_repository->GetUserByUid(from_uid);
    if (sender_info)
    {
        info.from_name = sender_info->name;
        info.from_nick = sender_info->nick;
        info.from_icon = sender_info->icon;
    }
    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    if (!_message_repository->SaveGroupMessage(info, &server_msg_id, &group_seq))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return result();
    }
    info.server_msg_id = server_msg_id;
    info.group_seq = group_seq;

    memochat::json::JsonValue forwarded_msg;
    forwarded_msg["msgid"] = info.msg_id;
    forwarded_msg["msgtype"] = info.msg_type;
    forwarded_msg["content"] = info.content;
    memochat::json::JsonValue mentions(memochat::json::array_t{});
    if (!info.mentions_json.empty())
    {
        memochat::json::JsonReader mentions_reader;
        memochat::json::JsonValue parsed_mentions;
        if (mentions_reader.parse(info.mentions_json, parsed_mentions) && parsed_mentions.is_array())
        {
            mentions = parsed_mentions;
        }
    }
    forwarded_msg["mentions"] = mentions;
    if (!info.file_name.empty())
    {
        forwarded_msg["file_name"] = info.file_name;
    }
    if (!info.mime.empty())
    {
        forwarded_msg["mime"] = info.mime;
    }
    if (info.size > 0)
    {
        forwarded_msg["size"] = info.size;
    }
    forwarded_msg["forwarded_from_msgid"] = source_msg_id;
    forwarded_msg["created_at"] = static_cast<int64_t>(now_ms);
    forwarded_msg["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    forwarded_msg["group_seq"] = static_cast<int64_t>(group_seq);
    if (info.reply_to_server_msg_id > 0)
    {
        forwarded_msg["reply_to_server_msg_id"] = static_cast<int64_t>(info.reply_to_server_msg_id);
    }
    {
        memochat::json::JsonReader forward_reader;
        memochat::json::JsonValue parsed_forward_meta;
        if (forward_reader.parse(info.forward_meta_json, parsed_forward_meta))
        {
            forwarded_msg["forward_meta"] = parsed_forward_meta;
        }
    }

    rtvalue["msg"] = forwarded_msg;
    rtvalue["created_at"] = static_cast<int64_t>(now_ms);
    rtvalue["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    rtvalue["group_seq"] = static_cast<int64_t>(group_seq);
    if (info.reply_to_server_msg_id > 0)
    {
        rtvalue["reply_to_server_msg_id"] = static_cast<int64_t>(info.reply_to_server_msg_id);
    }
    rtvalue["forward_meta"] = forward_meta;

    if (sender_info)
    {
        rtvalue["from_name"] = sender_info->name;
        rtvalue["from_nick"] = sender_info->nick;
        rtvalue["from_icon"] = sender_info->icon;
        rtvalue["from_user_id"] = sender_info->user_id;
    }
    std::shared_ptr<GroupInfo> group_info;
    if (_relation_repository->GetGroupById(group_id, group_info) && group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members)
    {
        if (member && member->status == 1)
        {
            recipients.push_back(member->uid);
        }
    }
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_CHAT_MSG_REQ, rtvalue, from_uid);
    return result();
}

void GroupMessageService::HandleForwardGroupMessage(const std::shared_ptr<CSession>& session,
                                                    short msg_id,
                                                    const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        ForwardGroupMessage(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::GroupReadAck(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();
    const int64_t group_id = root.get("groupid", 0).asInt64();
    int64_t read_ts = root.get("read_ts", 0).asInt64();
    const auto result = []()
    {
        return MessageCommandResult{0, "{}"};
    };
    if (uid <= 0 || group_id <= 0)
    {
        return result();
    }
    if (!_relation_repository->IsGroupMember(group_id, uid))
    {
        return result();
    }
    if (read_ts <= 0)
    {
        read_ts = NowMsGroupLocal();
    }
    _message_repository->UpsertGroupReadState(uid, group_id, read_ts);

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members)
    {
        if (member && member->status == 1)
        {
            recipients.push_back(member->uid);
        }
    }
    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_read_ack";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["fromuid"] = uid;
    notify["read_ts"] = static_cast<int64_t>(read_ts);
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, uid);
    return result();
}

void GroupMessageService::HandleGroupReadAck(const std::shared_ptr<CSession>& session,
                                             short msg_id,
                                             const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       GroupReadAck(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::UpdateGroupAnnouncement(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string announcement = root.get("announcement", "").asString();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["announcement"] = announcement;
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_UPDATE_GROUP_ANNOUNCEMENT_RSP, JsonToWireString(rtvalue)};
    };

    if (!_relation_repository->UpdateGroupAnnouncement(group_id, uid, announcement))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one)
        {
            recipients.push_back(one->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_announcement_updated";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["announcement"] = announcement;
    notify["operator_uid"] = uid;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

void GroupMessageService::HandleUpdateGroupAnnouncement(const std::shared_ptr<CSession>& session,
                                                        short msg_id,
                                                        const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        UpdateGroupAnnouncement(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::UpdateGroupIcon(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string icon = root.get("icon", "").asString();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["icon"] = icon;
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_UPDATE_GROUP_ICON_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || group_id <= 0 || icon.empty() || icon.size() > 512)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_relation_repository->UpdateGroupIcon(group_id, uid, icon))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one)
        {
            recipients.push_back(one->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_icon_updated";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["icon"] = icon;
    notify["operator_uid"] = uid;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

void GroupMessageService::HandleUpdateGroupIcon(const std::shared_ptr<CSession>& session,
                                                short msg_id,
                                                const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        UpdateGroupIcon(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::SetGroupAdmin(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const std::string target_user_id = root.get("target_user_id", "").asString();
    const int64_t group_id = root["groupid"].asInt64();
    const bool is_admin = memochat::json::glaze_safe_get<bool>(root, "is_admin", true);
    bool has_permission_bits = isMember(root, "permission_bits");
    int64_t permission_bits = root.get("permission_bits", 0).asInt64();
    auto merge_perm_flag = [&](const char* key, int64_t bit)
    {
        if (!root.isMember(key))
        {
            return;
        }
        has_permission_bits = true;
        if (memochat::json::glaze_safe_get<bool>(root, key, false))
        {
            permission_bits |= bit;
        }
        else
        {
            permission_bits &= ~bit;
        }
    };
    merge_perm_flag("can_change_group_info", kPermChangeGroupInfoLocal);
    merge_perm_flag("can_delete_messages", kPermDeleteMessagesLocal);
    merge_perm_flag("can_invite_users", kPermInviteUsersLocal);
    merge_perm_flag("can_manage_admins", kPermManageAdminsLocal);
    merge_perm_flag("can_pin_messages", kPermPinMessagesLocal);
    merge_perm_flag("can_ban_users", kPermBanUsersLocal);
    merge_perm_flag("can_manage_topics", kPermManageTopicsLocal);
    if (!is_admin)
    {
        permission_bits = 0;
        has_permission_bits = true;
    }
    int64_t requested_permission_bits = has_permission_bits ? permission_bits : 0;
    if (is_admin && !has_permission_bits && requested_permission_bits <= 0)
    {
        requested_permission_bits = kDefaultAdminPermBitsLocal;
    }
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["touid"] = target_uid;
    rtvalue["target_user_id"] = target_user_id;
    rtvalue["is_admin"] = is_admin;
    rtvalue["permission_bits"] = static_cast<int64_t>(requested_permission_bits);
    rtvalue["can_change_group_info"] = (requested_permission_bits & kPermChangeGroupInfoLocal) != 0;
    rtvalue["can_delete_messages"] = (requested_permission_bits & kPermDeleteMessagesLocal) != 0;
    rtvalue["can_invite_users"] = (requested_permission_bits & kPermInviteUsersLocal) != 0;
    rtvalue["can_manage_admins"] = (requested_permission_bits & kPermManageAdminsLocal) != 0;
    rtvalue["can_pin_messages"] = (requested_permission_bits & kPermPinMessagesLocal) != 0;
    rtvalue["can_ban_users"] = (requested_permission_bits & kPermBanUsersLocal) != 0;
    rtvalue["can_manage_topics"] = (requested_permission_bits & kPermManageTopicsLocal) != 0;
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_SET_GROUP_ADMIN_RSP, JsonToWireString(rtvalue)};
    };

    if (target_uid <= 0 || target_user_id.empty() ||
        !_relation_repository->SetGroupAdmin(group_id, uid, target_uid, is_admin, requested_permission_bits))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one)
        {
            recipients.push_back(one->uid);
        }
    }
    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_admin_changed";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["operator_uid"] = uid;
    notify["target_uid"] = target_uid;
    notify["target_user_id"] = target_user_id;
    notify["is_admin"] = is_admin;
    notify["permission_bits"] = static_cast<int64_t>(requested_permission_bits);
    notify["can_change_group_info"] = (requested_permission_bits & kPermChangeGroupInfoLocal) != 0;
    notify["can_delete_messages"] = (requested_permission_bits & kPermDeleteMessagesLocal) != 0;
    notify["can_invite_users"] = (requested_permission_bits & kPermInviteUsersLocal) != 0;
    notify["can_manage_admins"] = (requested_permission_bits & kPermManageAdminsLocal) != 0;
    notify["can_pin_messages"] = (requested_permission_bits & kPermPinMessagesLocal) != 0;
    notify["can_ban_users"] = (requested_permission_bits & kPermBanUsersLocal) != 0;
    notify["can_manage_topics"] = (requested_permission_bits & kPermManageTopicsLocal) != 0;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

void GroupMessageService::HandleSetGroupAdmin(const std::shared_ptr<CSession>& session,
                                              short msg_id,
                                              const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       SetGroupAdmin(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::MuteGroupMember(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const std::string target_user_id = root.get("target_user_id", "").asString();
    const int64_t group_id = root["groupid"].asInt64();
    const int mute_seconds = root.get("mute_seconds", 0).asInt();
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }
    const int64_t mute_until = (mute_seconds > 0) ? NowSecGroupLocal() + mute_seconds : 0;

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["touid"] = target_uid;
    rtvalue["target_user_id"] = target_user_id;
    rtvalue["mute_until"] = static_cast<int64_t>(mute_until);
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_MUTE_GROUP_MEMBER_RSP, JsonToWireString(rtvalue)};
    };

    if (target_uid <= 0 || target_user_id.empty() ||
        !_relation_repository->MuteGroupMember(group_id, uid, target_uid, mute_until))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one)
        {
            recipients.push_back(one->uid);
        }
    }
    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_mute_changed";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["operator_uid"] = uid;
    notify["target_uid"] = target_uid;
    notify["target_user_id"] = target_user_id;
    notify["mute_until"] = static_cast<int64_t>(mute_until);
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

void GroupMessageService::HandleMuteGroupMember(const std::shared_ptr<CSession>& session,
                                                short msg_id,
                                                const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        MuteGroupMember(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::KickGroupMember(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const std::string target_user_id = root.get("target_user_id", "").asString();
    const int64_t group_id = root["groupid"].asInt64();
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["touid"] = target_uid;
    rtvalue["target_user_id"] = target_user_id;
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_KICK_GROUP_MEMBER_RSP, JsonToWireString(rtvalue)};
    };

    if (target_uid <= 0 || target_user_id.empty() || !_relation_repository->KickGroupMember(group_id, uid, target_uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one)
        {
            recipients.push_back(one->uid);
        }
    }
    recipients.push_back(target_uid);

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_member_kicked";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["operator_uid"] = uid;
    notify["target_uid"] = target_uid;
    notify["target_user_id"] = target_user_id;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

void GroupMessageService::HandleKickGroupMember(const std::shared_ptr<CSession>& session,
                                                short msg_id,
                                                const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        KickGroupMember(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::QuitGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_QUIT_GROUP_RSP, JsonToWireString(rtvalue)};
    };

    if (!_relation_repository->QuitGroup(group_id, uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one)
        {
            recipients.push_back(one->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_member_quit";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["target_uid"] = uid;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

void GroupMessageService::HandleQuitGroup(const std::shared_ptr<CSession>& session,
                                          short msg_id,
                                          const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       QuitGroup(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::DissolveGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);

    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
        rtvalue["name"] = group_info->name;
    }

    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_DISSOLVE_GROUP_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || group_id <= 0 || !group_info || group_info->status != 1)
    {
        rtvalue["error"] = ErrorCodes::GroupNotFound;
        return result();
    }

    if (group_info->owner_uid != uid)
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one && one->status == 1)
        {
            recipients.push_back(one->uid);
        }
    }

    if (!_relation_repository->DissolveGroup(group_id, uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_dissolved";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["name"] = group_info ? group_info->name : "";
    notify["operator_uid"] = uid;
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

void GroupMessageService::HandleDissolveGroup(const std::shared_ptr<CSession>& session,
                                              short msg_id,
                                              const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       DissolveGroup(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}
