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
                                                       QStringLiteral("qrc:/res/head_1.jpg"));
}
}

void AppController::onAddAuthFriend(std::shared_ptr<AuthInfo> authInfo)
{
    if (authInfo) {
        _gateway.userMgr()->AddFriend(authInfo);
    }
    _chat_list_model.upsertFriend(authInfo);
    _contact_list_model.upsertFriend(authInfo);
    refreshDialogModel();
    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    if (authInfo) {
        _gateway.userMgr()->MarkApplyStatus(authInfo->_uid, 1);
        _apply_request_model.markApproved(authInfo->_uid);
        if (_current_contact_uid == authInfo->_uid) {
            setCurrentContact(authInfo->_uid, authInfo->_name, authInfo->_nick, authInfo->_icon,
                              authInfo->_nick, authInfo->_sex, authInfo->_user_id);
        }
    }
}

void AppController::onAuthRsp(std::shared_ptr<AuthRsp> authRsp)
{
    if (authRsp) {
        _gateway.userMgr()->AddFriend(authRsp);
    }
    _chat_list_model.upsertFriend(authRsp);
    _contact_list_model.upsertFriend(authRsp);
    refreshDialogModel();
    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    if (authRsp) {
        _gateway.userMgr()->MarkApplyStatus(authRsp->_uid, 1);
        _apply_request_model.markApproved(authRsp->_uid);
        setAuthStatus("好友添加成功", false);
        if (_current_contact_uid == authRsp->_uid) {
            setCurrentContact(authRsp->_uid, authRsp->_name, authRsp->_nick, authRsp->_icon,
                              authRsp->_nick, authRsp->_sex, authRsp->_user_id);
        }
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

void AppController::onUserSearch(std::shared_ptr<SearchInfo> searchInfo)
{
    setSearchPending(false);

    if (!searchInfo) {
        clearSearchResultOnly();
        setSearchStatus("未找到该用户", true);
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        clearSearchResultOnly();
        setSearchStatus("用户状态异常，请重新登录", true);
        return;
    }

    if (searchInfo->_uid == selfInfo->_uid) {
        clearSearchResultOnly();
        setSearchStatus("不能搜索自己", true);
        return;
    }

    if (_gateway.userMgr()->CheckFriendById(searchInfo->_uid)) {
        clearSearchResultOnly();
        setSearchStatus("已是好友，已切换到会话", false);
        selectChatByUid(searchInfo->_uid);
        switchChatTab(ChatTabPage);
        return;
    }

    _search_result_model.setResult(searchInfo);
    setSearchStatus("已找到用户", false);
}

void AppController::onFriendApply(std::shared_ptr<AddFriendApply> applyInfo)
{
    if (!applyInfo) {
        return;
    }

    if (_gateway.userMgr()->AlreadyApply(applyInfo->_from_uid)) {
        return;
    }

    auto apply = std::make_shared<ApplyInfo>(applyInfo);
    _gateway.userMgr()->AddApplyList(apply);
    _apply_request_model.upsertApply(apply);
    setAuthStatus(QString("收到来自 %1 的好友申请").arg(applyInfo->_name), false);
}

void AppController::onDialogListRsp(QJsonObject payload)
{
    const bool bootstrappingDialog = _dialog_bootstrap_loading;
    _dialog_bootstrap_loading = false;
    setDialogsReady(true);

    if (!payload.contains("dialogs")) {
        if (bootstrappingDialog && _chat_list_model.count() > 0
            && _current_chat_uid <= 0 && _current_group_id <= 0) {
            selectChatIndex(0);
        }
        return;
    }
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        if (bootstrappingDialog && _chat_list_model.count() > 0
            && _current_chat_uid <= 0 && _current_group_id <= 0) {
            selectChatIndex(0);
        }
        return;
    }

    const QJsonArray dialogs = payload.value("dialogs").toArray();
    std::vector<std::shared_ptr<FriendInfo>> merged;
    merged.reserve(dialogs.size());
    _dialog_server_pinned_map.clear();
    _dialog_server_mute_map.clear();
    const DialogDecorationState decorationState {
        &_dialog_draft_map,
        &_dialog_mention_map,
        nullptr,
        nullptr,
        &_dialog_local_pinned_set
    };

    for (const auto &one : dialogs) {
        const QJsonObject obj = one.toObject();
        const QString dialogType = obj.value("dialog_type").toString();
        const QString title = obj.value("title").toString();
        const QString avatar = obj.value("avatar").toString();
        const QString preview = obj.value("last_msg_preview").toString();

        if (dialogType == "private") {
            const int peerUid = obj.value("peer_uid").toInt();
            if (peerUid <= 0) {
                continue;
            }
            DialogEntrySeed seed;
            seed.dialogUid = peerUid;
            seed.dialogType = dialogType;
            seed.name = title;
            seed.nick = title;
            seed.icon = avatar;
            seed.previewText = preview;
            seed.unreadCount = obj.value("unread_count").toInt(0);
            seed.pinnedRank = obj.value("pinned_rank").toInt(0);
            seed.draftText = obj.value("draft_text").toString();
            qint64 lastMsgTs = obj.value("last_msg_ts").toVariant().toLongLong();
            if (lastMsgTs > 0 && lastMsgTs < 100000000000LL) {
                lastMsgTs *= 1000;
            }
            seed.lastMsgTs = lastMsgTs;
            seed.muteState = obj.value("mute_state").toInt(0);
            _dialog_server_pinned_map.insert(peerUid, seed.pinnedRank);
            _dialog_server_mute_map.insert(peerUid, seed.muteState);
            auto item = DialogListService::buildDialogEntry(seed, decorationState);
            merged.push_back(item);
            _chat_list_model.upsertFriend(item);
            _chat_list_initialized = true;
            continue;
        }

        if (dialogType == "group") {
            const qint64 groupId = obj.value("group_id").toVariant().toLongLong();
            if (groupId <= 0) {
                continue;
            }
            const int pseudoUid = -static_cast<int>(groupId);
            const QString groupIcon = avatar.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : avatar;
            DialogEntrySeed seed;
            seed.dialogUid = pseudoUid;
            seed.dialogType = dialogType;
            seed.name = title;
            seed.nick = title;
            seed.icon = groupIcon;
            seed.previewText = preview;
            seed.unreadCount = obj.value("unread_count").toInt(0);
            seed.pinnedRank = obj.value("pinned_rank").toInt(0);
            seed.draftText = obj.value("draft_text").toString();
            qint64 lastMsgTs = obj.value("last_msg_ts").toVariant().toLongLong();
            if (lastMsgTs > 0 && lastMsgTs < 100000000000LL) {
                lastMsgTs *= 1000;
            }
            seed.lastMsgTs = lastMsgTs;
            seed.muteState = obj.value("mute_state").toInt(0);
            _dialog_server_pinned_map.insert(pseudoUid, seed.pinnedRank);
            _dialog_server_mute_map.insert(pseudoUid, seed.muteState);
            auto item = DialogListService::buildDialogEntry(seed, decorationState);
            merged.push_back(item);
            _group_uid_map.insert(pseudoUid, groupId);
        }
    }

    if (merged.empty()
        && (!_gateway.userMgr()->GetFriendListSnapshot().empty()
            || !_gateway.userMgr()->GetGroupListSnapshot().empty())) {
        refreshDialogModel();
        if (bootstrappingDialog && _current_chat_uid <= 0 && _current_group_id <= 0
            && _dialog_list_model.count() > 0) {
            const QVariantMap firstDialog = _dialog_list_model.get(0);
            const int firstUid = firstDialog.value("uid").toInt();
            if (firstUid > 0) {
                selectChatByUid(firstUid);
            } else if (firstUid < 0) {
                const int groupIndex = _group_list_model.indexOfUid(firstUid);
                if (groupIndex >= 0) {
                    selectGroupIndex(groupIndex);
                }
            }
        }
        return;
    }

    DialogListService::sortDialogs(merged);
    _dialog_list_model.upsertBatch(merged, true);
    if (_current_group_id > 0) {
        const int selectedGroupDialogUid = -static_cast<int>(_current_group_id);
        _dialog_list_model.clearUnread(selectedGroupDialogUid);
        _dialog_list_model.clearMention(selectedGroupDialogUid);
        _dialog_mention_map.remove(selectedGroupDialogUid);
    } else if (_current_chat_uid > 0) {
        _dialog_list_model.clearUnread(_current_chat_uid);
        _dialog_list_model.clearMention(_current_chat_uid);
        _dialog_mention_map.remove(_current_chat_uid);
    }
    syncCurrentDialogDraft();

    const bool shouldSelectDialog = bootstrappingDialog || (_current_chat_uid <= 0 && _current_group_id <= 0);
    if (shouldSelectDialog) {
        if (!merged.empty() && merged.front()) {
            const int topDialogUid = merged.front()->_uid;
            if (topDialogUid > 0) {
                selectChatByUid(topDialogUid);
                _dialog_list_model.clearUnread(topDialogUid);
                _dialog_list_model.clearMention(topDialogUid);
                _dialog_mention_map.remove(topDialogUid);
            } else if (topDialogUid < 0) {
                const int topGroupIndex = _group_list_model.indexOfUid(topDialogUid);
                if (topGroupIndex >= 0) {
                    selectGroupIndex(topGroupIndex);
                }
            }
        } else if (_chat_list_model.count() > 0) {
            selectChatIndex(0);
        }
    }

    // 登录后首次拉取对话列表时，自动拉取有未读消息的对话
    if (bootstrappingDialog) {
        for (const auto &dialog : merged) {
            if (!dialog) {
                continue;
            }
            // 私聊且有未读消息
            if (dialog->_uid > 0 && dialog->_unread_count > 0) {
                // 拉取最新消息，before_ts = 0 表示拉取最新消息
                requestPrivateHistory(dialog->_uid, 0, QString());
            }
        }
    }
}

void AppController::onPrivateHistoryRsp(QJsonObject payload)
{
    setPrivateHistoryLoading(false);
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        _private_history_pending_before_ts = 0;
        _private_history_pending_before_msg_id.clear();
        _private_history_pending_peer_uid = 0;
        setCanLoadMorePrivateHistory(false);
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        _private_history_pending_before_ts = 0;
        _private_history_pending_before_msg_id.clear();
        _private_history_pending_peer_uid = 0;
        setCanLoadMorePrivateHistory(false);
        return;
    }

    const int peerUid = payload.value("peer_uid").toInt();
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
        if (_private_history_pending_before_ts > 0 || !_private_history_pending_before_msg_id.isEmpty()) {
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
                << (_private_history_pending_before_ts > 0 || !_private_history_pending_before_msg_id.isEmpty()
                        ? "prepend" : "reset")
                << "friend msg count:" << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL)
                << "message model count:" << _message_model.rowCount();
    } else {
        qInfo() << "Private history response cached for non-current dialog, peer uid:" << peerUid;
    }

    _private_history_pending_before_ts = 0;
    _private_history_pending_before_msg_id.clear();
    _private_history_pending_peer_uid = 0;
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
