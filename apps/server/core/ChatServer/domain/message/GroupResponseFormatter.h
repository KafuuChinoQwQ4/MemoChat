#pragma once

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
inline constexpr int64_t kPermChangeGroupInfo = 1LL << 0;
inline constexpr int64_t kPermDeleteMessages = 1LL << 1;
inline constexpr int64_t kPermInviteUsers = 1LL << 2;
inline constexpr int64_t kPermManageAdmins = 1LL << 3;
inline constexpr int64_t kPermPinMessages = 1LL << 4;
inline constexpr int64_t kPermBanUsers = 1LL << 5;
inline constexpr int64_t kPermManageTopics = 1LL << 6;
inline constexpr int64_t kDefaultAdminPermBits =
    kPermChangeGroupInfo | kPermDeleteMessages | kPermInviteUsers | kPermPinMessages | kPermBanUsers;

// Append the seven boolean can_* permission flags derived from `permission_bits`.
// Inserts the keys in the same order they were previously written inline.
void ApplyPermissionFlags(memochat::json::JsonValue& out, int64_t permission_bits);

// Serialize a single group history message into its wire JSON item.
// Mirrors the previous inline construction in GroupMessageService::GroupHistory,
// including the from_name/from_nick/from_icon fields taken from `msg`. The caller
// is responsible for the repository fallback that fills missing sender display info.
memochat::json::JsonValue SerializeHistoryMessage(const GroupMessageInfo& msg);

} // namespace memochat::chat::message::GroupResponseFormatter
