#pragma once

#include "ChatRelationGroupDtos.h"
#include "data.h"
#include "json/GlazeCompat.h"

#include <cstdint>

// Response/JSON building helpers extracted from GroupMessageService for readability.
// This is a pure formatting layer: it serializes domain structs into the wire JSON
// shapes the group message responses use. Field names and insertion order MUST stay
// byte-identical to the previous inline construction in GroupMessageService.cpp.
namespace memochat::chat::message::GroupResponseFormatter
{

// Group permission bits, used to derive the boolean can_* response flags.
// (Message-layer copy; previously the kPerm*Local constants in GroupMessageService.cpp.)
inline constexpr int64_t kPermChangeGroupInfo = memochat::chat::output::kGroupPermChangeGroupInfo;
inline constexpr int64_t kPermDeleteMessages = memochat::chat::output::kGroupPermDeleteMessages;
inline constexpr int64_t kPermInviteUsers = memochat::chat::output::kGroupPermInviteUsers;
inline constexpr int64_t kPermManageAdmins = memochat::chat::output::kGroupPermManageAdmins;
inline constexpr int64_t kPermPinMessages = memochat::chat::output::kGroupPermPinMessages;
inline constexpr int64_t kPermBanUsers = memochat::chat::output::kGroupPermBanUsers;
inline constexpr int64_t kPermManageTopics = memochat::chat::output::kGroupPermManageTopics;
inline constexpr int64_t kDefaultAdminPermBits = memochat::chat::output::kDefaultAdminPermBits;

// Append the seven boolean can_* permission flags derived from `permission_bits`.
// Inserts the keys in the same order they were previously written inline.
void ApplyPermissionFlags(memochat::json::JsonValue& out, int64_t permission_bits);

// Serialize a single group history message into its wire JSON item.
// Mirrors the previous inline construction in GroupMessageService::GroupHistory,
// including the from_name/from_nick/from_icon fields taken from `msg`. The caller
// is responsible for the repository fallback that fills missing sender display info.
memochat::json::JsonValue SerializeHistoryMessage(const GroupMessageInfo& msg);

} // namespace memochat::chat::message::GroupResponseFormatter
