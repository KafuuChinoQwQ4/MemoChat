#include "AppControllerGroupPayloads.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace memochat::app_group_payloads
{
namespace
{

QByteArray compactJson(const QJsonObject& obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

} // namespace

QByteArray buildRefreshGroupListPayload(int fromUid)
{
    QJsonObject obj;
    obj["fromuid"] = fromUid;
    return compactJson(obj);
}

QByteArray buildRequestDialogListPayload(int fromUid)
{
    QJsonObject obj;
    obj["fromuid"] = fromUid;
    return compactJson(obj);
}

QByteArray
buildInviteGroupMemberPayload(int fromUid, const QString& targetUserId, qint64 groupId, const QString& reason)
{
    QJsonObject obj;
    obj["fromuid"] = fromUid;
    obj["target_user_id"] = targetUserId;
    obj["groupid"] = static_cast<qint64>(groupId);
    obj["reason"] = reason;
    return compactJson(obj);
}

QByteArray buildApplyJoinGroupPayload(int fromUid, const QString& groupCode, const QString& reason)
{
    QJsonObject obj;
    obj["fromuid"] = fromUid;
    obj["group_code"] = groupCode;
    obj["reason"] = reason;
    return compactJson(obj);
}

QByteArray
buildEditMessagePayload(int fromUid, const QString& msgId, const QString& content, qint64 groupId, int peerUid)
{
    QJsonObject obj;
    obj["fromuid"] = fromUid;
    obj["msgid"] = msgId;
    obj["content"] = content.left(4096);
    if (groupId > 0)
    {
        obj["groupid"] = static_cast<qint64>(groupId);
    }
    else
    {
        obj["peer_uid"] = peerUid;
    }
    return compactJson(obj);
}

QByteArray buildRevokeMessagePayload(int fromUid, const QString& msgId, qint64 groupId, int peerUid)
{
    QJsonObject obj;
    obj["fromuid"] = fromUid;
    obj["msgid"] = msgId;
    if (groupId > 0)
    {
        obj["groupid"] = static_cast<qint64>(groupId);
    }
    else
    {
        obj["peer_uid"] = peerUid;
    }
    return compactJson(obj);
}

QByteArray buildForwardMessagePayload(int fromUid, const QString& msgId, qint64 groupId, int peerUid)
{
    QJsonObject obj;
    obj["fromuid"] = fromUid;
    obj["msgid"] = msgId;
    obj["client_msg_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (groupId > 0)
    {
        obj["groupid"] = static_cast<qint64>(groupId);
    }
    else
    {
        obj["peer_uid"] = peerUid;
    }
    return compactJson(obj);
}

QByteArray buildGroupHistoryPayload(int fromUid, qint64 groupId, qint64 beforeSeq, int limit)
{
    QJsonObject obj;
    obj["fromuid"] = fromUid;
    obj["groupid"] = static_cast<qint64>(groupId);
    obj["before_ts"] = 0;
    obj["before_seq"] = static_cast<qint64>(beforeSeq);
    obj["limit"] = limit;
    return compactJson(obj);
}

QString groupActionText(ReqId id)
{
    switch (id)
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
        case ID_SYNC_DRAFT_RSP:
            return "同步草稿";
        case ID_PIN_DIALOG_RSP:
            return "置顶会话";
        case ID_EDIT_PRIVATE_MSG_RSP:
            return "编辑私聊消息";
        case ID_REVOKE_PRIVATE_MSG_RSP:
            return "撤回私聊消息";
        case ID_FORWARD_PRIVATE_MSG_RSP:
            return "转发私聊消息";
        default:
            return "群操作";
    }
}

} // namespace memochat::app_group_payloads
