#include "GroupResponseFormatter.h"

namespace memochat::chat::message::GroupResponseFormatter
{

void ApplyPermissionFlags(memochat::json::JsonValue& out, int64_t permission_bits)
{
    out["can_change_group_info"] = (permission_bits & kPermChangeGroupInfo) != 0;
    out["can_delete_messages"] = (permission_bits & kPermDeleteMessages) != 0;
    out["can_invite_users"] = (permission_bits & kPermInviteUsers) != 0;
    out["can_manage_admins"] = (permission_bits & kPermManageAdmins) != 0;
    out["can_pin_messages"] = (permission_bits & kPermPinMessages) != 0;
    out["can_ban_users"] = (permission_bits & kPermBanUsers) != 0;
    out["can_manage_topics"] = (permission_bits & kPermManageTopics) != 0;
}

memochat::json::JsonValue SerializeHistoryMessage(const GroupMessageInfo& msg)
{
    memochat::json::JsonValue item;
    item["msgid"] = msg.msg_id;
    item["groupid"] = static_cast<int64_t>(msg.group_id);
    item["fromuid"] = msg.from_uid;
    item["msgtype"] = msg.msg_type;
    item["content"] = msg.content;
    memochat::json::JsonValue mentions(memochat::json::array_t{});
    if (!msg.mentions_json.empty())
    {
        memochat::json::JsonReader mentions_reader;
        memochat::json::JsonValue parsed_mentions;
        if (mentions_reader.parse(msg.mentions_json, parsed_mentions) && parsed_mentions.is_array())
        {
            mentions = parsed_mentions;
        }
    }
    item["mentions"] = mentions;
    if (!msg.file_name.empty())
    {
        item["file_name"] = msg.file_name;
    }
    if (!msg.mime.empty())
    {
        item["mime"] = msg.mime;
    }
    if (msg.size > 0)
    {
        item["size"] = msg.size;
    }
    item["created_at"] = static_cast<int64_t>(msg.created_at);
    item["server_msg_id"] = static_cast<int64_t>(msg.server_msg_id);
    item["group_seq"] = static_cast<int64_t>(msg.group_seq);
    if (msg.reply_to_server_msg_id > 0)
    {
        item["reply_to_server_msg_id"] = static_cast<int64_t>(msg.reply_to_server_msg_id);
    }
    if (!msg.forward_meta_json.empty())
    {
        memochat::json::JsonReader forward_reader;
        memochat::json::JsonValue forward_meta;
        if (forward_reader.parse(msg.forward_meta_json, forward_meta))
        {
            item["forward_meta"] = forward_meta;
        }
    }
    if (msg.edited_at_ms > 0)
    {
        item["edited_at_ms"] = static_cast<int64_t>(msg.edited_at_ms);
    }
    if (msg.deleted_at_ms > 0)
    {
        item["deleted_at_ms"] = static_cast<int64_t>(msg.deleted_at_ms);
    }
    item["from_name"] = msg.from_name;
    item["from_nick"] = msg.from_nick;
    item["from_icon"] = msg.from_icon;
    return item;
}

} // namespace memochat::chat::message::GroupResponseFormatter
