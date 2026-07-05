#include "GroupMessageWorkflow.hpp"

#include "ChatRuntime.hpp"
#include "ChatMessageCommandDtos.hpp"
#include "MessageServiceUtil.hpp"
#include "const.hpp"
#include "logging/Logger.hpp"
#include "ports/IDeliveryGateway.hpp"
#include "ports/IEventPublisher.hpp"
#include "ports/IMessageRepository.hpp"
#include "ports/IRelationRepository.hpp"

#include "json/GlazeCompat.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
using memochat::chat::message::JsonToWireString;
using memochat::chat::message::NowMs;
using memochat::chat::message::NowSec;
namespace ChatMessageCommand = memochat::chat::command;

constexpr std::size_t kMaxChatMessageContentBytes = 4096;
constexpr std::size_t kMaxGroupFileNameBytes = 255;
constexpr std::size_t kMaxGroupMimeBytes = 128;
constexpr int kMaxGroupAttachmentBytes = 100 * 1024 * 1024;

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

int AuthenticatedGroupRequestUidLocal(const MessageCommandRequest& request, int payload_uid)
{
    return request.session_uid > 0 ? request.session_uid : payload_uid;
}

bool HasControlCharacter(std::string_view value)
{
    return std::any_of(value.begin(),
                       value.end(),
                       [](unsigned char ch)
                       {
                           return ch < 0x20 || ch == 0x7F;
                       });
}

bool IsSafeGroupFileName(std::string_view value)
{
    if (value.empty() || value.size() > kMaxGroupFileNameBytes || HasControlCharacter(value))
    {
        return false;
    }
    if (value == "." || value == ".." || value.find("..") != std::string_view::npos)
    {
        return false;
    }
    return value.find('/') == std::string_view::npos && value.find('\\') == std::string_view::npos;
}

bool IsSafeMime(std::string_view value)
{
    if (value.empty() || value.size() > kMaxGroupMimeBytes || value.find('/') == std::string_view::npos ||
        HasControlCharacter(value))
    {
        return false;
    }
    return std::all_of(value.begin(),
                       value.end(),
                       [](unsigned char ch)
                       {
                           return std::isalnum(ch) || ch == '/' || ch == '.' || ch == '+' || ch == '-';
                       });
}

bool HasAttachmentMetadata(const GroupMessageInfo& info)
{
    return !info.file_name.empty() || !info.mime.empty() || info.size != 0;
}

bool IsValidAttachmentMetadata(const GroupMessageInfo& info)
{
    if (!HasAttachmentMetadata(info))
    {
        return true;
    }
    return IsSafeGroupFileName(info.file_name) && IsSafeMime(info.mime) && info.size > 0 &&
           info.size <= kMaxGroupAttachmentBytes;
}
} // namespace

GroupMessageWorkflow::GroupMessageWorkflow(IMessageRepository* message_repository,
                                           IRelationRepository* relation_repository,
                                           IDeliveryGateway* delivery_gateway,
                                           IEventPublisher* event_publisher)
    : _message_repository(message_repository)
    , _relation_repository(relation_repository)
    , _delivery_gateway(delivery_gateway)
    , _event_publisher(event_publisher)
{
}

MessageCommandResult GroupMessageWorkflow::GroupChatMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonValue root;
    ParseJsonObjectGroupLocal(request.payload_json, root);
    const int from_uid =
        AuthenticatedGroupRequestUidLocal(request, memochat::json::glaze_safe_get<int>(root, "fromuid", 0));
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
    if (info.msg_id.empty() || info.content.empty() || info.content.size() > kMaxChatMessageContentBytes ||
        !IsValidAttachmentMetadata(info))
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

MessageCommandResult GroupMessageWorkflow::EditGroupMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatMessageCommand::ChatGroupEditRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
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

MessageCommandResult GroupMessageWorkflow::RevokeGroupMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatMessageCommand::ChatGroupRevokeRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
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

MessageCommandResult GroupMessageWorkflow::ForwardGroupMessage(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatMessageCommand::ChatGroupForwardRequestFromJsonValue(root);
    const int from_uid = AuthenticatedGroupRequestUidLocal(request, command.from_uid);
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
