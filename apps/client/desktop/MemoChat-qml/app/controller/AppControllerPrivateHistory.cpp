#include "AppController.h"
#include "DialogListService.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

void AppController::loadCurrentChatMessages()
{
    if (_chat_state.uid <= 0)
    {
        _message_model.clear();
        _private_history_state.beforeTs = 0;
        _private_history_state.beforeMsgId.clear();
        _private_history_state.pendingBeforeTs = 0;
        _private_history_state.pendingBeforeMsgId.clear();
        _private_history_state.pendingPeerUid = 0;
        _group_state.historyBeforeSeq = 0;
        _group_state.historyHasMore = true;
        _loading_state.groupHistoryLoading = false;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(_chat_state.uid);
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!friendInfo || !selfInfo)
    {
        _message_model.clear();
        _private_history_state.beforeTs = 0;
        _private_history_state.beforeMsgId.clear();
        _private_history_state.pendingBeforeTs = 0;
        _private_history_state.pendingBeforeMsgId.clear();
        _private_history_state.pendingPeerUid = 0;
        _group_state.historyBeforeSeq = 0;
        _group_state.historyHasMore = true;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        return;
    }

    qInfo() << "Loading current private chat, peer uid:" << _chat_state.uid
            << "existing friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size());
    setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
    setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/head_1.png")
                                                                 : friendInfo->_icon);

    const auto localMessages = _private_cache_store.loadRecentMessages(selfInfo->_uid, _chat_state.uid, 20);
    if (!localMessages.empty())
    {
        _gateway.userMgr()->AppendFriendChatMsg(_chat_state.uid, localMessages);
        qInfo() << "Merged local private cache, peer uid:" << _chat_state.uid
                << "cache count:" << static_cast<qlonglong>(localMessages.size());
    }
    friendInfo = _gateway.userMgr()->GetFriendById(_chat_state.uid);
    if (!friendInfo)
    {
        _message_model.clear();
        setCanLoadMorePrivateHistory(false);
        return;
    }
    _message_model.setMessages(friendInfo->_chat_msgs, selfInfo->_uid);
    qint64 latestPeerTs = 0;
    for (const auto& one : friendInfo->_chat_msgs)
    {
        if (one && one->_from_uid == _chat_state.uid)
        {
            latestPeerTs = qMax(latestPeerTs, one->_created_at);
        }
    }
    if (latestPeerTs > 0)
    {
        sendPrivateReadAck(_chat_state.uid, latestPeerTs);
    }
    _private_history_state.beforeTs = _message_model.earliestCreatedAt();
    _private_history_state.beforeMsgId = _message_model.earliestMsgId();
    _private_history_state.pendingBeforeTs = 0;
    _private_history_state.pendingBeforeMsgId.clear();
    _private_history_state.pendingPeerUid = 0;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    qInfo() << "Private chat loaded, peer uid:" << _chat_state.uid
            << "friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size())
            << "message model count:" << _message_model.rowCount()
            << "earliest created at:" << _private_history_state.beforeTs
            << "earliest msg id:" << _private_history_state.beforeMsgId;
    requestPrivateHistory(_chat_state.uid, 0, QString());
}

void AppController::requestPrivateHistory(int peerUid, qint64 beforeTs, const QString& beforeMsgId)
{
    if (_loading_state.privateHistoryLoading || peerUid <= 0)
    {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }

    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["peer_uid"] = peerUid;
    payloadObj["before_ts"] = static_cast<qint64>(beforeTs);
    if (!beforeMsgId.trimmed().isEmpty())
    {
        payloadObj["before_msg_id"] = beforeMsgId.trimmed();
    }
    payloadObj["limit"] = 20;
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);

    _private_history_state.pendingPeerUid = peerUid;
    _private_history_state.pendingBeforeTs = beforeTs;
    _private_history_state.pendingBeforeMsgId = beforeMsgId.trimmed();
    setPrivateHistoryLoading(true);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PRIVATE_HISTORY_REQ, payload);
}

void AppController::requestPrivateHistoryForBootstrap(int peerUid)
{
    if (peerUid <= 0)
    {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }

    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["peer_uid"] = peerUid;
    payloadObj["before_ts"] = static_cast<qint64>(0);
    payloadObj["limit"] = 20;
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PRIVATE_HISTORY_REQ, payload);
}
