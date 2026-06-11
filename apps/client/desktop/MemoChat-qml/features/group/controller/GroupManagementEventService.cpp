#include "GroupManagementEventService.h"

#include <QVariant>

namespace memochat::group
{
namespace
{
QString eventName(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("event")).toString();
}

qint64 payloadGroupId(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("groupid")).toVariant().toLongLong();
}

void markRefreshAfterMembershipChange(GroupManagementEffect& effect)
{
    effect.requestGroupList = true;
}
} // namespace

QString GroupManagementEventService::defaultGroupIcon()
{
    return QStringLiteral("qrc:/res/chat_icon.png");
}

QString GroupManagementEventService::defaultPrivatePeerIcon()
{
    return QStringLiteral("qrc:/res/head_1.png");
}

QString GroupManagementEventService::normalizedGroupIcon(const QString& icon)
{
    const QString trimmedIcon = icon.trimmed();
    return trimmedIcon.isEmpty() ? defaultGroupIcon() : icon;
}

GroupManagementEffect GroupManagementEventService::reduceGroupListUpdated(const GroupManagementEventContext& context)
{
    GroupManagementEffect effect;
    effect.handled = true;
    effect.refreshGroupModel = true;
    effect.refreshDialogModel = true;
    effect.requestDialogList = true;

    if (context.currentGroup.groupId <= 0)
    {
        return effect;
    }

    if (context.refreshedCurrentGroup.exists && context.refreshedCurrentGroup.groupId > 0)
    {
        effect.selectCurrentGroup = true;
        effect.selectedGroupId = context.refreshedCurrentGroup.groupId;
        effect.selectedGroupName = context.refreshedCurrentGroup.name;
        effect.selectedGroupCode = context.refreshedCurrentGroup.code;
        effect.updateCurrentChatPeerIcon = true;
        effect.currentChatPeerIcon = normalizedGroupIcon(context.refreshedCurrentGroup.icon);
        effect.openGroupConversation = true;
        effect.openGroupId = context.refreshedCurrentGroup.groupId;
        effect.openGroupResetHistory = false;
        effect.syncCurrentDialogDraft = true;
        return effect;
    }

    effect.clearCurrentGroup = true;
    if (context.currentChat.privatePeerUid > 0)
    {
        effect.loadCurrentChatMessages = true;
        return effect;
    }

    effect.clearMessageModel = true;
    effect.resetCurrentChatProjection = true;
    effect.updateCurrentChatPeerIcon = true;
    effect.currentChatPeerIcon = defaultPrivatePeerIcon();
    return effect;
}

GroupManagementEffect GroupManagementEventService::reduceGroupInvite(qint64 groupId,
                                                                     const QString& groupCode,
                                                                     const QString& groupName,
                                                                     int operatorUid,
                                                                     const GroupManagementEventContext& context)
{
    Q_UNUSED(operatorUid);

    GroupManagementEffect effect;
    effect.handled = true;
    effect.hasStatus = true;
    effect.statusText = QStringLiteral("收到群邀请：%1").arg(groupName);
    effect.statusIsError = false;

    if (context.selfUid > 0)
    {
        effect.requestGroupList = true;
    }
    if (context.currentGroup.groupId <= 0 && groupId > 0)
    {
        effect.selectCurrentGroup = true;
        effect.selectedGroupId = groupId;
        effect.selectedGroupName = groupName;
        effect.selectedGroupCode = groupCode;
    }
    return effect;
}

GroupManagementEffect GroupManagementEventService::reduceGroupApply(qint64 groupId,
                                                                    int applicantUid,
                                                                    const QString& applicantUserId,
                                                                    const QString& reason)
{
    Q_UNUSED(groupId);

    GroupManagementEffect effect;
    effect.handled = true;
    effect.hasStatus = true;
    const QString displayId = applicantUserId.isEmpty() ? QString::number(applicantUid) : applicantUserId;
    effect.statusText = QStringLiteral("收到入群申请：%1 %2").arg(displayId, reason);
    effect.statusIsError = false;
    return effect;
}

GroupManagementEffect GroupManagementEventService::reduceGroupMemberChanged(const QJsonObject& payload,
                                                                            const GroupManagementEventContext& context)
{
    GroupManagementEffect effect;
    const QString event = eventName(payload);
    if (event.isEmpty())
    {
        effect.handled = true;
        effect.requestGroupList = true;
        return effect;
    }

    const bool isChatConversationEvent =
        event == QStringLiteral("group_read_ack") ||
                                event == QStringLiteral("group_msg_edited") ||
                                                        event == QStringLiteral("group_msg_revoked");
    if (isChatConversationEvent)
    {
        effect.handled = false;
        effect.chatConversationEvent = true;
        return effect;
    }

    effect.handled = true;
    effect.hasStatus = true;
    effect.statusText = QStringLiteral("群事件：%1").arg(event);
    effect.statusIsError = false;

    if (event == QStringLiteral("group_dissolved"))
    {
        const qint64 groupId = payloadGroupId(payload);
        if (groupId > 0)
        {
            effect.removeGroupIds.push_back(groupId);
            effect.clearGroupConversationIds.push_back(groupId);
            effect.clearCurrentChatIfGroupId = true;
            effect.currentChatGroupIdToClear = groupId;
            effect.refreshGroupModel = true;
            effect.refreshDialogModel = true;
        }
        effect.requestGroupList = true;
        return effect;
    }

    if (event == QStringLiteral("group_icon_updated"))
    {
        const qint64 groupId = payloadGroupId(payload);
        if (groupId > 0 && groupId == context.currentGroup.groupId)
        {
            effect.updateCurrentChatPeerIcon = true;
            effect.currentChatPeerIcon = normalizedGroupIcon(payload.value(QStringLiteral("icon")).toString());
        }
        markRefreshAfterMembershipChange(effect);
        return effect;
    }

    markRefreshAfterMembershipChange(effect);
    return effect;
}

} // namespace memochat::group
