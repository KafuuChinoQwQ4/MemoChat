#include "AppController.h"
#include "AppControllerGroupPayloads.h"
#include "usermgr.h"

#include <QJsonObject>

bool AppController::handleGroupRspError(ReqId reqId, int error, const QJsonObject& payload)
{
    if (reqId == ID_GROUP_HISTORY_RSP)
    {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        if (groupId <= 0 || groupId == _group_state.currentId)
        {
            _loading_state.groupHistoryLoading = false;
            setPrivateHistoryLoading(false);
        }
    }
    if (reqId == ID_GROUP_CHAT_MSG_RSP)
    {
        QString clientMsgId = payload.value("client_msg_id").toString();
        if (clientMsgId.isEmpty())
        {
            clientMsgId = payload.value("msg").toObject().value("msgid").toString();
        }
        qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        if (groupId <= 0 && !clientMsgId.isEmpty())
        {
            groupId = _group_state.pendingMsgGroupMap.value(clientMsgId, 0);
        }

        if (groupId > 0 && !clientMsgId.isEmpty())
        {
            if (_gateway.userMgr()->UpdateGroupChatMsgState(groupId, clientMsgId, "failed") &&
                groupId == _group_state.currentId)
            {
                _message_model.updateMessageState(clientMsgId, "failed");
            }
            _group_state.pendingMsgGroupMap.remove(clientMsgId);
        }
    }
    if (reqId == ID_SYNC_DRAFT_RSP)
    {
        return true;
    }
    if (reqId == ID_EDIT_PRIVATE_MSG_RSP || reqId == ID_REVOKE_PRIVATE_MSG_RSP || reqId == ID_FORWARD_PRIVATE_MSG_RSP)
    {
        setTip(QString("%1失败（错误码:%2）").arg(memochat::app_group_payloads::groupActionText(reqId)).arg(error),
               true);
    }
    else
    {
        const QString serverMessage = payload.value("message").toString().trimmed();
        setGroupStatus(
            serverMessage.isEmpty()
                ? QString("%1失败（错误码:%2）").arg(memochat::app_group_payloads::groupActionText(reqId)).arg(error)
                : QString("%1失败：%2（错误码:%3）")
                      .arg(memochat::app_group_payloads::groupActionText(reqId), serverMessage)
                      .arg(error),
            true);
    }
    return true;
}
