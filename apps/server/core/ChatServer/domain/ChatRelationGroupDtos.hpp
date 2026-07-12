#pragma once

#include "json/GlazeCompat.hpp"
#include "json/TypedJsonCodec.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace memochat::chat::output
{

inline constexpr int64_t kGroupPermChangeGroupInfo = 1LL << 0;
inline constexpr int64_t kGroupPermDeleteMessages = 1LL << 1;
inline constexpr int64_t kGroupPermInviteUsers = 1LL << 2;
inline constexpr int64_t kGroupPermManageAdmins = 1LL << 3;
inline constexpr int64_t kGroupPermPinMessages = 1LL << 4;
inline constexpr int64_t kGroupPermBanUsers = 1LL << 5;
inline constexpr int64_t kGroupPermManageTopics = 1LL << 6;
inline constexpr int64_t kDefaultAdminPermBits = kGroupPermChangeGroupInfo | kGroupPermDeleteMessages |
                                                 kGroupPermInviteUsers | kGroupPermPinMessages | kGroupPermBanUsers;

struct ChatGroupListRowDto
{
    int64_t groupid = 0;
    std::string group_code;
    std::string name;
    std::string icon;
    int owner_uid = 0;
    std::string announcement;
    int member_limit = 0;
    int member_count = 0;
    int is_all_muted = 0;
    int role = 0;
    int64_t permission_bits = 0;
};

struct ChatPendingGroupApplyRowDto
{
    int64_t apply_id = 0;
    int64_t groupid = 0;
    std::optional<std::string> group_code;
    int applicant_uid = 0;
    int inviter_uid = 0;
    std::optional<std::string> applicant_user_id;
    std::optional<std::string> inviter_user_id;
    std::string type;
    int status = 0;
    std::string reason;
};

struct ChatRelationApplyRowDto
{
    std::string name;
    int uid = 0;
    std::string user_id;
    std::string icon;
    std::string nick;
    int sex = 0;
    std::string desc;
    int status = 0;
    std::vector<std::string> labels;
};

struct ChatRelationFriendRowDto
{
    std::string name;
    int uid = 0;
    std::string user_id;
    std::string icon;
    std::string nick;
    int sex = 0;
    std::string desc;
    std::string back;
    std::vector<std::string> labels;
};

struct ChatRelationBootstrapDto
{
    std::vector<ChatRelationApplyRowDto> apply_list;
    std::vector<ChatRelationFriendRowDto> friend_list;
};

struct ChatDialogRowDto
{
    std::string dialog_id;
    std::string dialog_type;
    std::optional<int> peer_uid;
    std::optional<int64_t> group_id;
    std::string title;
    std::string avatar;
    std::string last_msg_preview;
    int64_t last_msg_ts = 0;
    int unread_count = 0;
    int pinned_rank = 0;
    std::string draft_text;
    int mute_state = 0;
};

template <typename T> inline bool WriteChatOutputJsonNoThrow(const T& value, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(value, out, error_out);
}

template <typename T> inline memochat::json::JsonValue ChatOutputDtoToJsonValue(const T& value)
{
    std::string body;
    if (!WriteChatOutputJsonNoThrow(value, &body, nullptr))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return root;
}

inline void AppendGroupPermissionFlags(memochat::json::JsonValue& out, int64_t permission_bits)
{
    out["can_change_group_info"] = (permission_bits & kGroupPermChangeGroupInfo) != 0;
    out["can_delete_messages"] = (permission_bits & kGroupPermDeleteMessages) != 0;
    out["can_invite_users"] = (permission_bits & kGroupPermInviteUsers) != 0;
    out["can_manage_admins"] = (permission_bits & kGroupPermManageAdmins) != 0;
    out["can_pin_messages"] = (permission_bits & kGroupPermPinMessages) != 0;
    out["can_ban_users"] = (permission_bits & kGroupPermBanUsers) != 0;
    out["can_manage_topics"] = (permission_bits & kGroupPermManageTopics) != 0;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupListRowDto& row)
{
    memochat::json::JsonValue out = ChatOutputDtoToJsonValue(row);
    AppendGroupPermissionFlags(out, row.permission_bits);
    return out;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPendingGroupApplyRowDto& row)
{
    return ChatOutputDtoToJsonValue(row);
}

inline memochat::json::JsonValue ToJsonValue(const ChatRelationApplyRowDto& row)
{
    return ChatOutputDtoToJsonValue(row);
}

inline memochat::json::JsonValue ToJsonValue(const ChatRelationFriendRowDto& row)
{
    return ChatOutputDtoToJsonValue(row);
}

inline memochat::json::JsonValue ToJsonValue(const ChatRelationBootstrapDto& dto)
{
    memochat::json::JsonValue out(memochat::json::object_t{});
    out["apply_list"] = memochat::json::array_t{};
    out["friend_list"] = memochat::json::array_t{};
    for (const auto& row : dto.apply_list)
    {
        out["apply_list"].append(ToJsonValue(row));
    }
    for (const auto& row : dto.friend_list)
    {
        out["friend_list"].append(ToJsonValue(row));
    }
    memochat::json::JsonValue normalized;
    if (memochat::json::glaze_parse(normalized, memochat::json::glaze_stringify(out)) && normalized.isObject())
    {
        return normalized;
    }
    return out;
}

inline memochat::json::JsonValue ToJsonValue(const ChatDialogRowDto& row)
{
    return ChatOutputDtoToJsonValue(row);
}

} // namespace memochat::chat::output
