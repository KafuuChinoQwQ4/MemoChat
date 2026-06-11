#include "GroupRequestPayloads.h"

#include <QRegularExpression>
#include <QUuid>

namespace
{
constexpr int kMaxGroupNameLength = 64;
constexpr int kMaxAnnouncementLength = 1000;
constexpr int kMaxMessageContentLength = 4096;

QJsonObject buildFromUidPayload(int fromUid)
{
    QJsonObject payload;
    payload["fromuid"] = fromUid;
    return payload;
}
} // namespace

namespace memochat::group_payload
{
QString trimmedGroupName(const QString& name)
{
    return name.trimmed();
}

QString trimmedUserId(const QString& userId)
{
    return userId.trimmed();
}

QString trimmedGroupCode(const QString& groupCode)
{
    return groupCode.trimmed();
}

QString trimmedMessageId(const QString& msgId)
{
    return msgId.trimmed();
}

bool isValidUserId(const QString& userId)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    return kUserIdPattern.match(trimmedUserId(userId)).hasMatch();
}

bool isValidGroupCode(const QString& groupCode)
{
    static const QRegularExpression kGroupCodePattern("^g[1-9][0-9]{8}$");
    return kGroupCodePattern.match(trimmedGroupCode(groupCode)).hasMatch();
}

bool isValidGroupName(const QString& name)
{
    const QString value = trimmedGroupName(name);
    return !value.isEmpty() && value.size() <= kMaxGroupNameLength;
}

QJsonArray
buildMemberUserIdArray(const QVariantList& memberUserIdList, const QString& selfUserId, QString* invalidUserId)
{
    if (invalidUserId)
    {
        invalidUserId->clear();
    }

    const QString normalizedSelfUserId = trimmedUserId(selfUserId);
    QJsonArray memberUserIds;
    for (const auto& one : memberUserIdList)
    {
        const QString userId = trimmedUserId(one.toString());
        if (userId.isEmpty() || userId == normalizedSelfUserId)
        {
            continue;
        }
        if (!isValidUserId(userId))
        {
            if (invalidUserId)
            {
                *invalidUserId = userId;
            }
            return {};
        }
        memberUserIds.append(userId);
    }
    return memberUserIds;
}

qint64 normalizeAdminPermissionBits(bool isAdmin, qint64 permissionBits)
{
    if (!isAdmin)
    {
        return 0;
    }

    qint64 normalized = permissionBits;
    if (normalized <= 0)
    {
        normalized = kDefaultAdminPermissionBits;
    }
    normalized &= kOwnerPermissionBits;
    return normalized > 0 ? normalized : kDefaultAdminPermissionBits;
}

QJsonObject buildRefreshGroupListPayload(int fromUid)
{
    return buildFromUidPayload(fromUid);
}

QJsonObject buildRequestDialogListPayload(int fromUid)
{
    return buildFromUidPayload(fromUid);
}

QJsonObject buildCreateGroupPayload(int fromUid, const QString& name, const QJsonArray& memberUserIds, int memberLimit)
{
    QJsonObject payload = buildFromUidPayload(fromUid);
    payload["name"] = trimmedGroupName(name);
    payload["member_limit"] = memberLimit;
    payload["member_user_ids"] = memberUserIds;
    return payload;
}

QJsonObject
buildInviteGroupMemberPayload(int fromUid, const QString& targetUserId, qint64 groupId, const QString& reason)
{
    QJsonObject payload = buildFromUidPayload(fromUid);
    payload["target_user_id"] = trimmedUserId(targetUserId);
    payload["groupid"] = static_cast<qint64>(groupId);
    payload["reason"] = reason;
    return payload;
}

QJsonObject buildApplyJoinGroupPayload(int fromUid, const QString& groupCode, const QString& reason)
{
    QJsonObject payload = buildFromUidPayload(fromUid);
    payload["group_code"] = trimmedGroupCode(groupCode);
    payload["reason"] = reason;
    return payload;
}

QJsonObject buildReviewGroupApplyPayload(int fromUid, qint64 applyId, bool agree)
{
    QJsonObject payload = buildFromUidPayload(fromUid);
    payload["apply_id"] = applyId;
    payload["agree"] = agree;
    return payload;
}

QJsonObject buildGroupIdPayload(int fromUid, qint64 groupId)
{
    QJsonObject payload = buildFromUidPayload(fromUid);
    payload["groupid"] = static_cast<qint64>(groupId);
    return payload;
}

QJsonObject buildUpdateGroupAnnouncementPayload(int fromUid, qint64 groupId, const QString& announcement)
{
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["announcement"] = announcement.left(kMaxAnnouncementLength);
    return payload;
}

QJsonObject buildUpdateGroupIconPayload(int fromUid, qint64 groupId, const QString& icon)
{
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["icon"] = icon;
    return payload;
}

QJsonObject
buildSetGroupAdminPayload(int fromUid, qint64 groupId, const QString& targetUserId, bool isAdmin, qint64 permissionBits)
{
    const qint64 normalizedPermBits = normalizeAdminPermissionBits(isAdmin, permissionBits);
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["target_user_id"] = trimmedUserId(targetUserId);
    payload["is_admin"] = isAdmin;
    payload["permission_bits"] = normalizedPermBits;
    payload["can_change_group_info"] = (normalizedPermBits & kPermissionChangeGroupInfo) != 0;
    payload["can_delete_messages"] = (normalizedPermBits & kPermissionDeleteMessages) != 0;
    payload["can_invite_users"] = (normalizedPermBits & kPermissionInviteUsers) != 0;
    payload["can_manage_admins"] = (normalizedPermBits & kPermissionManageAdmins) != 0;
    payload["can_pin_messages"] = (normalizedPermBits & kPermissionPinMessages) != 0;
    payload["can_ban_users"] = (normalizedPermBits & kPermissionBanUsers) != 0;
    payload["can_manage_topics"] = (normalizedPermBits & kPermissionManageTopics) != 0;
    return payload;
}

QJsonObject buildMuteGroupMemberPayload(int fromUid, qint64 groupId, const QString& targetUserId, int muteSeconds)
{
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["target_user_id"] = trimmedUserId(targetUserId);
    payload["mute_seconds"] = qMax(0, muteSeconds);
    return payload;
}

QJsonObject buildKickGroupMemberPayload(int fromUid, qint64 groupId, const QString& targetUserId)
{
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["target_user_id"] = trimmedUserId(targetUserId);
    return payload;
}

QJsonObject buildGroupEditMessagePayload(int fromUid, qint64 groupId, const QString& msgId, const QString& content)
{
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["msgid"] = trimmedMessageId(msgId);
    payload["content"] = content.left(kMaxMessageContentLength);
    return payload;
}

QJsonObject buildGroupRevokeMessagePayload(int fromUid, qint64 groupId, const QString& msgId)
{
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["msgid"] = trimmedMessageId(msgId);
    return payload;
}

QJsonObject buildGroupForwardMessagePayload(int fromUid, qint64 groupId, const QString& msgId)
{
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["msgid"] = trimmedMessageId(msgId);
    payload["client_msg_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return payload;
}

QJsonObject buildGroupHistoryPayload(int fromUid, qint64 groupId, qint64 beforeSeq, int limit)
{
    QJsonObject payload = buildGroupIdPayload(fromUid, groupId);
    payload["before_ts"] = 0;
    payload["before_seq"] = static_cast<qint64>(beforeSeq);
    payload["limit"] = limit;
    return payload;
}

QString actionText(ReqId reqId)
{
    switch (reqId)
    {
        case ID_CREATE_GROUP_RSP:
            return "建群";
        case ID_GET_GROUP_LIST_RSP:
            return "刷新群列表";
        case ID_INVITE_GROUP_MEMBER_RSP:
            return "邀请成员";
        case ID_APPLY_JOIN_GROUP_RSP:
            return "申请入群";
        case ID_REVIEW_GROUP_APPLY_RSP:
            return "审核申请";
        case ID_GROUP_CHAT_MSG_RSP:
            return "发送群消息";
        case ID_GROUP_HISTORY_RSP:
            return "拉取群历史";
        case ID_EDIT_GROUP_MSG_RSP:
            return "编辑群消息";
        case ID_REVOKE_GROUP_MSG_RSP:
            return "撤回群消息";
        case ID_FORWARD_GROUP_MSG_RSP:
            return "转发群消息";
        case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
            return "更新群公告";
        case ID_UPDATE_GROUP_ICON_RSP:
            return "更新群头像";
        case ID_MUTE_GROUP_MEMBER_RSP:
            return "禁言成员";
        case ID_SET_GROUP_ADMIN_RSP:
            return "设置管理员";
        case ID_KICK_GROUP_MEMBER_RSP:
            return "踢出成员";
        case ID_QUIT_GROUP_RSP:
            return "退出群聊";
        case ID_DISSOLVE_GROUP_RSP:
            return "解散群聊";
        default:
            return "群操作";
    }
}
} // namespace memochat::group_payload
