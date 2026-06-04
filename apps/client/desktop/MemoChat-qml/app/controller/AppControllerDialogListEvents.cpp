#include "AppController.h"
#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QVariantMap>

void AppController::onDialogListRsp(QJsonObject payload)
{
    const bool bootstrappingDialog = _bootstrap_state.dialogBootstrapLoading;
    _bootstrap_state.dialogBootstrapLoading = false;
    setDialogsReady(true);
    _bootstrap_state.chatListInitialized = true;

    if (!payload.contains("dialogs"))
    {
        if (bootstrappingDialog && _chat_list_model.count() > 0 && _chat_state.uid <= 0 && _group_state.currentId <= 0)
        {
            selectChatIndex(0);
        }
        flushPendingIncomingMessages();
        return;
    }
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        if (bootstrappingDialog && _chat_list_model.count() > 0 && _chat_state.uid <= 0 && _group_state.currentId <= 0)
        {
            selectChatIndex(0);
        }
        flushPendingIncomingMessages();
        return;
    }

    const QJsonArray dialogs = payload.value("dialogs").toArray();
    std::vector<std::shared_ptr<FriendInfo>> merged;
    merged.reserve(dialogs.size());
    QSet<int> privateDialogUids;
    QSet<qint64> groupDialogIds;
    _dialog_state.serverPinnedMap.clear();
    _dialog_state.serverMuteMap.clear();
    const DialogDecorationState decorationState{&_dialog_state.draftMap,
                                                &_dialog_state.mentionMap,
                                                nullptr,
                                                nullptr,
                                                &_dialog_state.localPinnedSet};

    for (const auto& one : dialogs)
    {
        const QJsonObject obj = one.toObject();
        const QString dialogType = obj.value("dialog_type").toString();
        const QString title = obj.value("title").toString();
        const QString avatar = obj.value("avatar").toString();
        const QString preview = obj.value("last_msg_preview").toString();

        if (dialogType == "private")
        {
            const int peerUid = obj.value("peer_uid").toInt();
            if (peerUid <= 0)
            {
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
            if (lastMsgTs > 0 && lastMsgTs < 100000000000LL)
            {
                lastMsgTs *= 1000;
            }
            seed.lastMsgTs = lastMsgTs;
            seed.muteState = obj.value("mute_state").toInt(0);
            _dialog_state.serverPinnedMap.insert(peerUid, seed.pinnedRank);
            _dialog_state.serverMuteMap.insert(peerUid, seed.muteState);
            privateDialogUids.insert(peerUid);
            DialogListService::applyFriendProfileToPrivateSeed(seed, _gateway.userMgr()->GetFriendById(peerUid));
            auto item = DialogListService::buildDialogEntry(seed, decorationState);
            merged.push_back(item);
            _chat_list_model.upsertFriend(item);
            continue;
        }

        if (dialogType == "group")
        {
            const qint64 groupId = obj.value("group_id").toVariant().toLongLong();
            if (groupId <= 0)
            {
                continue;
            }
            const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, groupId);
            if (pseudoUid == 0)
            {
                continue;
            }
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
            if (lastMsgTs > 0 && lastMsgTs < 100000000000LL)
            {
                lastMsgTs *= 1000;
            }
            seed.lastMsgTs = lastMsgTs;
            seed.muteState = obj.value("mute_state").toInt(0);
            auto groupInfo = std::make_shared<GroupInfoData>();
            groupInfo->_group_id = groupId;
            groupInfo->_name = title;
            groupInfo->_icon = groupIcon;
            groupInfo->_last_msg = preview;
            _gateway.userMgr()->UpsertGroup(groupInfo);
            _dialog_state.serverPinnedMap.insert(pseudoUid, seed.pinnedRank);
            _dialog_state.serverMuteMap.insert(pseudoUid, seed.muteState);
            groupDialogIds.insert(groupId);
            auto item = DialogListService::buildDialogEntry(seed, decorationState);
            merged.push_back(item);
            _group_list_model.upsertFriend(item);
            _group_state.dialogUidMap.insert(pseudoUid, groupId);
        }
    }

    DialogListService::appendMissingPrivateDialogs(merged,
                                                   _gateway.userMgr()->GetFriendListSnapshot(),
                                                   privateDialogUids,
                                                   decorationState);
    DialogListService::appendMissingGroupDialogs(merged,
                                                 _gateway.userMgr()->GetGroupListSnapshot(),
                                                 _group_state.dialogUidMap,
                                                 groupDialogIds,
                                                 decorationState);
    std::vector<QVariantMap> existingDialogs;
    existingDialogs.reserve(static_cast<size_t>(_dialog_list_model.count()));
    for (int i = 0; i < _dialog_list_model.count(); ++i)
    {
        existingDialogs.push_back(_dialog_list_model.get(i));
    }
    DialogListService::appendExistingDialogs(merged, existingDialogs, decorationState);
    for (const auto& dialog : merged)
    {
        if (dialog && dialog->_uid > 0)
        {
            _chat_list_model.upsertFriend(dialog);
        }
        else if (dialog && dialog->_uid < 0)
        {
            _group_list_model.upsertFriend(dialog);
        }
    }

    if (merged.empty() &&
        (!_gateway.userMgr()->GetFriendListSnapshot().empty() || !_gateway.userMgr()->GetGroupListSnapshot().empty()))
    {
        refreshDialogModel();
        if (bootstrappingDialog && _chat_state.uid <= 0 && _group_state.currentId <= 0 &&
            _dialog_list_model.count() > 0)
        {
            const QVariantMap firstDialog = _dialog_list_model.get(0);
            const int firstUid = firstDialog.value("uid").toInt();
            if (firstUid > 0)
            {
                selectChatByUid(firstUid);
            }
            else if (firstUid < 0)
            {
                const int groupIndex = _group_list_model.indexOfUid(firstUid);
                if (groupIndex >= 0)
                {
                    selectGroupIndex(groupIndex);
                }
            }
        }
        flushPendingIncomingMessages();
        return;
    }

    DialogListService::sortDialogs(merged);
    _dialog_list_model.upsertBatch(merged, true);
    if (_group_state.currentId > 0)
    {
        const int selectedGroupDialogUid =
            ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, _group_state.currentId);
        _dialog_list_model.clearUnread(selectedGroupDialogUid);
        _dialog_list_model.clearMention(selectedGroupDialogUid);
        _dialog_state.mentionMap.remove(selectedGroupDialogUid);
    }
    else if (_chat_state.uid > 0)
    {
        _dialog_list_model.clearUnread(_chat_state.uid);
        _dialog_list_model.clearMention(_chat_state.uid);
        _dialog_state.mentionMap.remove(_chat_state.uid);
    }
    syncCurrentDialogDraft();

    const bool shouldSelectDialog = bootstrappingDialog || (_chat_state.uid <= 0 && _group_state.currentId <= 0);
    if (shouldSelectDialog)
    {
        if (!merged.empty() && merged.front())
        {
            const int topDialogUid = merged.front()->_uid;
            if (topDialogUid > 0)
            {
                selectChatByUid(topDialogUid);
                _dialog_list_model.clearUnread(topDialogUid);
                _dialog_list_model.clearMention(topDialogUid);
                _dialog_state.mentionMap.remove(topDialogUid);
            }
            else if (topDialogUid < 0)
            {
                const int topGroupIndex = _group_list_model.indexOfUid(topDialogUid);
                if (topGroupIndex >= 0)
                {
                    selectGroupIndex(topGroupIndex);
                }
            }
        }
        else if (_chat_list_model.count() > 0)
        {
            selectChatIndex(0);
        }
    }

    if (bootstrappingDialog)
    {
        for (const auto& dialog : merged)
        {
            if (!dialog)
            {
                continue;
            }
            if (dialog->_uid > 0 && dialog->_unread_count > 0)
            {
                if (dialog->_uid == _chat_state.uid)
                {
                    requestPrivateHistory(dialog->_uid, 0, QString());
                }
                else
                {
                    requestPrivateHistoryForBootstrap(dialog->_uid);
                }
            }
            else if (dialog->_uid < 0 && dialog->_unread_count > 0)
            {
                const qint64 groupId =
                    ConversationSyncService::groupIdForDialogUid(_group_state.dialogUidMap, dialog->_uid);
                requestGroupHistoryForBootstrap(groupId);
            }
        }
    }
    flushPendingIncomingMessages();
}
