#include "AppController.h"

#include "ConversationSyncService.h"
#include "MessageContentCodec.h"
#include "MessagePayloadService.h"
#include "usermgr.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>

void AppController::onPrivateHistoryRsp(QJsonObject payload)
{
    const int peerUid = payload.value("peer_uid").toInt();
    const bool isPendingRequest = peerUid == _private_history_state.pendingPeerUid;
    if (isPendingRequest)
    {
        setPrivateHistoryLoading(false);
    }
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        if (isPendingRequest)
        {
            _private_history_state.pendingBeforeTs = 0;
            _private_history_state.pendingBeforeMsgId.clear();
            _private_history_state.pendingPeerUid = 0;
            setCanLoadMorePrivateHistory(false);
        }
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        if (isPendingRequest)
        {
            _private_history_state.pendingBeforeTs = 0;
            _private_history_state.pendingBeforeMsgId.clear();
            _private_history_state.pendingPeerUid = 0;
            setCanLoadMorePrivateHistory(false);
        }
        return;
    }

    const bool hasMore = payload.value("has_more").toBool(false);
    const QJsonArray messages = payload.value("messages").toArray();
    qInfo() << "Private history response, peer uid:" << peerUid << "current chat uid:" << _chat_state.uid
            << "pending peer uid:" << _private_history_state.pendingPeerUid
            << "before ts:" << _private_history_state.pendingBeforeTs
            << "before msg id:" << _private_history_state.pendingBeforeMsgId << "message count:" << messages.size()
            << "has more:" << hasMore;
    std::vector<std::shared_ptr<TextChatData>> parsed;
    parsed.reserve(messages.size());
    for (const auto& one : messages)
    {
        auto msg = MessagePayloadService::buildPrivateHistoryMessage(one.toObject());
        if (msg)
        {
            parsed.push_back(msg);
        }
    }
    std::sort(parsed.begin(),
              parsed.end(),
              [](const std::shared_ptr<TextChatData>& lhs, const std::shared_ptr<TextChatData>& rhs)
              {
                  if (!lhs || !rhs)
                  {
                      return static_cast<bool>(lhs);
                  }
                  if (lhs->_created_at != rhs->_created_at)
                  {
                      return lhs->_created_at < rhs->_created_at;
                  }
                  return lhs->_msg_id < rhs->_msg_id;
              });

    if (!parsed.empty())
    {
        _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, parsed);
        _gateway.userMgr()->AppendFriendChatMsg(peerUid, parsed);
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (friendInfo && !friendInfo->_chat_msgs.empty())
    {
        const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
        const qint64 lastTs = friendInfo->_chat_msgs.back() ? friendInfo->_chat_msgs.back()->_created_at : 0;
        ConversationSyncService::updatePrivatePreview(_chat_list_model, _dialog_list_model, peerUid, preview, lastTs);
    }

    if (peerUid == _chat_state.uid)
    {
        const bool isPaginationRequest = isPendingRequest && (_private_history_state.pendingBeforeTs > 0 ||
                                                              !_private_history_state.pendingBeforeMsgId.isEmpty());
        if (isPaginationRequest)
        {
            _message_model.prependMessages(parsed, selfInfo->_uid);
        }
        else if (friendInfo)
        {
            _message_model.setMessages(friendInfo->_chat_msgs, selfInfo->_uid);
        }
        else
        {
            _message_model.clear();
        }
        ConversationSyncService::syncHistoryCursor(_message_model,
                                                   _private_history_state.beforeTs,
                                                   _private_history_state.beforeMsgId);
        setCanLoadMorePrivateHistory(hasMore);
        qint64 latestPeerTs = 0;
        if (friendInfo)
        {
            for (const auto& one : friendInfo->_chat_msgs)
            {
                if (one && one->_from_uid == peerUid)
                {
                    latestPeerTs = qMax(latestPeerTs, one->_created_at);
                }
            }
        }
        if (latestPeerTs > 0)
        {
            sendPrivateReadAck(peerUid, latestPeerTs);
        }
        qInfo() << "Private chat view refreshed from history, peer uid:" << peerUid
                << "history append mode:" << (isPaginationRequest ? "prepend" : "reset")
                << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
                << "message model count:" << _message_model.rowCount();
    }
    else
    {
        qInfo() << "Private history response cached for non-current dialog, peer uid:" << peerUid;
    }

    if (isPendingRequest)
    {
        _private_history_state.pendingBeforeTs = 0;
        _private_history_state.pendingBeforeMsgId.clear();
        _private_history_state.pendingPeerUid = 0;
    }
}
