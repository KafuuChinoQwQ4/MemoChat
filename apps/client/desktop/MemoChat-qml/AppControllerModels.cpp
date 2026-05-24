#include "AppController.h"
#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

void AppController::refreshFriendModels()
{
    ensureChatListInitialized();
    bootstrapContacts();
    bootstrapGroups();
    refreshDialogModel();
}

void AppController::refreshApplyModel()
{
    const auto applyList = _gateway.userMgr()->GetApplyListSnapshot();
    _apply_request_model.setApplies(applyList);
}

void AppController::bootstrapDialogs()
{
    if (_dialogs_ready) {
        return;
    }
    _dialog_bootstrap_loading = true;
    setDialogsReady(false);
    requestDialogList();
}

void AppController::bootstrapContacts()
{
    if (_contacts_ready) {
        return;
    }

    ensureChatListInitialized();
    _contact_list_model.clear();
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    _contact_list_model.setFriends(contactList);
    _gateway.userMgr()->UpdateContactLoadedCount();
    refreshContactLoadMoreState();
    setContactsReady(true);
}

void AppController::bootstrapGroups()
{
    if (_groups_ready) {
        return;
    }

    refreshGroupModel();
    setGroupsReady(true);
    refreshGroupList();
}

void AppController::bootstrapApplies()
{
    if (_apply_ready) {
        return;
    }

    refreshApplyModel();
    setApplyReady(true);
}

void AppController::refreshGroupModel()
{
    _group_list_model.clear();
    _group_uid_map.clear();

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    std::vector<std::shared_ptr<FriendInfo>> converted;
    converted.reserve(groups.size());
    for (const auto &group : groups) {
        if (!group || group->_group_id <= 0) {
            continue;
        }
        const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, group->_group_id);
        if (pseudoUid == 0) {
            continue;
        }
        const QString groupIcon = group->_icon.trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : group->_icon;
        auto info = std::make_shared<FriendInfo>(pseudoUid,
                                                 group->_name,
                                                 group->_name,
                                                 groupIcon,
                                                 0,
                                                 group->_announcement,
                                                 group->_announcement,
                                                 group->_last_msg);
        converted.push_back(info);
    }

    _group_list_model.setFriends(converted);
}

void AppController::refreshDialogModel()
{
    _dialog_list_model.clear();
    std::vector<std::shared_ptr<FriendInfo>> dialogs;
    const DialogDecorationState decorationState {
        &_dialog_draft_map,
        &_dialog_mention_map,
        &_dialog_server_pinned_map,
        &_dialog_server_mute_map,
        &_dialog_local_pinned_set
    };

    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    dialogs.reserve(chats.size() + _group_uid_map.size());
    for (const auto &chat : chats) {
        if (!chat) {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = chat->_uid;
        seed.dialogType = QStringLiteral("private");
        seed.userId = chat->_user_id;
        seed.name = chat->_name;
        seed.nick = chat->_nick;
        seed.icon = chat->_icon;
        seed.desc = chat->_desc;
        seed.back = chat->_back;
        seed.previewText = chat->_last_msg;
        seed.sex = chat->_sex;
        if (!chat->_chat_msgs.empty() && chat->_chat_msgs.back()) {
            seed.lastMsgTs = chat->_chat_msgs.back()->_created_at;
        }
        dialogs.push_back(DialogListService::buildDialogEntry(seed, decorationState));
    }

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    for (const auto &group : groups) {
        if (!group || group->_group_id <= 0) {
            continue;
        }

        const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, group->_group_id);
        if (pseudoUid == 0) {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = pseudoUid;
        seed.dialogType = QStringLiteral("group");
        seed.name = group->_name;
        seed.nick = group->_name;
        seed.icon = group->_icon.trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : group->_icon;
        seed.desc = group->_announcement;
        seed.back = group->_announcement;
        seed.previewText = group->_last_msg;
        if (!group->_chat_msgs.empty() && group->_chat_msgs.back()) {
            seed.lastMsgTs = group->_chat_msgs.back()->_created_at;
        }
        dialogs.push_back(DialogListService::buildDialogEntry(seed, decorationState));
    }

    DialogListService::sortDialogs(dialogs);
    _dialog_list_model.upsertBatch(dialogs, true);
    syncCurrentDialogDraft();
}

void AppController::refreshDialogModelIncremental()
{
    const qint64 startTs = QDateTime::currentMSecsSinceEpoch();

    std::vector<std::shared_ptr<FriendInfo>> updates;
    const DialogDecorationState decorationState {
        &_dialog_draft_map,
        &_dialog_mention_map,
        &_dialog_server_pinned_map,
        &_dialog_server_mute_map,
        &_dialog_local_pinned_set
    };

    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    updates.reserve(chats.size() + 1);
    for (const auto &chat : chats) {
        if (!chat) {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = chat->_uid;
        seed.dialogType = QStringLiteral("private");
        seed.userId = chat->_user_id;
        seed.name = chat->_name;
        seed.nick = chat->_nick;
        seed.icon = chat->_icon;
        seed.desc = chat->_desc;
        seed.back = chat->_back;
        seed.previewText = chat->_last_msg;
        seed.sex = chat->_sex;
        if (!chat->_chat_msgs.empty() && chat->_chat_msgs.back()) {
            seed.lastMsgTs = chat->_chat_msgs.back()->_created_at;
        }
        auto item = DialogListService::buildDialogEntry(seed, decorationState);
        updates.push_back(item);
        _chat_list_model.upsertFriend(item);
    }

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    for (const auto &group : groups) {
        if (!group || group->_group_id <= 0) {
            continue;
        }

        const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, group->_group_id);
        if (pseudoUid == 0) {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = pseudoUid;
        seed.dialogType = QStringLiteral("group");
        seed.name = group->_name;
        seed.nick = group->_name;
        seed.icon = group->_icon.trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : group->_icon;
        seed.desc = group->_announcement;
        seed.back = group->_announcement;
        seed.previewText = group->_last_msg;
        if (!group->_chat_msgs.empty() && group->_chat_msgs.back()) {
            seed.lastMsgTs = group->_chat_msgs.back()->_created_at;
        }
        updates.push_back(DialogListService::buildDialogEntry(seed, decorationState));
    }

    _dialog_list_model.upsertBatch(updates);

    syncCurrentDialogDraft();

    const qint64 endTs = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "[PERF] refreshDialogModelIncremental - chats:" << chats.size()
            << "| groups:" << groups.size()
            << "| total:" << (endTs - startTs) << "ms";
}

void AppController::loadCurrentChatMessages()
{
    if (_current_chat_uid <= 0) {
        _message_model.clear();
        _private_history_before_ts = 0;
        _private_history_before_msg_id.clear();
        _private_history_pending_before_ts = 0;
        _private_history_pending_before_msg_id.clear();
        _private_history_pending_peer_uid = 0;
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        _group_history_loading = false;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!friendInfo || !selfInfo) {
        _message_model.clear();
        _private_history_before_ts = 0;
        _private_history_before_msg_id.clear();
        _private_history_pending_before_ts = 0;
        _private_history_pending_before_msg_id.clear();
        _private_history_pending_peer_uid = 0;
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        return;
    }

    qInfo() << "Loading current private chat, peer uid:" << _current_chat_uid
            << "existing friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size());
    setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
    setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty()
                           ? QStringLiteral("qrc:/res/head_1.jpg")
                           : friendInfo->_icon);

    const auto localMessages = _private_cache_store.loadRecentMessages(selfInfo->_uid, _current_chat_uid, 20);
    if (!localMessages.empty()) {
        _gateway.userMgr()->AppendFriendChatMsg(_current_chat_uid, localMessages);
        qInfo() << "Merged local private cache, peer uid:" << _current_chat_uid
                << "cache count:" << static_cast<qlonglong>(localMessages.size());
    }
    friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    if (!friendInfo) {
        _message_model.clear();
        setCanLoadMorePrivateHistory(false);
        return;
    }
    _message_model.setMessages(friendInfo->_chat_msgs, selfInfo->_uid);
    qint64 latestPeerTs = 0;
    for (const auto &one : friendInfo->_chat_msgs) {
        if (one && one->_from_uid == _current_chat_uid) {
            latestPeerTs = qMax(latestPeerTs, one->_created_at);
        }
    }
    if (latestPeerTs > 0) {
        sendPrivateReadAck(_current_chat_uid, latestPeerTs);
    }
    _private_history_before_ts = _message_model.earliestCreatedAt();
    _private_history_before_msg_id = _message_model.earliestMsgId();
    _private_history_pending_before_ts = 0;
    _private_history_pending_before_msg_id.clear();
    _private_history_pending_peer_uid = 0;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    qInfo() << "Private chat loaded, peer uid:" << _current_chat_uid
            << "friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size())
            << "message model count:" << _message_model.rowCount()
            << "earliest created at:" << _private_history_before_ts
            << "earliest msg id:" << _private_history_before_msg_id;
    requestPrivateHistory(_current_chat_uid, 0, QString());
}

void AppController::requestPrivateHistory(int peerUid, qint64 beforeTs, const QString &beforeMsgId)
{
    if (_private_history_loading || peerUid <= 0) {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }

    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["peer_uid"] = peerUid;
    payloadObj["before_ts"] = static_cast<qint64>(beforeTs);
    if (!beforeMsgId.trimmed().isEmpty()) {
        payloadObj["before_msg_id"] = beforeMsgId.trimmed();
    }
    payloadObj["limit"] = 20;
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);

    _private_history_pending_peer_uid = peerUid;
    _private_history_pending_before_ts = beforeTs;
    _private_history_pending_before_msg_id = beforeMsgId.trimmed();
    setPrivateHistoryLoading(true);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PRIVATE_HISTORY_REQ, payload);
}

void AppController::requestPrivateHistoryForBootstrap(int peerUid)
{
    if (peerUid <= 0) {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
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

void AppController::sendGroupReadAck(qint64 groupId, qint64 readTs)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || groupId <= 0) {
        return;
    }
    if (readTs <= 0) {
        readTs = QDateTime::currentMSecsSinceEpoch();
    }
    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["groupid"] = static_cast<qint64>(groupId);
    payloadObj["read_ts"] = static_cast<qint64>(readTs);
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_READ_ACK_REQ, payload);
}

void AppController::sendPrivateReadAck(int peerUid, qint64 readTs)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || peerUid <= 0) {
        return;
    }
    if (readTs <= 0) {
        readTs = QDateTime::currentMSecsSinceEpoch();
    }
    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["peer_uid"] = peerUid;
    payloadObj["read_ts"] = static_cast<qint64>(readTs);
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PRIVATE_READ_ACK_REQ, payload);
}

void AppController::selectChatByUid(int uid)
{
    qInfo() << "Selecting private chat by uid:" << uid
            << "current chat uid:" << _current_chat_uid
            << "current group id:" << _current_group_id;
    auto friendInfo = _gateway.userMgr()->GetFriendById(uid);
    if (!friendInfo) {
        const int dialogIndex = _dialog_list_model.indexOfUid(uid);
        const QVariantMap dialogItem = dialogIndex >= 0 ? _dialog_list_model.get(dialogIndex) : QVariantMap();
        auto placeholder = DialogListService::buildPlaceholderAuthInfo(
            uid,
            dialogItem,
            QStringLiteral("qrc:/res/head_1.jpg"));
        _gateway.userMgr()->AddFriend(placeholder);
        _chat_list_model.upsertFriend(placeholder);
        _contact_list_model.upsertFriend(placeholder);
        _dialog_list_model.upsertFriend(placeholder);
        friendInfo = _gateway.userMgr()->GetFriendById(uid);
        if (!friendInfo) {
            return;
        }
    }

    const int dialogIndex = _dialog_list_model.indexOfUid(uid);
    const QVariantMap dialogItem = dialogIndex >= 0 ? _dialog_list_model.get(dialogIndex) : QVariantMap();
    DialogEntrySeed seed;
    seed.dialogUid = uid;
    seed.dialogType = QStringLiteral("private");
    seed.userId = friendInfo->_user_id;
    seed.name = dialogItem.value(QStringLiteral("name"), friendInfo->_name).toString();
    seed.nick = dialogItem.value(QStringLiteral("nick"), friendInfo->_nick).toString();
    seed.icon = dialogItem.value(QStringLiteral("icon"), friendInfo->_icon).toString();
    seed.desc = friendInfo->_desc;
    seed.back = friendInfo->_back;
    seed.previewText = dialogItem.value(QStringLiteral("lastMsg"), friendInfo->_last_msg).toString();
    seed.sex = friendInfo->_sex;
    seed.pinnedRank = dialogItem.value(QStringLiteral("pinnedRank")).toInt();
    seed.draftText = dialogItem.value(QStringLiteral("draftText")).toString();
    seed.lastMsgTs = dialogItem.value(QStringLiteral("lastMsgTs")).toLongLong();
    seed.muteState = dialogItem.value(QStringLiteral("muteState")).toInt();
    if (seed.lastMsgTs <= 0 && !friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back()) {
        seed.lastMsgTs = friendInfo->_chat_msgs.back()->_created_at;
    }
    DialogListService::applyFriendProfileToPrivateSeed(seed, friendInfo);
    const DialogDecorationState decorationState {
        &_dialog_draft_map,
        &_dialog_mention_map,
        &_dialog_server_pinned_map,
        &_dialog_server_mute_map,
        &_dialog_local_pinned_set
    };
    auto dialogEntry = DialogListService::buildDialogEntry(seed, decorationState);
    if (dialogEntry) {
        _chat_list_model.upsertFriend(dialogEntry);
        _dialog_list_model.upsertFriend(dialogEntry);
    }

    _current_chat_uid = uid;
    _dialog_list_model.clearUnread(uid);
    _chat_list_model.clearUnread(uid);
    setCurrentGroup(0, QString());
    setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
    setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty()
                           ? QStringLiteral("qrc:/res/head_1.jpg")
                           : friendInfo->_icon);
    emitCurrentDialogUidChangedIfNeeded();
    qInfo() << "Private chat resolved, uid:" << uid
            << "name:" << friendInfo->_name
            << "friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size());
    loadCurrentChatMessages();
    syncCurrentDialogDraft();
}
