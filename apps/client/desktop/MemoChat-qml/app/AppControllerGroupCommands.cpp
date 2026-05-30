#include "AppController.h"
#include "AppCoordinators.h"
#include "AppControllerGroupPayloads.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QRegularExpression>
#include <QtGlobal>

void AppController::refreshGroupList()
{
    if (!_bootstrap_state.groupsReady)
    {
        bootstrapGroups();
        return;
    }
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || selfInfo->_uid <= 0)
    {
        setGroupStatus("用户状态异常，请重新登录", true);
        return;
    }
    const QByteArray payload = memochat::app_group_payloads::buildRefreshGroupListPayload(selfInfo->_uid);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GET_GROUP_LIST_REQ, payload);
}

void AppController::requestDialogList()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }
    const QByteArray payload = memochat::app_group_payloads::buildRequestDialogListPayload(selfInfo->_uid);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GET_DIALOG_LIST_REQ, payload);
}

void AppController::createGroup(const QString& name, const QVariantList& memberUserIdList)
{
    _group_coordinator->createGroup(name, memberUserIdList);
}

void AppController::inviteGroupMember(const QString& userId, const QString& reason)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedUserId = userId.trimmed();
    if (!selfInfo || _group_state.currentId <= 0 || !kUserIdPattern.match(trimmedUserId).hasMatch())
    {
        setGroupStatus("邀请参数非法", true);
        return;
    }
    if (selfInfo->_user_id == trimmedUserId)
    {
        setGroupStatus("不能邀请自己", true);
        return;
    }

    int targetUid = 0;
    const auto friends = _gateway.userMgr()->GetFriendListSnapshot();
    for (const auto& one : friends)
    {
        if (one && one->_user_id == trimmedUserId)
        {
            targetUid = one->_uid;
            break;
        }
    }
    if (targetUid <= 0 || !_gateway.userMgr()->CheckFriendById(targetUid))
    {
        setGroupStatus("仅支持邀请好友入群", true);
        return;
    }

    const QByteArray payload = memochat::app_group_payloads::buildInviteGroupMemberPayload(selfInfo->_uid,
                                                                                           trimmedUserId,
                                                                                           _group_state.currentId,
                                                                                           reason);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_INVITE_GROUP_MEMBER_REQ, payload);
    setGroupStatus("邀请已发送", false);
}

void AppController::applyJoinGroup(const QString& groupCode, const QString& reason)
{
    static const QRegularExpression kGroupCodePattern("^g[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedGroupCode = groupCode.trimmed();
    if (!selfInfo || !kGroupCodePattern.match(trimmedGroupCode).hasMatch())
    {
        setGroupStatus("申请参数非法", true);
        return;
    }
    const QByteArray payload =
        memochat::app_group_payloads::buildApplyJoinGroupPayload(selfInfo->_uid, trimmedGroupCode, reason);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_APPLY_JOIN_GROUP_REQ, payload);
    setGroupStatus("申请已发送", false);
}

void AppController::reviewGroupApply(qint64 applyId, bool agree)
{
    _group_coordinator->reviewGroupApply(applyId, agree);
}

void AppController::sendGroupTextMessage(const QString& text)
{
    _group_coordinator->sendGroupTextMessage(text);
}

void AppController::sendGroupImageMessage()
{
    _group_coordinator->sendGroupImageMessage();
}

void AppController::sendGroupFileMessage()
{
    _group_coordinator->sendGroupFileMessage();
}

void AppController::editGroupMessage(const QString& msgId, const QString& text)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedMsgId = msgId.trimmed();
    const QString newText = text.trimmed();
    if (!selfInfo || trimmedMsgId.isEmpty() || newText.isEmpty())
    {
        if (_group_state.currentId > 0)
        {
            setGroupStatus("编辑消息参数非法", true);
        }
        else
        {
            setTip("编辑消息参数非法", true);
        }
        return;
    }

    qint64 groupId = 0;
    int peerUid = 0;
    if (_group_state.currentId > 0)
    {
        groupId = _group_state.currentId;
    }
    else if (_chat_state.uid > 0)
    {
        peerUid = _chat_state.uid;
    }
    else
    {
        setTip("请选择会话", true);
        return;
    }
    const QByteArray payload =
        memochat::app_group_payloads::buildEditMessagePayload(selfInfo->_uid, trimmedMsgId, newText, groupId, peerUid);
    _gateway.chatTransport()->slot_send_data(_group_state.currentId > 0 ? ReqId::ID_EDIT_GROUP_MSG_REQ
                                                                        : ReqId::ID_EDIT_PRIVATE_MSG_REQ,
                                             payload);
}

void AppController::revokeGroupMessage(const QString& msgId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedMsgId = msgId.trimmed();
    if (!selfInfo || trimmedMsgId.isEmpty())
    {
        if (_group_state.currentId > 0)
        {
            setGroupStatus("撤回消息参数非法", true);
        }
        else
        {
            setTip("撤回消息参数非法", true);
        }
        return;
    }

    qint64 groupId = 0;
    int peerUid = 0;
    if (_group_state.currentId > 0)
    {
        groupId = _group_state.currentId;
    }
    else if (_chat_state.uid > 0)
    {
        peerUid = _chat_state.uid;
    }
    else
    {
        setTip("请选择会话", true);
        return;
    }
    const QByteArray payload =
        memochat::app_group_payloads::buildRevokeMessagePayload(selfInfo->_uid, trimmedMsgId, groupId, peerUid);
    _gateway.chatTransport()->slot_send_data(_group_state.currentId > 0 ? ReqId::ID_REVOKE_GROUP_MSG_REQ
                                                                        : ReqId::ID_REVOKE_PRIVATE_MSG_REQ,
                                             payload);
}

void AppController::forwardGroupMessage(const QString& msgId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedMsgId = msgId.trimmed();
    if (!selfInfo || trimmedMsgId.isEmpty())
    {
        if (_group_state.currentId > 0)
        {
            setGroupStatus("转发消息参数非法", true);
        }
        else
        {
            setTip("转发消息参数非法", true);
        }
        return;
    }

    if (_group_state.currentId <= 0 && _chat_state.uid > 0)
    {
        if (!_message_model.containsMessage(trimmedMsgId))
        {
            setTip("未找到要转发的消息", true);
            return;
        }
        const QByteArray payload =
            memochat::app_group_payloads::buildForwardMessagePayload(selfInfo->_uid, trimmedMsgId, 0, _chat_state.uid);
        _gateway.chatTransport()->slot_send_data(ReqId::ID_FORWARD_PRIVATE_MSG_REQ, payload);
        return;
    }
    if (_group_state.currentId <= 0)
    {
        setTip("请选择会话", true);
        return;
    }

    const QByteArray payload = memochat::app_group_payloads::buildForwardMessagePayload(selfInfo->_uid,
                                                                                        trimmedMsgId,
                                                                                        _group_state.currentId,
                                                                                        0);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_FORWARD_GROUP_MSG_REQ, payload);
}

void AppController::loadGroupHistory()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _group_state.currentId <= 0)
    {
        return;
    }
    if (_loading_state.groupHistoryLoading)
    {
        qInfo() << "Skip group history request because one is already loading, group id:" << _group_state.currentId;
        return;
    }
    if (!_group_state.historyHasMore && _group_state.historyBeforeSeq > 0)
    {
        return;
    }
    const QByteArray payload = memochat::app_group_payloads::buildGroupHistoryPayload(selfInfo->_uid,
                                                                                      _group_state.currentId,
                                                                                      _group_state.historyBeforeSeq,
                                                                                      50);
    _loading_state.groupHistoryLoading = true;
    qInfo() << "Requesting group history, group id:" << _group_state.currentId
            << "before seq:" << _group_state.historyBeforeSeq
            << "private history loading:" << _loading_state.privateHistoryLoading;
    setPrivateHistoryLoading(true);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_HISTORY_REQ, payload);
}

void AppController::requestGroupHistoryForBootstrap(qint64 groupId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || groupId <= 0)
    {
        return;
    }
    if (groupId == _group_state.currentId && _loading_state.groupHistoryLoading)
    {
        qInfo() << "Skip bootstrap group history because current group is already loading, group id:" << groupId;
        return;
    }

    const QByteArray payload = memochat::app_group_payloads::buildGroupHistoryPayload(selfInfo->_uid, groupId, 0, 50);
    if (groupId == _group_state.currentId)
    {
        _loading_state.groupHistoryLoading = true;
        setPrivateHistoryLoading(true);
    }
    qInfo() << "Requesting bootstrap group history, group id:" << groupId
            << "current group id:" << _group_state.currentId
            << "private history loading:" << _loading_state.privateHistoryLoading;
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_HISTORY_REQ, payload);
}
