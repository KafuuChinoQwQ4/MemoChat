#include "GroupMessageService.h"

#include "ChatRuntime.h"
#include "CSession.h"
#include "ChatGroupCommandDtos.h"
#include "ChatHistoryCommandDtos.h"
#include "ChatMessageCommandDtos.h"
#include "ChatRelationGroupDtos.h"
#include "GroupResponseFormatter.h"
#include "MessageServiceUtil.h"
#include "logging/Logger.h"

#include <chrono>
#include <iostream>
#include "json/GlazeCompat.h"
#include <unordered_set>

namespace
{
using memochat::chat::message::JsonToWireString;
using memochat::chat::message::NowMs;
using memochat::chat::message::NowSec;
namespace GroupResponseFormatter = memochat::chat::message::GroupResponseFormatter;
namespace ChatGroupCommand = memochat::chat::group;
namespace ChatHistoryCommand = memochat::chat::history;
namespace ChatMessageCommand = memochat::chat::command;
namespace ChatOutput = memochat::chat::output;

bool ParseJsonObjectGroupLocal(const std::string& payload, memochat::json::JsonValue& root)
{
    memochat::json::JsonCharReaderBuilder builder;
    std::unique_ptr<memochat::json::JsonCharReader> reader(builder.newCharReader());
    std::string errors;
    return reader->parse(payload.data(), payload.data() + payload.size(), &root, &errors) && root.is_object();
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
        ChatOutput::ChatGroupListRowDto row;
        row.groupid = group->group_id;
        row.group_code = group->group_code;
        row.name = group->name;
        row.icon = group->icon;
        row.owner_uid = group->owner_uid;
        row.announcement = group->announcement;
        row.member_limit = group->member_limit;
        row.member_count = group->member_count;
        row.is_all_muted = group->is_all_muted;
        row.role = group->role;
        row.permission_bits = group->permission_bits;
        out["group_list"].append(ChatOutput::ToJsonValue(row));
    }

    std::vector<std::shared_ptr<GroupApplyInfo>> applies;
    _relation_repository->GetPendingGroupApplyForReviewer(uid, applies, 30);
    for (const auto& apply : applies)
    {
        if (!apply)
        {
            continue;
        }
        ChatOutput::ChatPendingGroupApplyRowDto row;
        row.apply_id = apply->apply_id;
        row.groupid = apply->group_id;
        std::shared_ptr<GroupInfo> group_info;
        if (_relation_repository->GetGroupById(apply->group_id, group_info) && group_info)
        {
            row.group_code = group_info->group_code;
        }
        row.applicant_uid = apply->applicant_uid;
        row.inviter_uid = apply->inviter_uid;
        auto applicant = _relation_repository->GetUserByUid(apply->applicant_uid);
        if (applicant)
        {
            row.applicant_user_id = applicant->user_id;
        }
        if (apply->inviter_uid > 0)
        {
            auto inviter = _relation_repository->GetUserByUid(apply->inviter_uid);
            if (inviter)
            {
                row.inviter_user_id = inviter->user_id;
            }
        }
        row.type = apply->type;
        row.status = apply->status;
        row.reason = apply->reason;
        out["pending_group_apply_list"].append(ChatOutput::ToJsonValue(row));
    }
}

MessageCommandResult GroupMessageService::CreateGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);

    const auto command = ChatGroupCommand::ChatGroupCreateRequestFromJsonValue(root);
    const int owner_uid = command.owner_uid;
    const std::string& group_name = command.name;
    const std::string& announcement = command.announcement;
    const int member_limit = command.member_limit;
    std::vector<int> members;
    std::unordered_set<int> member_set;
    bool invalid_member_user_id = false;
    for (const auto& member_user_id : command.member_user_ids)
    {
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
    for (int uid : member_set)
    {
        members.push_back(uid);
    }

    ChatGroupCommand::ChatGroupCreateResponseDto rtdto;
    rtdto.error = ErrorCodes::Success;
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_CREATE_GROUP_RSP, JsonToWireString(rtvalue)};
    };
    const auto fail = [&rtdto, &rtvalue, &result](int error_code)
    {
        rtdto.error = error_code;
        rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
        return result();
    };

    if (owner_uid <= 0 || group_name.empty() || group_name.size() > 64 || invalid_member_user_id)
    {
        return fail(ErrorCodes::Error_Json);
    }
    for (int uid : members)
    {
        if (!_relation_repository->IsPrivateFriend(owner_uid, uid))
        {
            return fail(ErrorCodes::GroupPermissionDenied);
        }
    }

    int64_t group_id = 0;
    std::string group_code;
    if (!_relation_repository
             ->CreateGroup(owner_uid, group_name, announcement, member_limit, members, group_id, group_code) ||
        group_id <= 0)
    {
        return fail(ErrorCodes::RPCFailed);
    }

    rtdto.groupid = group_id;
    rtdto.group_code = group_code;
    rtdto.name = group_name;
    rtdto.announcement = announcement;
    rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
    BuildGroupListJson(owner_uid, rtvalue);

    if (!members.empty())
    {
        const auto notify = ChatMessageCommand::ToJsonValue(
            ChatMessageCommand::ChatGroupInviteCreatedEventDto{.error = ErrorCodes::Success,
                                                               .event = "group_invite",
                                                               .groupid = group_id,
                                                               .group_code = group_code,
                                                               .name = group_name,
                                                               .operator_uid = owner_uid});
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
    const auto command = ChatGroupCommand::ChatGroupListRequestFromJsonValue(root);
    const int uid = command.uid;

    memochat::json::JsonValue rtvalue =
        ChatGroupCommand::ToJsonValue(ChatGroupCommand::ChatGroupListResponseDto{.error = ErrorCodes::Success});
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
    const auto command = ChatGroupCommand::ChatGroupInviteMemberRequestFromJsonValue(root);
    const int from_uid = command.from_uid;
    const std::string& target_user_id = command.target_user_id;
    const int64_t group_id = command.group_id;
    const std::string& reason = command.reason;
    int to_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, to_uid))
    {
        to_uid = 0;
    }

    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(
        ChatGroupCommand::ChatGroupInviteMemberResponseDto{.error = ErrorCodes::Success,
                                                           .groupid = group_id,
                                                           .touid = to_uid,
                                                           .target_user_id = target_user_id});
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

    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupInviteMemberEventDto{.error = ErrorCodes::Success,
                                                          .event = "group_invite",
                                                          .groupid = group_id,
                                                          .group_code = group_info ? group_info->group_code : "",
                                                          .name = group_info ? group_info->name : "",
                                                          .operator_uid = from_uid,
                                                          .target_user_id = target_user_id,
                                                          .reason = reason});
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
    const auto command = ChatGroupCommand::ChatGroupApplyJoinRequestFromJsonValue(root);
    const int from_uid = command.from_uid;
    const std::string& group_code = command.group_code;
    int64_t group_id = 0;
    if (!_relation_repository->GetGroupIdByCode(group_code, group_id))
    {
        group_id = 0;
    }
    const std::string& reason = command.reason;

    memochat::json::JsonValue rtvalue =
        ChatGroupCommand::ToJsonValue(ChatGroupCommand::ChatGroupApplyJoinResponseDto{.error = ErrorCodes::Success,
                                                                                      .groupid = group_id,
                                                                                      .group_code = group_code});
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
    auto applicant = _relation_repository->GetUserByUid(from_uid);
    std::optional<std::string> applicant_user_id;
    if (applicant)
    {
        applicant_user_id = applicant->user_id;
    }
    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupApplyEventDto{.error = ErrorCodes::Success,
                                                   .event = "group_apply",
                                                   .groupid = group_id,
                                                   .group_code = group_code,
                                                   .applicant_uid = from_uid,
                                                   .applicant_user_id = applicant_user_id,
                                                   .reason = reason});
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
    const auto command = ChatGroupCommand::ChatGroupReviewApplyRequestFromJsonValue(root);
    const int reviewer_uid = command.reviewer_uid;
    const int64_t apply_id = command.apply_id;
    const bool agree = command.agree;

    ChatGroupCommand::ChatGroupReviewApplyResponseDto rtdto;
    rtdto.error = ErrorCodes::Success;
    rtdto.apply_id = apply_id;
    rtdto.agree = agree;
    const auto result = [&rtdto]()
    {
        return MessageCommandResult{ID_REVIEW_GROUP_APPLY_RSP, JsonToWireString(ChatGroupCommand::ToJsonValue(rtdto))};
    };

    if (reviewer_uid <= 0 || apply_id <= 0)
    {
        rtdto.error = ErrorCodes::Error_Json;
        return result();
    }

    std::shared_ptr<GroupApplyInfo> apply_info;
    if (!_relation_repository->ReviewGroupApply(apply_id, reviewer_uid, agree, apply_info) || !apply_info)
    {
        rtdto.error = ErrorCodes::GroupApplyNotFound;
        return result();
    }

    rtdto.groupid = static_cast<int64_t>(apply_info->group_id);
    rtdto.applicant_uid = apply_info->applicant_uid;
    std::shared_ptr<GroupInfo> apply_group;
    if (_relation_repository->GetGroupById(apply_info->group_id, apply_group) && apply_group)
    {
        rtdto.group_code = apply_group->group_code;
    }
    auto applicant = _relation_repository->GetUserByUid(apply_info->applicant_uid);
    if (applicant)
    {
        rtdto.applicant_user_id = applicant->user_id;
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

    std::optional<std::string> applicant_user_id;
    if (applicant)
    {
        applicant_user_id = applicant->user_id;
    }
    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupApplyReviewedEventDto{.error = ErrorCodes::Success,
                                                           .event = "group_member_changed",
                                                           .groupid = apply_info->group_id,
                                                           .group_code = apply_group ? apply_group->group_code : "",
                                                           .applicant_uid = apply_info->applicant_uid,
                                                           .applicant_user_id = applicant_user_id,
                                                           .agree = agree,
                                                           .operator_uid = reviewer_uid});
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

    ChatMessageCommand::ChatGroupSendResponseDto rtdto;
    rtdto.error = ErrorCodes::Success;
    rtdto.fromuid = from_uid;
    rtdto.groupid = group_id;
    if (!client_msg_id.empty())
    {
        rtdto.client_msg_id = client_msg_id;
    }
    const auto result = [&rtdto]()
    {
        return MessageCommandResult{ID_GROUP_CHAT_MSG_RSP, JsonToWireString(ChatMessageCommand::ToJsonValue(rtdto))};
    };

    if (from_uid <= 0 || group_id <= 0 || !msg.isObject())
    {
        rtdto.error = ErrorCodes::Error_Json;
        return result();
    }

    int role = 0;
    if (!_relation_repository->GetUserRoleInGroup(group_id, from_uid, role))
    {
        rtdto.error = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    const auto now_sec = NowSec();
    const auto now_ms = NowMs();
    for (const auto& member : members)
    {
        if (member && member->uid == from_uid && member->mute_until > now_sec)
        {
            rtdto.error = ErrorCodes::GroupMuted;
            return result();
        }
    }

    const bool mention_all = memochat::json::glaze_safe_get<bool>(msg, "mention_all", false);
    if (mention_all && role < 2)
    {
        rtdto.error = ErrorCodes::GroupPermissionDenied;
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
        rtdto.error = ErrorCodes::Error_Json;
        return result();
    }

    const auto accept_ts = NowMs();
    rtdto.accept_node = memochat::chatruntime::SelfServerName();
    rtdto.accept_ts = static_cast<int64_t>(accept_ts);
    rtdto.status = kafka_primary ? "accepted" : "persisted";

    auto sender_info = _relation_repository->GetUserByUid(from_uid);
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (sender_info)
    {
        rtdto.from_name = sender_info->name;
        rtdto.from_nick = sender_info->nick;
        rtdto.from_icon = sender_info->icon;
        rtdto.from_user_id = sender_info->user_id;
    }
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }

    auto event_payload = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupSendEventDto{.fromuid = from_uid,
                                                  .groupid = static_cast<int64_t>(group_id),
                                                  .trace_id = root.get("trace_id", "").asString(),
                                                  .request_id = root.get("request_id", "").asString(),
                                                  .span_id = root.get("span_id", "").asString(),
                                                  .event_id = client_msg_id,
                                                  .accept_node = memochat::chatruntime::SelfServerName(),
                                                  .accept_ts = static_cast<int64_t>(accept_ts)});
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
                rtdto.error = ErrorCodes::RPCFailed;
                rtdto.status = "failed";
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
        rtdto.error = ErrorCodes::RPCFailed;
        rtdto.status = "failed";
        return result();
    }
    info.server_msg_id = server_msg_id;
    info.group_seq = group_seq;

    memochat::json::JsonValue msg_out = msg;
    msg_out["created_at"] = static_cast<int64_t>(now_ms);
    msg_out["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    msg_out["group_seq"] = static_cast<int64_t>(group_seq);
    rtdto.msg = msg_out;
    rtdto.created_at = static_cast<int64_t>(now_ms);
    rtdto.server_msg_id = static_cast<int64_t>(server_msg_id);
    rtdto.group_seq = static_cast<int64_t>(group_seq);

    const memochat::json::JsonValue rtvalue = ChatMessageCommand::ToJsonValue(rtdto);
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
    const auto command = ChatHistoryCommand::ChatGroupHistoryRequestFromJsonValue(root);
    const int uid = command.uid;
    const int64_t group_id = command.group_id;
    const int64_t before_ts = command.before_ts;
    const int64_t before_seq = command.before_seq;
    const int limit = command.limit;

    memochat::json::JsonValue rtvalue =
        ChatGroupCommand::ToJsonValue(ChatGroupCommand::ChatGroupHistoryResponseDto{.error = ErrorCodes::Success,
                                                                                    .groupid = group_id,
                                                                                    .has_more = false,
                                                                                    .next_before_seq = 0});
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
        memochat::json::JsonValue item = GroupResponseFormatter::SerializeHistoryMessage(*one);
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
        read_ts = NowMs();
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
    const auto command = ChatMessageCommand::ChatGroupEditRequestFromJsonValue(root);
    const int uid = command.uid;
    const int64_t group_id = command.group_id;
    const std::string& target_msg_id = command.msgid;
    const std::string& content = command.content;
    const int64_t now_ms = NowMs();

    memochat::json::JsonValue rtvalue = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMessageChangedResultDto{.error = ErrorCodes::Success,
                                                             .groupid = group_id,
                                                             .msgid = target_msg_id,
                                                             .content = content,
                                                             .changed_at_ms = now_ms});
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

    const memochat::json::JsonValue notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMessageChangedEventDto{.error = ErrorCodes::Success,
                                                            .event = "group_msg_edited",
                                                            .groupid = group_id,
                                                            .msgid = target_msg_id,
                                                            .content = content,
                                                            .changed_at_ms = now_ms,
                                                            .operator_uid = uid});
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
    const auto command = ChatMessageCommand::ChatGroupRevokeRequestFromJsonValue(root);
    const int uid = command.uid;
    const int64_t group_id = command.group_id;
    const std::string& target_msg_id = command.msgid;
    const int64_t now_ms = NowMs();

    memochat::json::JsonValue rtvalue = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMessageChangedResultDto{.error = ErrorCodes::Success,
                                                             .groupid = group_id,
                                                             .msgid = target_msg_id,
                                                             .content = "[消息已撤回]",
                                                             .changed_at_ms = now_ms,
                                                             .deleted = true});
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

    const memochat::json::JsonValue notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMessageChangedEventDto{.error = ErrorCodes::Success,
                                                            .event = "group_msg_revoked",
                                                            .groupid = group_id,
                                                            .msgid = target_msg_id,
                                                            .content = "[消息已撤回]",
                                                            .changed_at_ms = now_ms,
                                                            .operator_uid = uid,
                                                            .deleted = true});
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
    const auto command = ChatMessageCommand::ChatGroupForwardRequestFromJsonValue(root);
    const int from_uid = command.from_uid;
    const int64_t group_id = command.group_id;
    const std::string& source_msg_id = command.source_msg_id;
    std::string client_msg_id = command.client_msg_id;

    memochat::json::JsonValue rtvalue =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatGroupForwardResultDto{.error = ErrorCodes::Success,
                                                                                      .fromuid = from_uid,
                                                                                      .groupid = group_id,
                                                                                      .client_msg_id = client_msg_id});
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

    const int64_t now_ms = NowMs();
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
    memochat::json::JsonValue forwarded_forward_meta;
    {
        memochat::json::JsonReader forward_reader;
        memochat::json::JsonValue parsed_forward_meta;
        if (forward_reader.parse(info.forward_meta_json, parsed_forward_meta))
        {
            forwarded_forward_meta = parsed_forward_meta;
        }
    }
    const memochat::json::JsonValue forwarded_msg = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupForwardMessageDto{.msgid = info.msg_id,
                                                       .msgtype = info.msg_type,
                                                       .content = info.content,
                                                       .mentions = mentions,
                                                       .file_name = info.file_name,
                                                       .mime = info.mime,
                                                       .size = info.size,
                                                       .forwarded_from_msgid = source_msg_id,
                                                       .created_at = now_ms,
                                                       .server_msg_id = server_msg_id,
                                                       .group_seq = group_seq,
                                                       .reply_to_server_msg_id = info.reply_to_server_msg_id,
                                                       .forward_meta = forwarded_forward_meta});

    ChatMessageCommand::ChatGroupForwardResultDto result_dto{.error = ErrorCodes::Success,
                                                             .fromuid = from_uid,
                                                             .groupid = group_id,
                                                             .client_msg_id = client_msg_id,
                                                             .created_at = now_ms,
                                                             .server_msg_id = server_msg_id,
                                                             .group_seq = group_seq,
                                                             .reply_to_server_msg_id = info.reply_to_server_msg_id,
                                                             .forward_meta = forward_meta,
                                                             .msg = forwarded_msg};

    if (sender_info)
    {
        result_dto.from_name = sender_info->name;
        result_dto.from_nick = sender_info->nick;
        result_dto.from_icon = sender_info->icon;
        result_dto.from_user_id = sender_info->user_id;
    }
    std::shared_ptr<GroupInfo> group_info;
    if (_relation_repository->GetGroupById(group_id, group_info) && group_info)
    {
        result_dto.group_code = group_info->group_code;
    }
    rtvalue = ChatMessageCommand::ToJsonValue(result_dto);

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
    const auto command = ChatGroupCommand::ChatGroupReadAckRequestFromJsonValue(root);
    const int uid = command.uid;
    const int64_t group_id = command.group_id;
    int64_t read_ts = command.read_ts;
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
        read_ts = NowMs();
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
    const auto notify =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatGroupReadAckEventDto{.error = ErrorCodes::Success,
                                                                                     .event = "group_read_ack",
                                                                                     .groupid = group_id,
                                                                                     .fromuid = uid,
                                                                                     .read_ts = read_ts});
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
    const auto command = ChatGroupCommand::ChatGroupAnnouncementUpdateRequestFromJsonValue(root);
    const int uid = command.uid;
    const int64_t group_id = command.group_id;
    const std::string& announcement = command.announcement;

    ChatGroupCommand::ChatGroupAnnouncementUpdateResponseDto rtdto{.error = ErrorCodes::Success,
                                                                   .groupid = group_id,
                                                                   .announcement = announcement};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
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

    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupAnnouncementUpdatedEventDto{.error = ErrorCodes::Success,
                                                                 .event = "group_announcement_updated",
                                                                 .groupid = group_id,
                                                                 .group_code = group_info ? group_info->group_code : "",
                                                                 .announcement = announcement,
                                                                 .operator_uid = uid});
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
    const auto command = ChatGroupCommand::ChatGroupIconUpdateRequestFromJsonValue(root);
    const int uid = command.uid;
    const int64_t group_id = command.group_id;
    const std::string& icon = command.icon;

    ChatGroupCommand::ChatGroupIconUpdateResponseDto rtdto{.error = ErrorCodes::Success,
                                                           .groupid = group_id,
                                                           .icon = icon};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
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

    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupIconUpdatedEventDto{.error = ErrorCodes::Success,
                                                         .event = "group_icon_updated",
                                                         .groupid = group_id,
                                                         .group_code = group_info ? group_info->group_code : "",
                                                         .icon = icon,
                                                         .operator_uid = uid});
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
    const auto command = ChatGroupCommand::ChatGroupSetAdminRequestFromJsonValue(root);
    const int uid = command.uid;
    const std::string& target_user_id = command.target_user_id;
    const int64_t group_id = command.group_id;
    const bool is_admin = command.is_admin;
    const int64_t requested_permission_bits = command.requested_permission_bits;
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }

    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(
        ChatGroupCommand::ChatGroupSetAdminResponseDto{.error = ErrorCodes::Success,
                                                       .groupid = group_id,
                                                       .touid = target_uid,
                                                       .target_user_id = target_user_id,
                                                       .is_admin = is_admin,
                                                       .permission_bits = requested_permission_bits});
    GroupResponseFormatter::ApplyPermissionFlags(rtvalue, requested_permission_bits);
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
    auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupAdminChangedEventDto{.error = ErrorCodes::Success,
                                                          .event = "group_admin_changed",
                                                          .groupid = group_id,
                                                          .group_code = group_info ? group_info->group_code : "",
                                                          .operator_uid = uid,
                                                          .target_uid = target_uid,
                                                          .target_user_id = target_user_id,
                                                          .is_admin = is_admin,
                                                          .permission_bits = requested_permission_bits});
    GroupResponseFormatter::ApplyPermissionFlags(notify, requested_permission_bits);
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
    const auto command = ChatGroupCommand::ChatGroupMemberActionRequestFromJsonValue(root);
    const int uid = command.uid;
    const std::string& target_user_id = command.target_user_id;
    const int64_t group_id = command.group_id;
    const int mute_seconds = command.mute_seconds;
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }
    const int64_t mute_until = (mute_seconds > 0) ? NowSec() + mute_seconds : 0;

    ChatGroupCommand::ChatGroupMuteMemberResponseDto rtdto{.error = ErrorCodes::Success,
                                                           .groupid = group_id,
                                                           .touid = target_uid,
                                                           .target_user_id = target_user_id,
                                                           .mute_until = mute_until};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
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
    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMuteChangedEventDto{.error = ErrorCodes::Success,
                                                         .event = "group_mute_changed",
                                                         .groupid = group_id,
                                                         .group_code = group_info ? group_info->group_code : "",
                                                         .operator_uid = uid,
                                                         .target_uid = target_uid,
                                                         .target_user_id = target_user_id,
                                                         .mute_until = mute_until});
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
    const auto command = ChatGroupCommand::ChatGroupMemberActionRequestFromJsonValue(root);
    const int uid = command.uid;
    const std::string& target_user_id = command.target_user_id;
    const int64_t group_id = command.group_id;
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }

    ChatGroupCommand::ChatGroupKickMemberResponseDto rtdto{.error = ErrorCodes::Success,
                                                           .groupid = group_id,
                                                           .touid = target_uid,
                                                           .target_user_id = target_user_id};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
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

    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMemberKickedEventDto{.error = ErrorCodes::Success,
                                                          .event = "group_member_kicked",
                                                          .groupid = group_id,
                                                          .group_code = group_info ? group_info->group_code : "",
                                                          .operator_uid = uid,
                                                          .target_uid = target_uid,
                                                          .target_user_id = target_user_id});
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
    const auto command = ChatGroupCommand::ChatGroupIdRequestFromJsonValue(root);
    const int uid = command.uid;
    const int64_t group_id = command.group_id;

    ChatGroupCommand::ChatGroupQuitResponseDto rtdto{.error = ErrorCodes::Success, .groupid = group_id};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
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

    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMemberQuitEventDto{.error = ErrorCodes::Success,
                                                        .event = "group_member_quit",
                                                        .groupid = group_id,
                                                        .group_code = group_info ? group_info->group_code : "",
                                                        .target_uid = uid});
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
    const auto command = ChatGroupCommand::ChatGroupIdRequestFromJsonValue(root);
    const int uid = command.uid;
    const int64_t group_id = command.group_id;

    ChatGroupCommand::ChatGroupDissolveResponseDto rtdto{.error = ErrorCodes::Success, .groupid = group_id};

    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
        rtdto.name = group_info->name;
    }

    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
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

    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupDissolvedEventDto{.error = ErrorCodes::Success,
                                                       .event = "group_dissolved",
                                                       .groupid = group_id,
                                                       .group_code = group_info ? group_info->group_code : "",
                                                       .name = group_info ? group_info->name : "",
                                                       .operator_uid = uid});
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
