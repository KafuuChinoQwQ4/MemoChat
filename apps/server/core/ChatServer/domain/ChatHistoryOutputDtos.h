#pragma once

#include "data.h"
#include "json/GlazeCompat.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace memochat::chat::history::output
{

struct ChatHistoryDynamicJson
{
    bool present = false;
    memochat::json::JsonValue value;
};

struct ChatPrivateHistoryMessageDto
{
    std::string msgid;
    std::string content;
    int fromuid = 0;
    int touid = 0;
    std::string from_user_id;
    int64_t created_at = 0;
    int64_t reply_to_server_msg_id = 0;
    ChatHistoryDynamicJson forward_meta;
    int64_t edited_at_ms = 0;
    int64_t deleted_at_ms = 0;
};

struct ChatPrivateOfflinePushMessageDto
{
    std::string msgid;
    std::string content;
    std::string from_user_id;
    int64_t created_at = 0;
    int64_t reply_to_server_msg_id = 0;
    ChatHistoryDynamicJson forward_meta;
    int64_t edited_at_ms = 0;
    int64_t deleted_at_ms = 0;
};

struct ChatPrivateOfflinePushNotifyDto
{
    int error = 0;
    int fromuid = 0;
    int touid = 0;
    std::string from_user_id;
    memochat::json::JsonValue text_array;
};

struct ChatGroupHistoryMessageDto
{
    std::string msgid;
    int64_t groupid = 0;
    int fromuid = 0;
    std::string msgtype;
    std::string content;
    memochat::json::JsonValue mentions;
    std::string file_name;
    std::string mime;
    int64_t size = 0;
    int64_t created_at = 0;
    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    int64_t reply_to_server_msg_id = 0;
    ChatHistoryDynamicJson forward_meta;
    int64_t edited_at_ms = 0;
    int64_t deleted_at_ms = 0;
    std::string from_name;
    std::string from_nick;
    std::string from_icon;
};

inline ChatHistoryDynamicJson ParseHistoryJsonValue(std::string_view body)
{
    ChatHistoryDynamicJson parsed;
    if (body.empty())
    {
        return parsed;
    }

    memochat::json::JsonValue root;
    if (memochat::json::glaze_parse(root, body))
    {
        parsed.present = true;
        parsed.value = root;
    }
    return parsed;
}

inline memochat::json::JsonValue ParseHistoryJsonArrayOrEmpty(std::string_view body)
{
    memochat::json::JsonValue fallback(memochat::json::array_t{});
    if (body.empty())
    {
        return fallback;
    }

    memochat::json::JsonValue parsed;
    if (memochat::json::glaze_parse(parsed, body) && memochat::json::glaze_is_array(parsed))
    {
        return parsed;
    }
    return fallback;
}

inline ChatPrivateHistoryMessageDto ChatPrivateHistoryMessageFromInfo(const PrivateMessageInfo& info)
{
    ChatPrivateHistoryMessageDto dto;
    dto.msgid = info.msg_id;
    dto.content = info.content;
    dto.fromuid = info.from_uid;
    dto.touid = info.to_uid;
    dto.from_user_id = info.from_user_id;
    dto.created_at = info.created_at;
    dto.reply_to_server_msg_id = info.reply_to_server_msg_id;
    dto.forward_meta = ParseHistoryJsonValue(info.forward_meta_json);
    dto.edited_at_ms = info.edited_at_ms;
    dto.deleted_at_ms = info.deleted_at_ms;
    return dto;
}

inline ChatPrivateOfflinePushMessageDto ChatPrivateOfflinePushMessageFromInfo(const PrivateMessageInfo& info)
{
    ChatPrivateOfflinePushMessageDto dto;
    dto.msgid = info.msg_id;
    dto.content = info.content;
    dto.from_user_id = info.from_user_id;
    dto.created_at = info.created_at;
    dto.reply_to_server_msg_id = info.reply_to_server_msg_id;
    dto.forward_meta = ParseHistoryJsonValue(info.forward_meta_json);
    dto.edited_at_ms = info.edited_at_ms;
    dto.deleted_at_ms = info.deleted_at_ms;
    return dto;
}

inline ChatGroupHistoryMessageDto ChatGroupHistoryMessageFromInfo(const GroupMessageInfo& info)
{
    ChatGroupHistoryMessageDto dto;
    dto.msgid = info.msg_id;
    dto.groupid = info.group_id;
    dto.fromuid = info.from_uid;
    dto.msgtype = info.msg_type;
    dto.content = info.content;
    dto.mentions = ParseHistoryJsonArrayOrEmpty(info.mentions_json);
    dto.file_name = info.file_name;
    dto.mime = info.mime;
    dto.size = info.size;
    dto.created_at = info.created_at;
    dto.server_msg_id = info.server_msg_id;
    dto.group_seq = info.group_seq;
    dto.reply_to_server_msg_id = info.reply_to_server_msg_id;
    dto.forward_meta = ParseHistoryJsonValue(info.forward_meta_json);
    dto.edited_at_ms = info.edited_at_ms;
    dto.deleted_at_ms = info.deleted_at_ms;
    dto.from_name = info.from_name;
    dto.from_nick = info.from_nick;
    dto.from_icon = info.from_icon;
    return dto;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateHistoryMessageDto& dto)
{
    memochat::json::JsonValue item;
    item["msgid"] = dto.msgid;
    item["content"] = dto.content;
    item["fromuid"] = dto.fromuid;
    item["touid"] = dto.touid;
    if (!dto.from_user_id.empty())
    {
        item["from_user_id"] = dto.from_user_id;
    }
    item["created_at"] = static_cast<int64_t>(dto.created_at);
    if (dto.reply_to_server_msg_id > 0)
    {
        item["reply_to_server_msg_id"] = static_cast<int64_t>(dto.reply_to_server_msg_id);
    }
    if (dto.forward_meta.present)
    {
        item["forward_meta"] = dto.forward_meta.value;
    }
    if (dto.edited_at_ms > 0)
    {
        item["edited_at_ms"] = static_cast<int64_t>(dto.edited_at_ms);
    }
    if (dto.deleted_at_ms > 0)
    {
        item["deleted_at_ms"] = static_cast<int64_t>(dto.deleted_at_ms);
    }
    return item;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateOfflinePushMessageDto& dto)
{
    memochat::json::JsonValue item;
    item["msgid"] = dto.msgid;
    item["content"] = dto.content;
    if (!dto.from_user_id.empty())
    {
        item["from_user_id"] = dto.from_user_id;
    }
    item["created_at"] = static_cast<int64_t>(dto.created_at);
    if (dto.reply_to_server_msg_id > 0)
    {
        item["reply_to_server_msg_id"] = static_cast<int64_t>(dto.reply_to_server_msg_id);
    }
    if (dto.forward_meta.present)
    {
        item["forward_meta"] = dto.forward_meta.value;
    }
    if (dto.edited_at_ms > 0)
    {
        item["edited_at_ms"] = static_cast<int64_t>(dto.edited_at_ms);
    }
    if (dto.deleted_at_ms > 0)
    {
        item["deleted_at_ms"] = static_cast<int64_t>(dto.deleted_at_ms);
    }
    return item;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateOfflinePushNotifyDto& dto)
{
    memochat::json::JsonValue root;
    root["error"] = dto.error;
    root["fromuid"] = dto.fromuid;
    root["touid"] = dto.touid;
    if (!dto.from_user_id.empty())
    {
        root["from_user_id"] = dto.from_user_id;
    }
    root["text_array"] = dto.text_array;
    return root;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupHistoryMessageDto& dto)
{
    memochat::json::JsonValue item;
    item["msgid"] = dto.msgid;
    item["groupid"] = static_cast<int64_t>(dto.groupid);
    item["fromuid"] = dto.fromuid;
    item["msgtype"] = dto.msgtype;
    item["content"] = dto.content;
    item["mentions"] = dto.mentions;
    if (!dto.file_name.empty())
    {
        item["file_name"] = dto.file_name;
    }
    if (!dto.mime.empty())
    {
        item["mime"] = dto.mime;
    }
    if (dto.size > 0)
    {
        item["size"] = static_cast<int64_t>(dto.size);
    }
    item["created_at"] = static_cast<int64_t>(dto.created_at);
    item["server_msg_id"] = static_cast<int64_t>(dto.server_msg_id);
    item["group_seq"] = static_cast<int64_t>(dto.group_seq);
    if (dto.reply_to_server_msg_id > 0)
    {
        item["reply_to_server_msg_id"] = static_cast<int64_t>(dto.reply_to_server_msg_id);
    }
    if (dto.forward_meta.present)
    {
        item["forward_meta"] = dto.forward_meta.value;
    }
    if (dto.edited_at_ms > 0)
    {
        item["edited_at_ms"] = static_cast<int64_t>(dto.edited_at_ms);
    }
    if (dto.deleted_at_ms > 0)
    {
        item["deleted_at_ms"] = static_cast<int64_t>(dto.deleted_at_ms);
    }
    item["from_name"] = dto.from_name;
    item["from_nick"] = dto.from_nick;
    item["from_icon"] = dto.from_icon;
    return item;
}

} // namespace memochat::chat::history::output
