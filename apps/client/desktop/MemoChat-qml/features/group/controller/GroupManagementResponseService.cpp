#include "GroupManagementResponseService.h"

#include "GroupRequestPayloads.h"

#include <QVariant>

namespace memochat::group
{
namespace
{
qint64 payloadGroupId(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("groupid")).toVariant().toLongLong();
}

QString payloadGroupName(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("name")).toString().trimmed();
}

void setSuccessStatus(GroupManagementResponseEffect& effect, const QString& text)
{
    effect.hasStatus = true;
    effect.statusText = text;
    effect.statusIsError = false;
}

void requestGroupRefresh(GroupManagementResponseEffect& effect)
{
    effect.refreshGroupList = true;
}

void requestVisibleGroupRefresh(GroupManagementResponseEffect& effect)
{
    effect.refreshGroupList = true;
    effect.refreshGroupModel = true;
    effect.refreshDialogModel = true;
}

void removeGroupAndConversation(GroupManagementResponseEffect& effect, qint64 groupId)
{
    if (groupId <= 0)
    {
        return;
    }
    effect.removeGroupIds.push_back(groupId);
    effect.clearGroupConversationIds.push_back(groupId);
}
} // namespace

QString GroupManagementResponseService::defaultGroupIcon()
{
    return QStringLiteral("qrc:/res/chat_icon.png");
}

QString GroupManagementResponseService::normalizedGroupIcon(const QString& icon)
{
    const QString trimmedIcon = icon.trimmed();
    return trimmedIcon.isEmpty() ? defaultGroupIcon() : icon;
}

bool GroupManagementResponseService::isManagementResponse(int reqId)
{
    switch (static_cast<ReqId>(reqId))
    {
        case ID_CREATE_GROUP_RSP:
        case ID_INVITE_GROUP_MEMBER_RSP:
        case ID_APPLY_JOIN_GROUP_RSP:
        case ID_REVIEW_GROUP_APPLY_RSP:
        case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
        case ID_UPDATE_GROUP_ICON_RSP:
        case ID_MUTE_GROUP_MEMBER_RSP:
        case ID_SET_GROUP_ADMIN_RSP:
        case ID_KICK_GROUP_MEMBER_RSP:
        case ID_QUIT_GROUP_RSP:
        case ID_DISSOLVE_GROUP_RSP:
            return true;
        default:
            return false;
    }
}

QString GroupManagementResponseService::responseActionText(int reqId)
{
    return memochat::group_payload::actionText(static_cast<ReqId>(reqId));
}

GroupManagementResponseEffect
GroupManagementResponseService::reduceSuccess(int reqId,
                                              const QJsonObject& payload,
                                              const GroupManagementResponseContext& context)
{
    GroupManagementResponseEffect effect;
    if (!isManagementResponse(reqId))
    {
        return effect;
    }

    effect.handled = true;
    switch (static_cast<ReqId>(reqId))
    {
        case ID_CREATE_GROUP_RSP:
        {
            const qint64 groupId = payloadGroupId(payload);
            const QString groupName = payloadGroupName(payload);
            setSuccessStatus(effect,
                             groupName.isEmpty()
                             ? QStringLiteral("群聊创建成功")
                             : QStringLiteral("群聊创建成功：%1").arg(groupName));
            requestVisibleGroupRefresh(effect);
            if (context.dialogUidForCreatedGroup != 0)
            {
                effect.selectDialogUid = true;
                effect.dialogUid = context.dialogUidForCreatedGroup;
            }
            if (groupId > 0)
            {
                effect.emitGroupCreatedId = true;
                effect.createdGroupId = groupId;
            }
            return effect;
        }
        case ID_INVITE_GROUP_MEMBER_RSP:
            setSuccessStatus(effect, QStringLiteral("邀请成员成功"));
            requestGroupRefresh(effect);
            return effect;
        case ID_APPLY_JOIN_GROUP_RSP:
            setSuccessStatus(effect, QStringLiteral("入群申请已提交"));
            return effect;
        case ID_REVIEW_GROUP_APPLY_RSP:
            setSuccessStatus(effect, QStringLiteral("审核操作已完成"));
            requestGroupRefresh(effect);
            return effect;
        case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
            setSuccessStatus(effect, QStringLiteral("群公告已更新"));
            requestGroupRefresh(effect);
            return effect;
        case ID_UPDATE_GROUP_ICON_RSP:
        {
            const qint64 groupId = payloadGroupId(payload);
            if (context.currentGroupId > 0 && context.currentGroupId == groupId)
            {
                effect.updateCurrentChatPeerIcon = true;
                effect.currentChatPeerIcon = normalizedGroupIcon(payload.value(QStringLiteral("icon")).toString());
            }
            setSuccessStatus(effect, QStringLiteral("群头像已更新"));
            requestGroupRefresh(effect);
            return effect;
        }
        case ID_MUTE_GROUP_MEMBER_RSP:
            setSuccessStatus(effect, QStringLiteral("禁言设置已生效"));
            return effect;
        case ID_SET_GROUP_ADMIN_RSP:
            setSuccessStatus(effect, QStringLiteral("管理员设置已更新"));
            requestGroupRefresh(effect);
            return effect;
        case ID_KICK_GROUP_MEMBER_RSP:
            setSuccessStatus(effect, QStringLiteral("成员已移出群聊"));
            requestGroupRefresh(effect);
            return effect;
        case ID_QUIT_GROUP_RSP:
        {
            const qint64 groupId = payloadGroupId(payload);
            setSuccessStatus(effect, QStringLiteral("已退出当前群聊"));
            requestGroupRefresh(effect);
            removeGroupAndConversation(effect, groupId);
            return effect;
        }
        case ID_DISSOLVE_GROUP_RSP:
        {
            const qint64 groupId = payloadGroupId(payload);
            setSuccessStatus(effect, QStringLiteral("群聊已解散"));
            requestGroupRefresh(effect);
            removeGroupAndConversation(effect, groupId);
            return effect;
        }
        default:
            return effect;
    }
}

GroupManagementResponseEffect
GroupManagementResponseService::reduceError(int reqId, int error, const QJsonObject& payload)
{
    GroupManagementResponseEffect effect;
    if (!isManagementResponse(reqId))
    {
        return effect;
    }

    const QString serverMessage = payload.value(QStringLiteral("message")).toString().trimmed();
    const QString action = responseActionText(reqId);
    effect.handled = true;
    effect.hasStatus = true;
    effect.statusIsError = true;
    effect.statusText =
        serverMessage.isEmpty() ? QStringLiteral("%1失败（错误码:%2）").arg(action).arg(error)
                                : QStringLiteral("%1失败：%2（错误码:%3）").arg(action, serverMessage).arg(error);
    return effect;
}

} // namespace memochat::group
