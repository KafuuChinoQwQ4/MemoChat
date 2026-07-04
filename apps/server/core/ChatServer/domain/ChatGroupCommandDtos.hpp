#pragma once

#include "json/GlazeCompat.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace memochat::chat::group
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

struct ChatGroupCreateRequestDto
{
    int owner_uid = 0;
    std::string name;
    std::string announcement;
    int member_limit = 200;
    std::vector<std::string> member_user_ids;
};

struct ChatGroupListRequestDto
{
    int uid = 0;
};

struct ChatGroupInviteMemberRequestDto
{
    int from_uid = 0;
    std::string target_user_id;
    int64_t group_id = 0;
    std::string reason;
};

struct ChatGroupApplyJoinRequestDto
{
    int from_uid = 0;
    std::string group_code;
    std::string reason;
};

struct ChatGroupReviewApplyRequestDto
{
    int reviewer_uid = 0;
    int64_t apply_id = 0;
    bool agree = false;
};

struct ChatGroupReadAckRequestDto
{
    int uid = 0;
    int64_t group_id = 0;
    int64_t read_ts = 0;
};

struct ChatGroupAnnouncementUpdateRequestDto
{
    int uid = 0;
    int64_t group_id = 0;
    std::string announcement;
};

struct ChatGroupIconUpdateRequestDto
{
    int uid = 0;
    int64_t group_id = 0;
    std::string icon;
};

struct ChatGroupIdRequestDto
{
    int uid = 0;
    int64_t group_id = 0;
};

struct ChatGroupSetAdminRequestDto
{
    int uid = 0;
    std::string target_user_id;
    int64_t group_id = 0;
    bool is_admin = true;
    bool has_permission_bits = false;
    int64_t permission_bits = 0;
    int64_t requested_permission_bits = 0;
};

struct ChatGroupMemberActionRequestDto
{
    int uid = 0;
    std::string target_user_id;
    int64_t group_id = 0;
    int mute_seconds = 0;
};

struct ChatGroupInviteMemberResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    int touid = 0;
    std::string target_user_id;
};

struct ChatGroupApplyJoinResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    std::string group_code;
};

struct ChatGroupAnnouncementUpdateResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    std::string announcement;
    std::optional<std::string> group_code;
};

struct ChatGroupIconUpdateResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    std::string icon;
    std::optional<std::string> group_code;
};

struct ChatGroupQuitResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    std::optional<std::string> group_code;
};

struct ChatGroupDissolveResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    std::optional<std::string> group_code;
    std::optional<std::string> name;
};

struct ChatGroupMuteMemberResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    int touid = 0;
    std::string target_user_id;
    int64_t mute_until = 0;
    std::optional<std::string> group_code;
};

struct ChatGroupKickMemberResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    int touid = 0;
    std::string target_user_id;
    std::optional<std::string> group_code;
};

// SetGroupAdmin owns only the fixed scalar shell. The derived can_* permission
// flags are appended by GroupResponseFormatter::ApplyPermissionFlags() and the
// caller-gated group_code is appended after them, both preserving the current
// wire key order (... permission_bits, can_*, group_code).
struct ChatGroupSetAdminResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    int touid = 0;
    std::string target_user_id;
    bool is_admin = false;
    int64_t permission_bits = 0;
};

// GetGroupList owns only the leading error scalar. The group_list and
// pending_group_apply_list arrays are appended by BuildGroupListJson() after the
// shell, preserving wire order (object_t is insertion-ordered).
struct ChatGroupListResponseDto
{
    int error = 0;
};

// GroupHistory owns only the leading scalar shell. The dynamic messages array is
// assigned by the caller immediately after the shell (its current wire position,
// between next_before_seq and the caller-gated group_code), and group_code is
// appended later. The DTO deliberately excludes messages and group_code so the
// field inventory == the serialized scalar fields and wire order is preserved.
struct ChatGroupHistoryResponseDto
{
    int error = 0;
    int64_t groupid = 0;
    bool has_more = false;
    int64_t next_before_seq = 0;
};

// CreateGroup response root. {error} shell on early-return; the success path adds
// the fixed tail {groupid, group_code, name, announcement} then BuildGroupListJson
// appends the dynamic group_list/pending_group_apply_list arrays after it. The DTO
// owns only error + the caller-gated success tail; the arrays stay caller-applied.
struct ChatGroupCreateResponseDto
{
    int error = 0;
    std::optional<int64_t> groupid;
    std::optional<std::string> group_code;
    std::optional<std::string> name;
    std::optional<std::string> announcement;
};

// ReviewGroupApply response root. {error, apply_id, agree} shell built once; the
// success path adds groupid + applicant_uid plus caller-gated group_code /
// applicant_user_id. Error branches overwrite only error.
struct ChatGroupReviewApplyResponseDto
{
    int error = 0;
    int64_t apply_id = 0;
    bool agree = false;
    std::optional<int64_t> groupid;
    std::optional<int> applicant_uid;
    std::optional<std::string> group_code;
    std::optional<std::string> applicant_user_id;
};

inline int ReadFromUidOrUid(const memochat::json::JsonValue& root)
{
    return root["fromuid"].asInt();
}

inline ChatGroupCreateRequestDto ChatGroupCreateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupCreateRequestDto request;
    request.owner_uid = root["fromuid"].asInt();
    request.name = root["name"].asString();
    request.announcement = root.get("announcement", "").asString();
    request.member_limit = root.get("member_limit", 200).asInt();
    if (memochat::json::isMember(root, "member_user_ids") && root["member_user_ids"].is_array())
    {
        for (const auto& one : root["member_user_ids"])
        {
            request.member_user_ids.push_back(one.asString());
        }
    }
    return request;
}

inline void MergeGroupPermissionFlag(const memochat::json::JsonValue& root,
                                     const char* key,
                                     int64_t bit,
                                     bool& has_permission_bits,
                                     int64_t& permission_bits)
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
}

inline void
MergeGroupPermissionFlags(const memochat::json::JsonValue& root, bool& has_permission_bits, int64_t& permission_bits)
{
    MergeGroupPermissionFlag(root,
                             "can_change_group_info",
                             kGroupPermChangeGroupInfo,
                             has_permission_bits,
                             permission_bits);
    MergeGroupPermissionFlag(root,
                             "can_delete_messages",
                             kGroupPermDeleteMessages,
                             has_permission_bits,
                             permission_bits);
    MergeGroupPermissionFlag(root, "can_invite_users", kGroupPermInviteUsers, has_permission_bits, permission_bits);
    MergeGroupPermissionFlag(root, "can_manage_admins", kGroupPermManageAdmins, has_permission_bits, permission_bits);
    MergeGroupPermissionFlag(root, "can_pin_messages", kGroupPermPinMessages, has_permission_bits, permission_bits);
    MergeGroupPermissionFlag(root, "can_ban_users", kGroupPermBanUsers, has_permission_bits, permission_bits);
    MergeGroupPermissionFlag(root, "can_manage_topics", kGroupPermManageTopics, has_permission_bits, permission_bits);
}

inline ChatGroupListRequestDto ChatGroupListRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupListRequestDto request;
    request.uid = root["fromuid"].asInt();
    return request;
}

inline ChatGroupInviteMemberRequestDto ChatGroupInviteMemberRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupInviteMemberRequestDto request;
    request.from_uid = root["fromuid"].asInt();
    request.target_user_id = root.get("target_user_id", "").asString();
    request.group_id = root["groupid"].asInt64();
    request.reason = root.get("reason", "").asString();
    return request;
}

inline ChatGroupApplyJoinRequestDto ChatGroupApplyJoinRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupApplyJoinRequestDto request;
    request.from_uid = root["fromuid"].asInt();
    request.group_code = root.get("group_code", "").asString();
    request.reason = root.get("reason", "").asString();
    return request;
}

inline ChatGroupReviewApplyRequestDto ChatGroupReviewApplyRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupReviewApplyRequestDto request;
    request.reviewer_uid = root["fromuid"].asInt();
    request.apply_id = root["apply_id"].asInt64();
    request.agree = memochat::json::glaze_safe_get<bool>(root, "agree", false);
    return request;
}

inline ChatGroupReadAckRequestDto ChatGroupReadAckRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupReadAckRequestDto request;
    request.uid = ReadFromUidOrUid(root);
    request.group_id = root.get("groupid", 0).asInt64();
    request.read_ts = root.get("read_ts", 0).asInt64();
    return request;
}

inline ChatGroupAnnouncementUpdateRequestDto
ChatGroupAnnouncementUpdateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupAnnouncementUpdateRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.group_id = root["groupid"].asInt64();
    request.announcement = root.get("announcement", "").asString();
    return request;
}

inline ChatGroupIconUpdateRequestDto ChatGroupIconUpdateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupIconUpdateRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.group_id = root["groupid"].asInt64();
    request.icon = root.get("icon", "").asString();
    return request;
}

inline ChatGroupIdRequestDto ChatGroupIdRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupIdRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.group_id = root["groupid"].asInt64();
    return request;
}

inline ChatGroupSetAdminRequestDto ChatGroupSetAdminRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupSetAdminRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.target_user_id = root.get("target_user_id", "").asString();
    request.group_id = root["groupid"].asInt64();
    request.is_admin = memochat::json::glaze_safe_get<bool>(root, "is_admin", true);
    request.has_permission_bits = memochat::json::isMember(root, "permission_bits");
    request.permission_bits = root.get("permission_bits", 0).asInt64();
    MergeGroupPermissionFlags(root, request.has_permission_bits, request.permission_bits);
    if (!request.is_admin)
    {
        request.permission_bits = 0;
        request.has_permission_bits = true;
    }
    request.requested_permission_bits = request.has_permission_bits ? request.permission_bits : 0;
    if (request.is_admin && !request.has_permission_bits && request.requested_permission_bits <= 0)
    {
        request.requested_permission_bits = kDefaultAdminPermBits;
    }
    return request;
}

inline ChatGroupMemberActionRequestDto ChatGroupMemberActionRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupMemberActionRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.target_user_id = root.get("target_user_id", "").asString();
    request.group_id = root["groupid"].asInt64();
    request.mute_seconds = root.get("mute_seconds", 0).asInt();
    return request;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupInviteMemberResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["touid"] = dto.touid;
    value["target_user_id"] = dto.target_user_id;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupApplyJoinResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["group_code"] = dto.group_code;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupAnnouncementUpdateResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["announcement"] = dto.announcement;
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupIconUpdateResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["icon"] = dto.icon;
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupQuitResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupDissolveResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    if (dto.name.has_value())
    {
        value["name"] = *dto.name;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupMuteMemberResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["touid"] = dto.touid;
    value["target_user_id"] = dto.target_user_id;
    value["mute_until"] = static_cast<int64_t>(dto.mute_until);
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatGroupKickMemberResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["touid"] = dto.touid;
    value["target_user_id"] = dto.target_user_id;
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    return value;
}

// Emits only the fixed scalar shell in declared order. The caller appends the
// can_* flags via GroupResponseFormatter::ApplyPermissionFlags() and then the
// optional group_code, preserving the existing wire order.
inline memochat::json::JsonValue ToJsonValue(const ChatGroupSetAdminResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["touid"] = dto.touid;
    value["target_user_id"] = dto.target_user_id;
    value["is_admin"] = dto.is_admin;
    value["permission_bits"] = static_cast<int64_t>(dto.permission_bits);
    return value;
}

// Emits only the leading error scalar; BuildGroupListJson() appends the two
// arrays after this shell.
inline memochat::json::JsonValue ToJsonValue(const ChatGroupListResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    return value;
}

// Emits only the leading scalar shell in declared order. The caller assigns the
// dynamic messages array next and appends the caller-gated group_code afterward.
inline memochat::json::JsonValue ToJsonValue(const ChatGroupHistoryResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["groupid"] = static_cast<int64_t>(dto.groupid);
    value["has_more"] = dto.has_more;
    value["next_before_seq"] = static_cast<int64_t>(dto.next_before_seq);
    return value;
}

// Emits error then the caller-gated success tail (groupid, group_code, name,
// announcement) in declared order; the caller appends the dynamic group list
// arrays via BuildGroupListJson afterward.
inline memochat::json::JsonValue ToJsonValue(const ChatGroupCreateResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    if (dto.groupid.has_value())
    {
        value["groupid"] = static_cast<int64_t>(*dto.groupid);
    }
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    if (dto.name.has_value())
    {
        value["name"] = *dto.name;
    }
    if (dto.announcement.has_value())
    {
        value["announcement"] = *dto.announcement;
    }
    return value;
}

// Emits error, apply_id, agree, then the caller-gated success fields in declared
// order (groupid, applicant_uid, group_code, applicant_user_id).
inline memochat::json::JsonValue ToJsonValue(const ChatGroupReviewApplyResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["apply_id"] = static_cast<int64_t>(dto.apply_id);
    value["agree"] = dto.agree;
    if (dto.groupid.has_value())
    {
        value["groupid"] = static_cast<int64_t>(*dto.groupid);
    }
    if (dto.applicant_uid.has_value())
    {
        value["applicant_uid"] = *dto.applicant_uid;
    }
    if (dto.group_code.has_value())
    {
        value["group_code"] = *dto.group_code;
    }
    if (dto.applicant_user_id.has_value())
    {
        value["applicant_user_id"] = *dto.applicant_user_id;
    }
    return value;
}

} // namespace memochat::chat::group
