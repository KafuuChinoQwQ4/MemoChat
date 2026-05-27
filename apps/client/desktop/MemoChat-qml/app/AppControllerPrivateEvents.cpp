#include "AppController.h"
#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "MessagePayloadService.h"
#include "MessageContentCodec.h"
#include "usermgr.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <algorithm>

namespace {
std::shared_ptr<AuthInfo> buildDialogPlaceholder(const FriendListModel &dialogListModel, int uid)
{
    const int dialogIndex = dialogListModel.indexOfUid(uid);
    const QVariantMap dialogItem = dialogIndex >= 0 ? dialogListModel.get(dialogIndex) : QVariantMap();
    return DialogListService::buildPlaceholderAuthInfo(uid,
                                                       dialogItem,
                                                       QStringLiteral("qrc:/res/head_1.png"));
}
}

void AppController::onTextChatMsg(std::shared_ptr<TextChatMsg> msg)
{
    if (!msg || msg->_chat_msgs.empty()) {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const bool fromSelf = (msg->_from_uid == selfUid);
    const int peerUid = (msg->_from_uid == selfUid) ? msg->_to_uid : msg->_from_uid;
    qInfo() << "Private chat message received, peer uid:" << peerUid
            << "from self:" << fromSelf
            << "current chat uid:" << _current_chat_uid
            << "current group id:" << _current_group_id
            << "batch size:" << static_cast<int>(msg->_chat_msgs.size());
    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo && peerUid > 0) {
        auto placeholder = buildDialogPlaceholder(_dialog_list_model, peerUid);
        _gateway.userMgr()->AddFriend(placeholder);
        _chat_list_model.upsertFriend(placeholder);
        _contact_list_model.upsertFriend(placeholder);
        _dialog_list_model.upsertFriend(placeholder);
        friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    }
    qint64 latestPeerTs = 0;
    for (const auto &one : msg->_chat_msgs) {
        if (one && (one->_deleted_at_ms > 0 || one->_msg_content == QStringLiteral("[消息已撤回]"))) {
            one->_msg_state = QStringLiteral("deleted");
        } else if (one && one->_edited_at_ms > 0) {
            one->_msg_state = QStringLiteral("edited");
        }
        if (one && one->_from_uid == peerUid) {
            latestPeerTs = qMax(latestPeerTs, one->_created_at);
        }
    }
    _gateway.userMgr()->AppendFriendChatMsg(peerUid, msg->_chat_msgs);
    _private_cache_store.upsertMessages(selfUid, peerUid, msg->_chat_msgs);
    friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    QString preview;
    if (friendInfo && !friendInfo->_chat_msgs.empty()) {
        preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
    } else {
        preview = MessageContentCodec::toPreviewText(msg->_chat_msgs.back()->_msg_content);
    }
    qint64 lastTs = QDateTime::currentMSecsSinceEpoch();
    if (!msg->_chat_msgs.empty() && msg->_chat_msgs.back()) {
        lastTs = msg->_chat_msgs.back()->_created_at;
    }
    ConversationSyncService::updatePrivatePreview(_chat_list_model, _dialog_list_model, peerUid, preview, lastTs);
    const bool isViewingCurrentPrivate = (_current_group_id <= 0 && peerUid == _current_chat_uid);
    if (isViewingCurrentPrivate) {
        ConversationSyncService::clearPrivateUnread(_chat_list_model, _dialog_list_model, peerUid);
        if (latestPeerTs > 0) {
            sendPrivateReadAck(peerUid, latestPeerTs);
        }
    } else if (!fromSelf) {
        ConversationSyncService::incrementPrivateUnread(_dialog_list_model, peerUid);
    }

    if (peerUid != _current_chat_uid) {
        qInfo() << "Private chat message stored for background dialog, peer uid:" << peerUid;
        return;
    }

    for (const auto &chat : msg->_chat_msgs) {
        _message_model.appendMessage(chat, selfUid);
    }
    ConversationSyncService::syncHistoryCursor(_message_model, _private_history_before_ts, _private_history_before_msg_id);
    qInfo() << "Private chat view refreshed from live message, peer uid:" << peerUid
            << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
            << "message model count:" << _message_model.rowCount();
}

void AppController::onMessageStatus(QJsonObject payload)
{
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    const QString status = payload.value("status").toString();
    const QString scope = payload.value("scope").toString();
    const QString clientMsgId = payload.value("client_msg_id").toString();

    if (clientMsgId.isEmpty()) {
        return;
    }

    const QString nextState = [error, status]() -> QString {
        if (error != ErrorCodes::SUCCESS) {
            return QStringLiteral("failed");
        }
        if (status == QStringLiteral("accepted")) {
            return QStringLiteral("accepted");
        }
        if (status == QStringLiteral("queued_retry")) {
            return QStringLiteral("queued_retry");
        }
        if (status == QStringLiteral("offline_pending")) {
            return QStringLiteral("offline_pending");
        }
        if (status == QStringLiteral("persisted") || status == QStringLiteral("delivered")) {
            return QStringLiteral("sent");
        }
        return status.isEmpty() ? QStringLiteral("sent") : status;
    }();

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const int selfUid = selfInfo ? selfInfo->_uid : _gateway.userMgr()->GetUid();

    if (scope == QStringLiteral("group") || payload.value("groupid").toVariant().toLongLong() > 0) {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        if (groupId <= 0) {
            return;
        }

        const QJsonObject msgObj = payload.value("msg").toObject();
        if (error == ErrorCodes::SUCCESS && status == QStringLiteral("persisted") && !msgObj.isEmpty()) {
            const QString senderName = payload.value("from_nick").toString(
                payload.value("from_name").toString(selfInfo ? selfInfo->_nick : QString()));
            const QString senderIcon = normalizeIconPath(payload.value("from_icon").toString(
                selfInfo ? selfInfo->_icon : QString()));
            auto correctedMsg = MessagePayloadService::buildGroupAckMessage(payload, msgObj, selfUid, senderName, senderIcon);
            if (correctedMsg) {
                _gateway.userMgr()->UpsertGroupChatMsg(groupId, correctedMsg);
                if (selfInfo && _group_cache_store.isReady()) {
                    _group_cache_store.upsertMessages(selfInfo->_uid, groupId, {correctedMsg});
                }
                if (groupId == _current_group_id) {
                    _message_model.upsertMessage(correctedMsg, selfUid);
                }
                _pending_group_msg_group_map.remove(clientMsgId);
                return;
            }
        }

        if (_gateway.userMgr()->UpdateGroupChatMsgState(groupId, clientMsgId, nextState) && groupId == _current_group_id) {
            _message_model.updateMessageState(clientMsgId, nextState);
        }
        if (selfInfo) {
            auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
            if (groupInfo && _group_cache_store.isReady()) {
                _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
            }
        }
        if (nextState == QStringLiteral("failed")) {
            _pending_group_msg_group_map.remove(clientMsgId);
        }
        return;
    }

    const int peerUid = payload.value("peer_uid").toInt(payload.value("touid").toInt());
    if (peerUid <= 0) {
        return;
    }
    if (_gateway.userMgr()->UpdatePrivateChatMsgState(peerUid, clientMsgId, nextState) && peerUid == _current_chat_uid) {
        _message_model.updateMessageState(clientMsgId, nextState);
    }
    if (selfInfo) {
        auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
        if (friendInfo) {
            _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, friendInfo->_chat_msgs);
        }
    }
}

void AppController::onPrivateHistoryRsp(QJsonObject payload)
{
    const int peerUid = payload.value("peer_uid").toInt();
    const bool isPendingRequest = peerUid == _private_history_pending_peer_uid;
    if (isPendingRequest) {
        setPrivateHistoryLoading(false);
    }
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        if (isPendingRequest) {
            _private_history_pending_before_ts = 0;
            _private_history_pending_before_msg_id.clear();
            _private_history_pending_peer_uid = 0;
            setCanLoadMorePrivateHistory(false);
        }
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        if (isPendingRequest) {
            _private_history_pending_before_ts = 0;
            _private_history_pending_before_msg_id.clear();
            _private_history_pending_peer_uid = 0;
            setCanLoadMorePrivateHistory(false);
        }
        return;
    }

    const bool hasMore = payload.value("has_more").toBool(false);
    const QJsonArray messages = payload.value("messages").toArray();
    qInfo() << "Private history response, peer uid:" << peerUid
            << "current chat uid:" << _current_chat_uid
            << "pending peer uid:" << _private_history_pending_peer_uid
            << "before ts:" << _private_history_pending_before_ts
            << "before msg id:" << _private_history_pending_before_msg_id
            << "message count:" << messages.size()
            << "has more:" << hasMore;
    std::vector<std::shared_ptr<TextChatData>> parsed;
    parsed.reserve(messages.size());
    for (const auto &one : messages) {
        auto msg = MessagePayloadService::buildPrivateHistoryMessage(one.toObject());
        if (msg) {
            parsed.push_back(msg);
        }
    }
    std::sort(parsed.begin(), parsed.end(),
              [](const std::shared_ptr<TextChatData> &lhs, const std::shared_ptr<TextChatData> &rhs) {
                  if (!lhs || !rhs) {
                      return static_cast<bool>(lhs);
                  }
                  if (lhs->_created_at != rhs->_created_at) {
                      return lhs->_created_at < rhs->_created_at;
                  }
                  return lhs->_msg_id < rhs->_msg_id;
              });

    if (!parsed.empty()) {
        _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, parsed);
        _gateway.userMgr()->AppendFriendChatMsg(peerUid, parsed);
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (friendInfo && !friendInfo->_chat_msgs.empty()) {
        const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
        const qint64 lastTs = friendInfo->_chat_msgs.back() ? friendInfo->_chat_msgs.back()->_created_at : 0;
        ConversationSyncService::updatePrivatePreview(_chat_list_model, _dialog_list_model, peerUid, preview, lastTs);
    }

    if (peerUid == _current_chat_uid) {
        const bool isPaginationRequest = isPendingRequest
            && (_private_history_pending_before_ts > 0 || !_private_history_pending_before_msg_id.isEmpty());
        if (isPaginationRequest) {
            _message_model.prependMessages(parsed, selfInfo->_uid);
        } else if (friendInfo) {
            _message_model.setMessages(friendInfo->_chat_msgs, selfInfo->_uid);
        } else {
            _message_model.clear();
        }
        ConversationSyncService::syncHistoryCursor(_message_model, _private_history_before_ts, _private_history_before_msg_id);
        setCanLoadMorePrivateHistory(hasMore);
        qint64 latestPeerTs = 0;
        if (friendInfo) {
            for (const auto &one : friendInfo->_chat_msgs) {
                if (one && one->_from_uid == peerUid) {
                    latestPeerTs = qMax(latestPeerTs, one->_created_at);
                }
            }
        }
        if (latestPeerTs > 0) {
            sendPrivateReadAck(peerUid, latestPeerTs);
        }
        qInfo() << "Private chat view refreshed from history, peer uid:" << peerUid
                << "history append mode:"
                << (isPaginationRequest ? "prepend" : "reset")
                << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
                << "message model count:" << _message_model.rowCount();
    } else {
        qInfo() << "Private history response cached for non-current dialog, peer uid:" << peerUid;
    }

    if (isPendingRequest) {
        _private_history_pending_before_ts = 0;
        _private_history_pending_before_msg_id.clear();
        _private_history_pending_peer_uid = 0;
    }
}

void AppController::onPrivateMsgChanged(QJsonObject payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }
    if (payload.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const int fromUid = payload.value("fromuid").toInt();
    const int peerUidField = payload.value("peer_uid").toInt();
    const QString event = payload.value("event").toString();
    const MessageUpdateFields updateFields = MessagePayloadService::parseMessageUpdateFields(payload, event);
    const QString &msgId = updateFields.msgId;
    const QString &content = updateFields.content;
    if (msgId.isEmpty() || content.isEmpty()) {
        return;
    }

    int peerUid = 0;
    if (fromUid == selfUid) {
        peerUid = peerUidField;
    } else if (fromUid > 0) {
        peerUid = fromUid;
    } else {
        peerUid = peerUidField;
    }
    if (peerUid <= 0) {
        return;
    }

    if (!_gateway.userMgr()->UpdatePrivateChatMsgContent(peerUid,
                                                         msgId,
                                                         content,
                                                         updateFields.state,
                                                         updateFields.editedAtMs,
                                                         updateFields.deletedAtMs)) {
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo) {
        return;
    }

    if (_private_cache_store.isReady()) {
        _private_cache_store.upsertMessages(selfUid, peerUid, friendInfo->_chat_msgs);
    }
    if (!friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back()) {
        const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
        const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
        ConversationSyncService::updatePrivatePreview(_chat_list_model, _dialog_list_model, peerUid, preview, lastTs);
    }
    if (peerUid == _current_chat_uid) {
        _message_model.patchMessageContent(msgId,
                                           content,
                                           updateFields.state,
                                           updateFields.editedAtMs,
                                           updateFields.deletedAtMs);
    }
}

void AppController::onPrivateReadAck(QJsonObject payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }
    if (payload.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const int readerUid = payload.value("fromuid").toInt();
    if (readerUid <= 0 || readerUid == selfUid) {
        return;
    }

    qint64 readTs = payload.value("read_ts").toVariant().toLongLong();
    if (readTs <= 0) {
        readTs = QDateTime::currentMSecsSinceEpoch();
    } else if (readTs < 100000000000LL) {
        readTs *= 1000;
    }
    if (_gateway.userMgr()->MarkPrivateOutgoingReadUntil(readerUid, selfUid, readTs) <= 0) {
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(readerUid);
    if (!friendInfo) {
        return;
    }
    if (_private_cache_store.isReady()) {
        _private_cache_store.upsertMessages(selfUid, readerUid, friendInfo->_chat_msgs);
    }
    if (readerUid == _current_chat_uid) {
        for (const auto &one : friendInfo->_chat_msgs) {
            if (!one || one->_from_uid != selfUid || one->_created_at > readTs) {
                continue;
            }
            if (one->_msg_state != QStringLiteral("read")) {
                continue;
            }
            _message_model.updateMessageState(one->_msg_id, QStringLiteral("read"));
        }
    }
}
