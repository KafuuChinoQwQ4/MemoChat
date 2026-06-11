#include "ChatProtocolRouter.h"

#include <QString>

namespace memochat::chat
{
GroupResponseRoute ChatProtocolRouter::routeGroupResponse(ReqId reqId)
{
    switch (reqId)
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
            return GroupResponseRoute::Management;
        case ID_GROUP_HISTORY_RSP:
            return GroupResponseRoute::History;
        case ID_EDIT_GROUP_MSG_RSP:
        case ID_REVOKE_GROUP_MSG_RSP:
            return GroupResponseRoute::GroupMessageMutation;
        case ID_EDIT_PRIVATE_MSG_RSP:
        case ID_REVOKE_PRIVATE_MSG_RSP:
            return GroupResponseRoute::PrivateMessageMutation;
        case ID_FORWARD_PRIVATE_MSG_RSP:
            return GroupResponseRoute::PrivateForward;
        case ID_GROUP_CHAT_MSG_RSP:
        case ID_FORWARD_GROUP_MSG_RSP:
            return GroupResponseRoute::GroupMessageAck;
        case ID_SYNC_DRAFT_RSP:
        case ID_PIN_DIALOG_RSP:
            return GroupResponseRoute::DialogMeta;
        default:
            return GroupResponseRoute::Ignore;
    }
}

MessageStatusRoute ChatProtocolRouter::routeMessageStatus(const QJsonObject& payload)
{
    const QString clientMsgId = payload.value(QStringLiteral("client_msg_id")).toString();
    if (clientMsgId.isEmpty())
    {
        return MessageStatusRoute::Ignore;
    }

    const QString scope = payload.value(QStringLiteral("scope")).toString();
    if (scope == QStringLiteral("group") || payload.value(QStringLiteral("groupid")).toVariant().toLongLong() > 0)
    {
        return MessageStatusRoute::GroupMessageStatus;
    }
    return MessageStatusRoute::PrivateMessageStatus;
}

GroupMemberEventRoute ChatProtocolRouter::routeGroupMemberEvent(const QJsonObject& payload)
{
    const QString event = payload.value(QStringLiteral("event")).toString();
    if (event == QStringLiteral("group_read_ack"))
    {
        return GroupMemberEventRoute::GroupReadAck;
    }
    if (event == QStringLiteral("group_msg_edited") || event == QStringLiteral("group_msg_revoked"))
    {
        return GroupMemberEventRoute::GroupMessageMutation;
    }
    return GroupMemberEventRoute::GroupManagement;
}
} // namespace memochat::chat
