#pragma once

#include "json/GlazeCompat.h"

#include <cstdint>
#include <optional>
#include <string>

namespace memochat::chat::command
{

struct ChatPrivateEditRequestDto
{
    int uid = 0;
    int peer_uid = 0;
    std::string msgid;
    std::string content;
};

struct ChatPrivateForwardRequestDto
{
    int from_uid = 0;
    int peer_uid = 0;
    std::string source_msg_id;
    std::string client_msg_id;
};

struct ChatPrivateRevokeRequestDto
{
    int uid = 0;
    int peer_uid = 0;
    std::string msgid;
};

struct ChatPrivateMessageChangedResultDto
{
    int error = 0;
    int fromuid = 0;
    int peer_uid = 0;
    std::string msgid;
    std::string content;
    int64_t changed_at_ms = 0;
    bool deleted = false;
};

struct ChatPrivateMessageChangedEventDto
{
    int error = 0;
    std::string event;
    int fromuid = 0;
    int peer_uid = 0;
    std::string msgid;
    std::string content;
    int64_t changed_at_ms = 0;
    bool deleted = false;
};

struct ChatPrivateReadAckEventDto
{
    int error = 0;
    std::string event;
    int fromuid = 0;
    int peer_uid = 0;
    int64_t read_ts = 0;
};

struct ChatPrivateSendEventDto
{
    int fromuid = 0;
    int touid = 0;
    std::string trace_id;
    std::string request_id;
    std::string span_id;
    std::string event_id;
    std::string accept_node;
    int64_t accept_ts = 0;
};

// Private send-path response root (TextChatMessage rtvalue). Built once across
// the branches by typed field assignment; the optional tail fields are emitted
// only when engaged, in the exact current wire order: error, fromuid, touid,
// client_msg_id, accept_node, accept_ts, status, from_user_id, text_array.
struct ChatPrivateSendResponseDto
{
    int error = 0;
    int fromuid = 0;
    int touid = 0;
    std::optional<std::string> client_msg_id;
    std::optional<std::string> accept_node;
    std::optional<int64_t> accept_ts;
    std::optional<std::string> status;
    std::optional<std::string> from_user_id;
    std::optional<memochat::json::JsonValue> text_array;
};

struct ChatGroupSendEventDto
{
    int fromuid = 0;
    int64_t groupid = 0;
    std::string trace_id;
    std::string request_id;
    std::string span_id;
    std::string event_id;
    std::string accept_node;
    int64_t accept_ts = 0;
};

// Group send-path response root (GroupChatMessage rtvalue). Built once across the
// branches by typed field assignment; optional fields are emitted only when
// engaged, in the exact current wire order: error, fromuid, groupid,
// client_msg_id, accept_node, accept_ts, status, from_name, from_nick, from_icon,
// from_user_id, group_code, msg, created_at, server_msg_id, group_seq.
struct ChatGroupSendResponseDto
{
    int error = 0;
    int fromuid = 0;
    int64_t groupid = 0;
    std::optional<std::string> client_msg_id;
    std::optional<std::string> accept_node;
    std::optional<int64_t> accept_ts;
    std::optional<std::string> status;
    std::optional<std::string> from_name;
    std::optional<std::string> from_nick;
    std::optional<std::string> from_icon;
    std::optional<std::string> from_user_id;
    std::optional<std::string> group_code;
    std::optional<memochat::json::JsonValue> msg;
    std::optional<int64_t> created_at;
    std::optional<int64_t> server_msg_id;
    std::optional<int64_t> group_seq;
};

struct ChatPrivateForwardMessageDto
{
    std::string msgid;
    std::string content;
    int64_t created_at = 0;
    int64_t reply_to_server_msg_id = 0;
    memochat::json::JsonValue forward_meta;
    std::optional<std::string> from_user_id;
};

struct ChatPrivateForwardResultDto
{
    int error = 0;
    int fromuid = 0;
    int peer_uid = 0;
    int touid = 0;
    std::string client_msg_id;
    int64_t created_at = 0;
    std::optional<std::string> from_user_id;
    memochat::json::JsonValue msg;
};

struct ChatGroupEditRequestDto
{
    int uid = 0;
    int64_t group_id = 0;
    std::string msgid;
    std::string content;
};

struct ChatGroupForwardRequestDto
{
    int from_uid = 0;
    int64_t group_id = 0;
    std::string source_msg_id;
    std::string client_msg_id;
};

struct ChatGroupRevokeRequestDto
{
    int uid = 0;
    int64_t group_id = 0;
    std::string msgid;
};

struct ChatGroupMessageChangedResultDto
{
    int error = 0;
    int64_t groupid = 0;
    std::string msgid;
    std::string content;
    int64_t changed_at_ms = 0;
    bool deleted = false;
};

struct ChatGroupMessageChangedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string msgid;
    std::string content;
    int64_t changed_at_ms = 0;
    int operator_uid = 0;
    bool deleted = false;
};

struct ChatGroupReadAckEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    int fromuid = 0;
    int64_t read_ts = 0;
};

struct ChatGroupInviteCreatedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    std::string name;
    int operator_uid = 0;
};

struct ChatGroupInviteMemberEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    std::string name;
    int operator_uid = 0;
    std::string target_user_id;
    std::string reason;
};

struct ChatGroupApplyEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    int applicant_uid = 0;
    std::optional<std::string> applicant_user_id;
    std::string reason;
};

struct ChatGroupApplyReviewedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    int applicant_uid = 0;
    std::optional<std::string> applicant_user_id;
    bool agree = false;
    int operator_uid = 0;
};

struct ChatGroupAnnouncementUpdatedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    std::string announcement;
    int operator_uid = 0;
};

struct ChatGroupIconUpdatedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    std::string icon;
    int operator_uid = 0;
};

struct ChatGroupAdminChangedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    int operator_uid = 0;
    int target_uid = 0;
    std::string target_user_id;
    bool is_admin = false;
    int64_t permission_bits = 0;
};

struct ChatGroupMuteChangedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    int operator_uid = 0;
    int target_uid = 0;
    std::string target_user_id;
    int64_t mute_until = 0;
};

struct ChatGroupMemberKickedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    int operator_uid = 0;
    int target_uid = 0;
    std::string target_user_id;
};

struct ChatGroupMemberQuitEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    int target_uid = 0;
};

struct ChatGroupDissolvedEventDto
{
    int error = 0;
    std::string event;
    int64_t groupid = 0;
    std::string group_code;
    std::string name;
    int operator_uid = 0;
};

struct ChatGroupForwardMessageDto
{
    std::string msgid;
    std::string msgtype;
    std::string content;
    memochat::json::JsonValue mentions;
    std::string file_name;
    std::string mime;
    int64_t size = 0;
    std::string forwarded_from_msgid;
    int64_t created_at = 0;
    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    int64_t reply_to_server_msg_id = 0;
    memochat::json::JsonValue forward_meta;
};

struct ChatGroupForwardResultDto
{
    int error = 0;
    int fromuid = 0;
    int64_t groupid = 0;
    std::string client_msg_id;
    int64_t created_at = 0;
    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    int64_t reply_to_server_msg_id = 0;
    memochat::json::JsonValue forward_meta;
    memochat::json::JsonValue msg;
    std::string from_name;
    std::string from_nick;
    std::string from_icon;
    std::string from_user_id;
    std::string group_code;
};

inline ChatPrivateEditRequestDto ChatPrivateEditRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatPrivateEditRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.peer_uid = root["peer_uid"].asInt();
    request.msgid = root.get("msgid", "").asString();
    request.content = root.get("content", "").asString();
    return request;
}

inline ChatPrivateForwardRequestDto ChatPrivateForwardRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatPrivateForwardRequestDto request;
    request.from_uid = root["fromuid"].asInt();
    request.peer_uid = root["peer_uid"].asInt();
    request.source_msg_id = root.get("msgid", "").asString();
    request.client_msg_id = root.get("client_msg_id", "").asString();
    return request;
}

inline ChatPrivateRevokeRequestDto ChatPrivateRevokeRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatPrivateRevokeRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.peer_uid = root["peer_uid"].asInt();
    request.msgid = root.get("msgid", "").asString();
    return request;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateSendEventDto& dto)
{
    memochat::json::JsonValue value;
    value["fromuid"] = dto.fromuid;
    value["touid"] = dto.touid;
    value["trace_id"] = dto.trace_id;
    value["request_id"] = dto.request_id;
    value["span_id"] = dto.span_id;
    value["event_id"] = dto.event_id;
    value["accept_node"] = dto.accept_node;
    value["accept_ts"] = static_cast<int64_t>(dto.accept_ts);
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupSendEventDto& dto)
{
    memochat::json::JsonValue value;
    value["fromuid"] = dto.fromuid;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["trace_id"] = dto.trace_id;
    value["request_id"] = dto.request_id;
    value["span_id"] = dto.span_id;
    value["event_id"] = dto.event_id;
    value["accept_node"] = dto.accept_node;
    value["accept_ts"] = static_cast<int64_t>(dto.accept_ts);
    return value;
}

// Writes the private send response in the exact current wire order; optional
// tail fields are emitted only when engaged.
inline memochat::json::JsonValue ToJsonValue(const ChatPrivateSendResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["fromuid"] = dto.fromuid;
    value["touid"] = dto.touid;
    if (dto.client_msg_id.has_value())
    {
        value["client_msg_id"] = *dto.client_msg_id;
    }
    if (dto.accept_node.has_value())
    {
        value["accept_node"] = *dto.accept_node;
    }
    if (dto.accept_ts.has_value())
    {
        value["accept_ts"] = static_cast<int64_t>(*dto.accept_ts);
    }
    if (dto.status.has_value())
    {
        value["status"] = *dto.status;
    }
    if (dto.from_user_id.has_value())
    {
        value["from_user_id"] = *dto.from_user_id;
    }
    if (dto.text_array.has_value())
    {
        value["text_array"] = *dto.text_array;
    }
    return value;
}

// Writes the group send response in the exact current wire order; optional
// fields are emitted only when engaged.
inline memochat::json::JsonValue ToJsonValue(const ChatGroupSendResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["fromuid"] = dto.fromuid;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    if (dto.client_msg_id.has_value())
    {
        value["client_msg_id"] = *dto.client_msg_id;
    }
    if (dto.accept_node.has_value())
    {
        value["accept_node"] = *dto.accept_node;
    }
    if (dto.accept_ts.has_value())
    {
        value["accept_ts"] = static_cast<int64_t>(*dto.accept_ts);
    }
    if (dto.status.has_value())
    {
        value["status"] = *dto.status;
    }
    if (dto.from_name.has_value())
    {
        value["from_name"] = *dto.from_name;
    }
    if (dto.from_nick.has_value())
    {
        value["from_nick"] = *dto.from_nick;
    }
    if (dto.from_icon.has_value())
    {
        value["from_icon"] = *dto.from_icon;
    }
    if (dto.from_user_id.has_value())
    {
        value["from_user_id"] = *dto.from_user_id;
    }
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    if (dto.msg.has_value())
    {
        value["msg"] = *dto.msg;
    }
    if (dto.created_at.has_value())
    {
        value["created_at"] = static_cast<int64_t>(*dto.created_at);
    }
    if (dto.server_msg_id.has_value())
    {
        value["server_msg_id"] = static_cast<int64_t>(*dto.server_msg_id);
    }
    if (dto.group_seq.has_value())
    {
        value["group_seq"] = static_cast<int64_t>(*dto.group_seq);
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateMessageChangedResultDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["fromuid"] = dto.fromuid;
    value["peer_uid"] = dto.peer_uid;
    value["msgid"] = dto.msgid;
    value["content"] = dto.content;
    if (dto.deleted)
    {
        value["deleted_at_ms"] = static_cast<int64_t>(dto.changed_at_ms);
    }
    else
    {
        value["edited_at_ms"] = static_cast<int64_t>(dto.changed_at_ms);
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateMessageChangedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["fromuid"] = dto.fromuid;
    value["peer_uid"] = dto.peer_uid;
    value["msgid"] = dto.msgid;
    value["content"] = dto.content;
    if (dto.deleted)
    {
        value["deleted_at_ms"] = static_cast<int64_t>(dto.changed_at_ms);
    }
    else
    {
        value["edited_at_ms"] = static_cast<int64_t>(dto.changed_at_ms);
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateReadAckEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["fromuid"] = dto.fromuid;
    value["peer_uid"] = dto.peer_uid;
    value["read_ts"] = static_cast<int64_t>(dto.read_ts);
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateForwardMessageDto& dto)
{
    memochat::json::JsonValue value;
    value["msgid"] = dto.msgid;
    value["content"] = dto.content;
    value["created_at"] = static_cast<int64_t>(dto.created_at);
    if (dto.reply_to_server_msg_id > 0)
    {
        value["reply_to_server_msg_id"] = static_cast<int64_t>(dto.reply_to_server_msg_id);
    }
    value["forward_meta"] = dto.forward_meta;
    if (dto.from_user_id.has_value())
    {
        value["from_user_id"] = *dto.from_user_id;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateForwardResultDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["fromuid"] = dto.fromuid;
    value["peer_uid"] = dto.peer_uid;
    value["touid"] = dto.touid;
    if (!dto.client_msg_id.empty())
    {
        value["client_msg_id"] = dto.client_msg_id;
    }
    if (dto.created_at > 0)
    {
        value["created_at"] = static_cast<int64_t>(dto.created_at);
    }
    if (dto.from_user_id.has_value())
    {
        value["from_user_id"] = *dto.from_user_id;
    }
    if (!dto.msg.isNull())
    {
        value["msg"] = dto.msg;
        memochat::json::JsonValue text_array(memochat::json::array_t{});
        text_array.append(dto.msg);
        value["text_array"] = text_array;
    }
    return value;
}

inline ChatGroupEditRequestDto ChatGroupEditRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupEditRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.group_id = root["groupid"].asInt64();
    request.msgid = root.get("msgid", "").asString();
    request.content = root.get("content", "").asString();
    return request;
}

inline ChatGroupForwardRequestDto ChatGroupForwardRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupForwardRequestDto request;
    request.from_uid = root["fromuid"].asInt();
    request.group_id = root.get("groupid", 0).asInt64();
    request.source_msg_id = root.get("msgid", "").asString();
    request.client_msg_id = root.get("client_msg_id", "").asString();
    return request;
}

inline ChatGroupRevokeRequestDto ChatGroupRevokeRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupRevokeRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.group_id = root["groupid"].asInt64();
    request.msgid = root.get("msgid", "").asString();
    return request;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupMessageChangedResultDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["msgid"] = dto.msgid;
    value["content"] = dto.content;
    if (dto.deleted)
    {
        value["deleted_at_ms"] = static_cast<int64_t>(dto.changed_at_ms);
    }
    else
    {
        value["edited_at_ms"] = static_cast<int64_t>(dto.changed_at_ms);
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupMessageChangedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["msgid"] = dto.msgid;
    value["content"] = dto.content;
    if (dto.deleted)
    {
        value["deleted_at_ms"] = static_cast<int64_t>(dto.changed_at_ms);
    }
    else
    {
        value["edited_at_ms"] = static_cast<int64_t>(dto.changed_at_ms);
    }
    value["operator_uid"] = dto.operator_uid;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupReadAckEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["fromuid"] = dto.fromuid;
    value["read_ts"] = static_cast<int64_t>(dto.read_ts);
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupInviteCreatedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["name"] = dto.name;
    value["operator_uid"] = dto.operator_uid;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupInviteMemberEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["name"] = dto.name;
    value["operator_uid"] = dto.operator_uid;
    value["target_user_id"] = dto.target_user_id;
    value["reason"] = dto.reason;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupApplyEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["applicant_uid"] = dto.applicant_uid;
    if (dto.applicant_user_id.has_value())
    {
        value["applicant_user_id"] = *dto.applicant_user_id;
    }
    value["reason"] = dto.reason;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupApplyReviewedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["applicant_uid"] = dto.applicant_uid;
    if (dto.applicant_user_id.has_value())
    {
        value["applicant_user_id"] = *dto.applicant_user_id;
    }
    value["agree"] = dto.agree;
    value["operator_uid"] = dto.operator_uid;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupAnnouncementUpdatedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["announcement"] = dto.announcement;
    value["operator_uid"] = dto.operator_uid;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupIconUpdatedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["icon"] = dto.icon;
    value["operator_uid"] = dto.operator_uid;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupAdminChangedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["operator_uid"] = dto.operator_uid;
    value["target_uid"] = dto.target_uid;
    value["target_user_id"] = dto.target_user_id;
    value["is_admin"] = dto.is_admin;
    value["permission_bits"] = static_cast<int64_t>(dto.permission_bits);
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupMuteChangedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["operator_uid"] = dto.operator_uid;
    value["target_uid"] = dto.target_uid;
    value["target_user_id"] = dto.target_user_id;
    value["mute_until"] = static_cast<int64_t>(dto.mute_until);
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupMemberKickedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["operator_uid"] = dto.operator_uid;
    value["target_uid"] = dto.target_uid;
    value["target_user_id"] = dto.target_user_id;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupMemberQuitEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["target_uid"] = dto.target_uid;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupDissolvedEventDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["event"] = dto.event;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    value["name"] = dto.name;
    value["operator_uid"] = dto.operator_uid;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupForwardMessageDto& dto)
{
    memochat::json::JsonValue value;
    value["msgid"] = dto.msgid;
    value["msgtype"] = dto.msgtype;
    value["content"] = dto.content;
    value["mentions"] = dto.mentions;
    if (!dto.file_name.empty())
    {
        value["file_name"] = dto.file_name;
    }
    if (!dto.mime.empty())
    {
        value["mime"] = dto.mime;
    }
    if (dto.size > 0)
    {
        value["size"] = static_cast<int64_t>(dto.size);
    }
    value["forwarded_from_msgid"] = dto.forwarded_from_msgid;
    value["created_at"] = static_cast<int64_t>(dto.created_at);
    value["server_msg_id"] = static_cast<int64_t>(dto.server_msg_id);
    value["group_seq"] = static_cast<int64_t>(dto.group_seq);
    if (dto.reply_to_server_msg_id > 0)
    {
        value["reply_to_server_msg_id"] = static_cast<int64_t>(dto.reply_to_server_msg_id);
    }
    value["forward_meta"] = dto.forward_meta;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupForwardResultDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["fromuid"] = dto.fromuid;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    if (!dto.client_msg_id.empty())
    {
        value["client_msg_id"] = dto.client_msg_id;
    }
    if (!dto.msg.isNull())
    {
        value["msg"] = dto.msg;
    }
    if (dto.created_at > 0)
    {
        value["created_at"] = static_cast<int64_t>(dto.created_at);
    }
    if (dto.server_msg_id > 0)
    {
        value["server_msg_id"] = static_cast<int64_t>(dto.server_msg_id);
    }
    if (dto.group_seq > 0)
    {
        value["group_seq"] = static_cast<int64_t>(dto.group_seq);
    }
    if (dto.reply_to_server_msg_id > 0)
    {
        value["reply_to_server_msg_id"] = static_cast<int64_t>(dto.reply_to_server_msg_id);
    }
    if (!dto.forward_meta.isNull())
    {
        value["forward_meta"] = dto.forward_meta;
    }
    if (!dto.from_name.empty())
    {
        value["from_name"] = dto.from_name;
    }
    if (!dto.from_nick.empty())
    {
        value["from_nick"] = dto.from_nick;
    }
    if (!dto.from_icon.empty())
    {
        value["from_icon"] = dto.from_icon;
    }
    if (!dto.from_user_id.empty())
    {
        value["from_user_id"] = dto.from_user_id;
    }
    if (!dto.group_code.empty())
    {
        value["group_code"] = dto.group_code;
    }
    return value;
}

} // namespace memochat::chat::command
