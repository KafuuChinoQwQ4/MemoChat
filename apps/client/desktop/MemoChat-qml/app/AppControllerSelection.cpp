#include "AppController.h"
#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "IconPathUtils.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

void AppController::selectChatIndex(int index)
{
    const QVariantMap item = _chat_list_model.get(index);
    if (item.isEmpty()) {
        setPendingReplyContext(QString(), QString(), QString());
        _current_chat_uid = 0;
        setCurrentGroup(0, QString());
        setCurrentChatPeerName(QString());
        setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
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
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    setPendingReplyContext(QString(), QString(), QString());
    _current_chat_uid = item.value("uid").toInt();
    _dialog_list_model.clearUnread(_current_chat_uid);
    _chat_list_model.clearUnread(_current_chat_uid);
    setCurrentGroup(0, QString());
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    _group_history_loading = false;
    qInfo() << "Selecting private chat by index:" << index
            << "uid:" << _current_chat_uid
            << "name:" << item.value("name").toString();
    const auto friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    if (friendInfo) {
        _chat_list_model.upsertFriend(friendInfo);
        setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
        setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty()
                               ? QStringLiteral("qrc:/res/head_1.png")
                               : friendInfo->_icon);
    } else {
        setCurrentChatPeerName(item.value("name").toString());
        const QString icon = item.value("icon").toString();
        setCurrentChatPeerIcon(icon.trimmed().isEmpty()
                               ? QStringLiteral("qrc:/res/head_1.png")
                               : icon);
    }
    emitCurrentDialogUidChangedIfNeeded();
    loadCurrentChatMessages();
    syncCurrentDialogDraft();
}

void AppController::selectContactIndex(int index)
{
    ensureContactsInitialized();
    const QVariantMap item = _contact_list_model.get(index);
    if (item.isEmpty()) {
        return;
    }

    const QString nick = item.value("nick").toString();
    QString back = item.value("back").toString();
    if (back.isEmpty()) {
        back = nick;
    }

    setCurrentContact(item.value("uid").toInt(),
                      item.value("name").toString(),
                      nick,
                      item.value("icon").toString(),
                      back,
                      item.value("sex").toInt(),
                      item.value("userId").toString());
    setContactPane(FriendInfoPane);
    setAuthStatus(QString(), false);
}

QVariantMap AppController::contactProfileByUid(int uid) const
{
    const auto friendInfo = _gateway.userMgr()->GetFriendById(uid);
    if (!friendInfo) {
        return {};
    }

    return {
        {"uid", friendInfo->_uid},
        {"userId", friendInfo->_user_id},
        {"name", friendInfo->_name},
        {"nick", friendInfo->_nick},
        {"icon", normalizeIconPath(friendInfo->_icon)},
        {"desc", friendInfo->_desc},
        {"back", friendInfo->_back},
        {"sex", friendInfo->_sex}
    };
}

void AppController::deleteFriend(int uid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || selfInfo->_uid <= 0 || uid <= 0 || uid == selfInfo->_uid) {
        setAuthStatus("联系人状态异常，无法删除", true);
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["friend_uid"] = uid;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_DELETE_FRIEND_REQ, payload);
    setAuthStatus("正在删除联系人...", false);
}

void AppController::selectGroupIndex(int index)
{
    ensureGroupsInitialized();
    const QVariantMap item = _group_list_model.get(index);
    if (item.isEmpty()) {
        setPendingReplyContext(QString(), QString(), QString());
        setCurrentGroup(0, QString());
        _message_model.clear();
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        _group_history_loading = false;
        setCanLoadMorePrivateHistory(false);
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    const int pseudoUid = item.value("uid").toInt();
    const qint64 groupId = ConversationSyncService::groupIdForDialogUid(_group_uid_map, pseudoUid);
    if (groupId <= 0) {
        setPendingReplyContext(QString(), QString(), QString());
        setCurrentGroup(0, QString());
        _message_model.clear();
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        _group_history_loading = false;
        setCanLoadMorePrivateHistory(false);
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    QString groupCode;
    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (groupInfo) {
        groupCode = groupInfo->_group_code;
    }
    setPendingReplyContext(QString(), QString(), QString());
    setCurrentGroup(groupId, item.value("name").toString(), groupCode);
    _dialog_list_model.clearUnread(pseudoUid);
    _dialog_list_model.clearMention(pseudoUid);
    _dialog_mention_map.remove(pseudoUid);
    _current_chat_uid = 0;
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    setCurrentChatPeerName(_current_group_name);
    const QString selectedGroupIcon = item.value("icon").toString().trimmed().isEmpty()
        ? QStringLiteral("qrc:/res/chat_icon.png")
        : item.value("icon").toString();
    setCurrentChatPeerIcon(selectedGroupIcon);
    emitCurrentDialogUidChangedIfNeeded();

    groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo) {
        _message_model.clear();
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (selfInfo && _group_cache_store.isReady()) {
        const auto localMessages = _group_cache_store.loadRecentMessages(selfInfo->_uid, groupId, 50);
        for (const auto &one : localMessages) {
            _gateway.userMgr()->UpsertGroupChatMsg(groupId, one);
        }
    }

    groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo) {
        _message_model.clear();
        return;
    }

    _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    qint64 readTs = 0;
    if (!groupInfo->_chat_msgs.empty() && groupInfo->_chat_msgs.back()) {
        readTs = groupInfo->_chat_msgs.back()->_created_at;
    }
    sendGroupReadAck(groupId, readTs);
    loadGroupHistory();
    syncCurrentDialogDraft();
}

void AppController::selectGroupByDialogUid(int dialogUid, qint64 groupId)
{
    if (dialogUid >= 0 || groupId <= 0) {
        return;
    }

    QVariantMap item;
    const int groupIndex = _group_list_model.indexOfUid(dialogUid);
    if (groupIndex >= 0) {
        item = _group_list_model.get(groupIndex);
    }
    if (item.isEmpty()) {
        const int dialogIndex = _dialog_list_model.indexOfUid(dialogUid);
        if (dialogIndex >= 0) {
            item = _dialog_list_model.get(dialogIndex);
        }
    }

    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    QString groupName = item.value("name").toString();
    QString groupIcon = item.value("icon").toString();
    QString groupCode;
    if (groupInfo) {
        if (groupName.trimmed().isEmpty()) {
            groupName = groupInfo->_name;
        }
        if (groupIcon.trimmed().isEmpty()) {
            groupIcon = groupInfo->_icon;
        }
        groupCode = groupInfo->_group_code;
    }
    if (groupName.trimmed().isEmpty()) {
        groupName = QString("群聊%1").arg(groupId);
    }
    if (!groupInfo) {
        groupInfo = std::make_shared<GroupInfoData>();
        groupInfo->_group_id = groupId;
        groupInfo->_name = groupName;
        groupInfo->_icon = groupIcon;
        _gateway.userMgr()->UpsertGroup(groupInfo);
    }

    setPendingReplyContext(QString(), QString(), QString());
    setCurrentGroup(groupId, groupName, groupCode);
    _dialog_list_model.clearUnread(dialogUid);
    _dialog_list_model.clearMention(dialogUid);
    _group_list_model.clearUnread(dialogUid);
    _dialog_mention_map.remove(dialogUid);
    _current_chat_uid = 0;
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    _group_history_loading = false;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    setCurrentChatPeerName(_current_group_name);
    setCurrentChatPeerIcon(groupIcon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : groupIcon);
    emitCurrentDialogUidChangedIfNeeded();

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (groupInfo && selfInfo && _group_cache_store.isReady()) {
        const auto localMessages = _group_cache_store.loadRecentMessages(selfInfo->_uid, groupId, 50);
        for (const auto &one : localMessages) {
            _gateway.userMgr()->UpsertGroupChatMsg(groupId, one);
        }
        groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    }

    if (groupInfo) {
        _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
        qint64 readTs = 0;
        if (!groupInfo->_chat_msgs.empty() && groupInfo->_chat_msgs.back()) {
            readTs = groupInfo->_chat_msgs.back()->_created_at;
        }
        sendGroupReadAck(groupId, readTs);
    } else {
        _message_model.clear();
    }
    loadGroupHistory();
    syncCurrentDialogDraft();
}

void AppController::selectDialogByUid(int uid)
{
    if (uid == 0) {
        return;
    }

    qInfo() << "Selecting dialog by uid:" << uid
            << "current chat uid:" << _current_chat_uid
            << "current group id:" << _current_group_id;

    if (uid > 0) {
        selectChatByUid(uid);
        return;
    }

    const int groupIndex = _group_list_model.indexOfUid(uid);
    if (groupIndex >= 0) {
        selectGroupIndex(groupIndex);
        return;
    }

    const qint64 groupId = ConversationSyncService::groupIdForDialogUid(_group_uid_map, uid);
    if (groupId > 0) {
        selectGroupByDialogUid(uid, groupId);
    }
}

void AppController::showApplyRequests()
{
    ensureApplyInitialized();
    setContactPane(ApplyRequestPane);
    setAuthStatus(QString(), false);
}

void AppController::jumpChatWithCurrentContact()
{
    if (_current_contact_uid <= 0) {
        return;
    }

    selectChatByUid(_current_contact_uid);
    switchChatTab(ChatTabPage);
}

void AppController::loadMoreChats()
{
    ensureChatListInitialized();
    if (_chat_loading_more) {
        return;
    }

    if (_gateway.userMgr()->IsLoadChatFin()) {
        refreshChatLoadMoreState();
        return;
    }

    setChatLoadingMore(true);
    const auto chatList = _gateway.userMgr()->GetChatListPerPage();
    _chat_list_model.appendFriends(chatList);
    _gateway.userMgr()->UpdateChatLoadedCount();
    setChatLoadingMore(false);
    refreshChatLoadMoreState();
}

void AppController::loadMorePrivateHistory()
{
    if (_private_history_loading || !_can_load_more_private_history) {
        return;
    }
    if (_current_group_id > 0) {
        loadGroupHistory();
        return;
    }
    if (_current_chat_uid > 0) {
        qint64 beforeTs = _message_model.earliestCreatedAt();
        QString beforeMsgId = _message_model.earliestMsgId();
        if (beforeTs <= 0) {
            beforeTs = _private_history_before_ts;
            beforeMsgId = _private_history_before_msg_id;
        }
        requestPrivateHistory(_current_chat_uid, beforeTs, beforeMsgId);
    }
}

void AppController::loadMoreContacts()
{
    ensureContactsInitialized();
    if (_contact_loading_more) {
        return;
    }

    if (_gateway.userMgr()->IsLoadConFin()) {
        refreshContactLoadMoreState();
        return;
    }

    setContactLoadingMore(true);
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    _contact_list_model.appendFriends(contactList);
    _gateway.userMgr()->UpdateContactLoadedCount();
    setContactLoadingMore(false);
    refreshContactLoadMoreState();
}
