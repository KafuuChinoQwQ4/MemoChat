#include "GroupResponseFormatter.hpp"

#include "ChatHistoryOutputDtos.hpp"

import memochat.chat.group_response_algorithms;

namespace memochat::chat::message::GroupResponseFormatter
{

void ApplyPermissionFlags(memochat::json::JsonValue& out, int64_t permission_bits)
{
    out["can_change_group_info"] =
        group_response::modules::HasPermissionBit(permission_bits, memochat::chat::output::kGroupPermChangeGroupInfo);
    out["can_delete_messages"] =
        group_response::modules::HasPermissionBit(permission_bits, memochat::chat::output::kGroupPermDeleteMessages);
    out["can_invite_users"] =
        group_response::modules::HasPermissionBit(permission_bits, memochat::chat::output::kGroupPermInviteUsers);
    out["can_manage_admins"] =
        group_response::modules::HasPermissionBit(permission_bits, memochat::chat::output::kGroupPermManageAdmins);
    out["can_pin_messages"] =
        group_response::modules::HasPermissionBit(permission_bits, memochat::chat::output::kGroupPermPinMessages);
    out["can_ban_users"] =
        group_response::modules::HasPermissionBit(permission_bits, memochat::chat::output::kGroupPermBanUsers);
    out["can_manage_topics"] =
        group_response::modules::HasPermissionBit(permission_bits, memochat::chat::output::kGroupPermManageTopics);
}

memochat::json::JsonValue SerializeHistoryMessage(const GroupMessageInfo& msg)
{
    return memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatGroupHistoryMessageFromInfo(msg));
}

} // namespace memochat::chat::message::GroupResponseFormatter
