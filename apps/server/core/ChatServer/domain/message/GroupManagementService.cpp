#include "GroupManagementService.hpp"

#include "ChatGroupCommandDtos.hpp"
#include "ChatMessageCommandDtos.hpp"
#include "GroupResponseFormatter.hpp"
#include "MessageServiceUtil.hpp"
#include "const.hpp"
#include "ports/IDeliveryGateway.hpp"
#include "ports/IRelationRepository.hpp"

#include "json/GlazeCompat.hpp"

#include <memory>
#include <vector>

namespace
{
using memochat::chat::message::JsonToWireString;
using memochat::chat::message::NowSec;
namespace ChatGroupCommand = memochat::chat::group;
namespace ChatMessageCommand = memochat::chat::command;
namespace GroupResponseFormatter = memochat::chat::message::GroupResponseFormatter;

constexpr std::size_t kMaxGroupAnnouncementBytes = 4096;

int AuthenticatedGroupRequestUidLocal(const MessageCommandRequest& request, int payload_uid)
{
    return request.session_uid > 0 ? request.session_uid : payload_uid;
}

std::vector<int> ActiveGroupRecipients(IRelationRepository* relation_repository, int64_t group_id)
{
    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    relation_repository->GetGroupMemberList(group_id, members);

    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one)
        {
            recipients.push_back(one->uid);
        }
    }
    return recipients;
}

} // namespace

GroupManagementService::GroupManagementService(IRelationRepository* relation_repository,
                                               IDeliveryGateway* delivery_gateway)
    : _relation_repository(relation_repository)
    , _delivery_gateway(delivery_gateway)
{
}

MessageCommandResult GroupManagementService::UpdateGroupAnnouncement(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupAnnouncementUpdateRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const int64_t group_id = command.group_id;
    const std::string& announcement = command.announcement;

    ChatGroupCommand::ChatGroupAnnouncementUpdateResponseDto rtdto{.error = ErrorCodes::Success,
                                                                   .groupid = group_id,
                                                                   .announcement = announcement};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_UPDATE_GROUP_ANNOUNCEMENT_RSP, JsonToWireString(rtvalue)};
    };

    if (announcement.size() > kMaxGroupAnnouncementBytes)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }

    if (!_relation_repository->UpdateGroupAnnouncement(group_id, uid, announcement))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const auto recipients = ActiveGroupRecipients(_relation_repository, group_id);
    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupAnnouncementUpdatedEventDto{.error = ErrorCodes::Success,
                                                                 .event = "group_announcement_updated",
                                                                 .groupid = group_id,
                                                                 .group_code = group_info ? group_info->group_code : "",
                                                                 .announcement = announcement,
                                                                 .operator_uid = uid});
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

MessageCommandResult GroupManagementService::UpdateGroupIcon(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupIconUpdateRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const int64_t group_id = command.group_id;
    const std::string& icon = command.icon;

    ChatGroupCommand::ChatGroupIconUpdateResponseDto rtdto{.error = ErrorCodes::Success,
                                                           .groupid = group_id,
                                                           .icon = icon};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_UPDATE_GROUP_ICON_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || group_id <= 0 || icon.empty() || icon.size() > 512)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return result();
    }
    if (!_relation_repository->UpdateGroupIcon(group_id, uid, icon))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const auto recipients = ActiveGroupRecipients(_relation_repository, group_id);
    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupIconUpdatedEventDto{.error = ErrorCodes::Success,
                                                         .event = "group_icon_updated",
                                                         .groupid = group_id,
                                                         .group_code = group_info ? group_info->group_code : "",
                                                         .icon = icon,
                                                         .operator_uid = uid});
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

MessageCommandResult GroupManagementService::SetGroupAdmin(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupSetAdminRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const std::string& target_user_id = command.target_user_id;
    const int64_t group_id = command.group_id;
    const bool is_admin = command.is_admin;
    const int64_t requested_permission_bits = command.requested_permission_bits;
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }

    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(
        ChatGroupCommand::ChatGroupSetAdminResponseDto{.error = ErrorCodes::Success,
                                                       .groupid = group_id,
                                                       .touid = target_uid,
                                                       .target_user_id = target_user_id,
                                                       .is_admin = is_admin,
                                                       .permission_bits = requested_permission_bits});
    GroupResponseFormatter::ApplyPermissionFlags(rtvalue, requested_permission_bits);
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_SET_GROUP_ADMIN_RSP, JsonToWireString(rtvalue)};
    };

    if (target_uid <= 0 || target_user_id.empty() ||
        !_relation_repository->SetGroupAdmin(group_id, uid, target_uid, is_admin, requested_permission_bits))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const auto recipients = ActiveGroupRecipients(_relation_repository, group_id);
    auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupAdminChangedEventDto{.error = ErrorCodes::Success,
                                                          .event = "group_admin_changed",
                                                          .groupid = group_id,
                                                          .group_code = group_info ? group_info->group_code : "",
                                                          .operator_uid = uid,
                                                          .target_uid = target_uid,
                                                          .target_user_id = target_user_id,
                                                          .is_admin = is_admin,
                                                          .permission_bits = requested_permission_bits});
    GroupResponseFormatter::ApplyPermissionFlags(notify, requested_permission_bits);
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

MessageCommandResult GroupManagementService::MuteGroupMember(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupMemberActionRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const std::string& target_user_id = command.target_user_id;
    const int64_t group_id = command.group_id;
    const int mute_seconds = command.mute_seconds;
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }
    const int64_t mute_until = (mute_seconds > 0) ? NowSec() + mute_seconds : 0;

    ChatGroupCommand::ChatGroupMuteMemberResponseDto rtdto{.error = ErrorCodes::Success,
                                                           .groupid = group_id,
                                                           .touid = target_uid,
                                                           .target_user_id = target_user_id,
                                                           .mute_until = mute_until};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_MUTE_GROUP_MEMBER_RSP, JsonToWireString(rtvalue)};
    };

    if (target_uid <= 0 || target_user_id.empty() ||
        !_relation_repository->MuteGroupMember(group_id, uid, target_uid, mute_until))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const auto recipients = ActiveGroupRecipients(_relation_repository, group_id);
    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMuteChangedEventDto{.error = ErrorCodes::Success,
                                                         .event = "group_mute_changed",
                                                         .groupid = group_id,
                                                         .group_code = group_info ? group_info->group_code : "",
                                                         .operator_uid = uid,
                                                         .target_uid = target_uid,
                                                         .target_user_id = target_user_id,
                                                         .mute_until = mute_until});
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

MessageCommandResult GroupManagementService::KickGroupMember(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupMemberActionRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const std::string& target_user_id = command.target_user_id;
    const int64_t group_id = command.group_id;
    int target_uid = 0;
    if (!_relation_repository->GetUidByUserId(target_user_id, target_uid))
    {
        target_uid = 0;
    }

    ChatGroupCommand::ChatGroupKickMemberResponseDto rtdto{.error = ErrorCodes::Success,
                                                           .groupid = group_id,
                                                           .touid = target_uid,
                                                           .target_user_id = target_user_id};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_KICK_GROUP_MEMBER_RSP, JsonToWireString(rtvalue)};
    };

    if (target_uid <= 0 || target_user_id.empty() || !_relation_repository->KickGroupMember(group_id, uid, target_uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    auto recipients = ActiveGroupRecipients(_relation_repository, group_id);
    recipients.push_back(target_uid);

    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMemberKickedEventDto{.error = ErrorCodes::Success,
                                                          .event = "group_member_kicked",
                                                          .groupid = group_id,
                                                          .group_code = group_info ? group_info->group_code : "",
                                                          .operator_uid = uid,
                                                          .target_uid = target_uid,
                                                          .target_user_id = target_user_id});
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

MessageCommandResult GroupManagementService::QuitGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupIdRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const int64_t group_id = command.group_id;

    ChatGroupCommand::ChatGroupQuitResponseDto rtdto{.error = ErrorCodes::Success, .groupid = group_id};
    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
    }
    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_QUIT_GROUP_RSP, JsonToWireString(rtvalue)};
    };

    if (!_relation_repository->QuitGroup(group_id, uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const auto recipients = ActiveGroupRecipients(_relation_repository, group_id);
    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupMemberQuitEventDto{.error = ErrorCodes::Success,
                                                        .event = "group_member_quit",
                                                        .groupid = group_id,
                                                        .group_code = group_info ? group_info->group_code : "",
                                                        .target_uid = uid});
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}

MessageCommandResult GroupManagementService::DissolveGroup(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupIdRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const int64_t group_id = command.group_id;

    ChatGroupCommand::ChatGroupDissolveResponseDto rtdto{.error = ErrorCodes::Success, .groupid = group_id};

    std::shared_ptr<GroupInfo> group_info;
    _relation_repository->GetGroupById(group_id, group_info);
    if (group_info)
    {
        rtdto.group_code = group_info->group_code;
        rtdto.name = group_info->name;
    }

    memochat::json::JsonValue rtvalue = ChatGroupCommand::ToJsonValue(rtdto);
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_DISSOLVE_GROUP_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || group_id <= 0 || !group_info || group_info->status != 1)
    {
        rtvalue["error"] = ErrorCodes::GroupNotFound;
        return result();
    }

    if (group_info->owner_uid != uid)
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members)
    {
        if (one && one->status == 1)
        {
            recipients.push_back(one->uid);
        }
    }

    if (!_relation_repository->DissolveGroup(group_id, uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }

    const auto notify = ChatMessageCommand::ToJsonValue(
        ChatMessageCommand::ChatGroupDissolvedEventDto{.error = ErrorCodes::Success,
                                                       .event = "group_dissolved",
                                                       .groupid = group_id,
                                                       .group_code = group_info ? group_info->group_code : "",
                                                       .name = group_info ? group_info->name : "",
                                                       .operator_uid = uid});
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
    return result();
}
