#include "GroupMembershipService.hpp"

#include "ChatGroupCommandDtos.hpp"
#include "ChatMessageCommandDtos.hpp"
#include "ChatRelationGroupDtos.hpp"
#include "MessageServiceUtil.hpp"
#include "const.hpp"
#include "ports/IDeliveryGateway.hpp"
#include "ports/IRelationRepository.hpp"

#include "json/GlazeCompat.hpp"

#include <memory>
#include <optional>
#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{
using memochat::chat::message::JsonToWireString;
namespace ChatGroupCommand = memochat::chat::group;
namespace ChatMessageCommand = memochat::chat::command;
namespace ChatOutput = memochat::chat::output;

constexpr std::size_t kMaxGroupAnnouncementBytes = 4096;

int AuthenticatedGroupRequestUidLocal(const MessageCommandRequest& request, int payload_uid)
{
    return request.session_uid > 0 ? request.session_uid : payload_uid;
}
} // namespace

GroupMembershipService::GroupMembershipService(IRelationRepository* relation_repository,
                                               IDeliveryGateway* delivery_gateway)
    : _relation_repository(relation_repository)
    , _delivery_gateway(delivery_gateway)
{
}

void GroupMembershipService::BuildGroupListJson(int uid, memochat::json::JsonValue& out)
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

MessageCommandResult GroupMembershipService::CreateGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);

    const auto command = ChatGroupCommand::ChatGroupCreateRequestFromJsonValue(root);
    const int owner_uid = AuthenticatedGroupRequestUidLocal(request, command.owner_uid);
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

    if (owner_uid <= 0 || group_name.empty() || group_name.size() > 64 ||
        announcement.size() > kMaxGroupAnnouncementBytes || invalid_member_user_id)
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

MessageCommandResult GroupMembershipService::GetGroupList(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupListRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);

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

MessageCommandResult GroupMembershipService::InviteGroupMember(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupInviteMemberRequestFromJsonValue(root);
    const int from_uid = AuthenticatedGroupRequestUidLocal(request, command.from_uid);
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

MessageCommandResult GroupMembershipService::ApplyJoinGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupApplyJoinRequestFromJsonValue(root);
    const int from_uid = AuthenticatedGroupRequestUidLocal(request, command.from_uid);
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

MessageCommandResult GroupMembershipService::ReviewGroupApply(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupReviewApplyRequestFromJsonValue(root);
    const int reviewer_uid = AuthenticatedGroupRequestUidLocal(request, command.reviewer_uid);
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
