#include "AppController.h"
#include "DialogListService.h"
#include "usermgr.h"

#include <QDebug>
#include <QVariantMap>

void AppController::selectChatIndex(int index)
{
    const QVariantMap item = _chat_list_model.get(index);
    if (item.isEmpty())
    {
        setPendingReplyContext(QString(), QString(), QString());
        _chat_state.uid = 0;
        setCurrentGroup(0, QString());
        setCurrentChatPeerName(QString());
        setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
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
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    setPendingReplyContext(QString(), QString(), QString());
    _chat_state.uid = item.value("uid").toInt();
    _dialog_list_model.clearUnread(_chat_state.uid);
    _chat_list_model.clearUnread(_chat_state.uid);
    setCurrentGroup(0, QString());
    _group_state.historyBeforeSeq = 0;
    _group_state.historyHasMore = true;
    _loading_state.groupHistoryLoading = false;
    qInfo() << "Selecting private chat by index:" << index << "uid:" << _chat_state.uid
            << "name:" << item.value("name").toString();
    const auto friendInfo = _gateway.userMgr()->GetFriendById(_chat_state.uid);
    if (friendInfo)
    {
        _chat_list_model.upsertFriend(friendInfo);
        setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
        setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/head_1.png")
                                                                     : friendInfo->_icon);
    }
    else
    {
        setCurrentChatPeerName(item.value("name").toString());
        const QString icon = item.value("icon").toString();
        setCurrentChatPeerIcon(icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/head_1.png") : icon);
    }
    emitCurrentDialogUidChangedIfNeeded();
    loadCurrentChatMessages();
    syncCurrentDialogDraft();
}

void AppController::selectChatByUid(int uid)
{
    qInfo() << "Selecting private chat by uid:" << uid << "current chat uid:" << _chat_state.uid
            << "current group id:" << _group_state.currentId;
    auto friendInfo = _gateway.userMgr()->GetFriendById(uid);
    if (!friendInfo)
    {
        const int dialogIndex = _dialog_list_model.indexOfUid(uid);
        const QVariantMap dialogItem = dialogIndex >= 0 ? _dialog_list_model.get(dialogIndex) : QVariantMap();
        auto placeholder =
            DialogListService::buildPlaceholderAuthInfo(uid, dialogItem, QStringLiteral("qrc:/res/head_1.png"));
        _gateway.userMgr()->AddFriend(placeholder);
        _chat_list_model.upsertFriend(placeholder);
        _contact_list_model.upsertFriend(placeholder);
        _dialog_list_model.upsertFriend(placeholder);
        friendInfo = _gateway.userMgr()->GetFriendById(uid);
        if (!friendInfo)
        {
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
    if (seed.lastMsgTs <= 0 && !friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back())
    {
        seed.lastMsgTs = friendInfo->_chat_msgs.back()->_created_at;
    }
    DialogListService::applyFriendProfileToPrivateSeed(seed, friendInfo);
    const DialogDecorationState decorationState{&_dialog_state.draftMap,
                                                &_dialog_state.mentionMap,
                                                &_dialog_state.serverPinnedMap,
                                                &_dialog_state.serverMuteMap,
                                                &_dialog_state.localPinnedSet};
    auto dialogEntry = DialogListService::buildDialogEntry(seed, decorationState);
    if (dialogEntry)
    {
        _chat_list_model.upsertFriend(dialogEntry);
        _dialog_list_model.upsertFriend(dialogEntry);
    }

    _chat_state.uid = uid;
    _dialog_list_model.clearUnread(uid);
    _chat_list_model.clearUnread(uid);
    setCurrentGroup(0, QString());
    setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
    setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/head_1.png")
                                                                 : friendInfo->_icon);
    emitCurrentDialogUidChangedIfNeeded();
    qInfo() << "Private chat resolved, uid:" << uid << "name:" << friendInfo->_name
            << "friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size());
    loadCurrentChatMessages();
    syncCurrentDialogDraft();
}
