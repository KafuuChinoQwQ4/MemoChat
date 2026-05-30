#include "AppController.h"
#include "ConversationSyncService.h"
#include "usermgr.h"

#include <QVariantMap>

void AppController::selectGroupIndex(int index)
{
    ensureGroupsInitialized();
    const QVariantMap item = _group_list_model.get(index);
    if (item.isEmpty())
    {
        setPendingReplyContext(QString(), QString(), QString());
        setCurrentGroup(0, QString());
        _message_model.clear();
        _group_state.historyBeforeSeq = 0;
        _group_state.historyHasMore = true;
        _loading_state.groupHistoryLoading = false;
        setCanLoadMorePrivateHistory(false);
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    const int pseudoUid = item.value("uid").toInt();
    const qint64 groupId = ConversationSyncService::groupIdForDialogUid(_group_state.dialogUidMap, pseudoUid);
    if (groupId <= 0)
    {
        setPendingReplyContext(QString(), QString(), QString());
        setCurrentGroup(0, QString());
        _message_model.clear();
        _group_state.historyBeforeSeq = 0;
        _group_state.historyHasMore = true;
        _loading_state.groupHistoryLoading = false;
        setCanLoadMorePrivateHistory(false);
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    QString groupCode;
    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (groupInfo)
    {
        groupCode = groupInfo->_group_code;
    }
    setPendingReplyContext(QString(), QString(), QString());
    setCurrentGroup(groupId, item.value("name").toString(), groupCode);
    _dialog_list_model.clearUnread(pseudoUid);
    _dialog_list_model.clearMention(pseudoUid);
    _dialog_state.mentionMap.remove(pseudoUid);
    _chat_state.uid = 0;
    _group_state.historyBeforeSeq = 0;
    _group_state.historyHasMore = true;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    setCurrentChatPeerName(_group_state.currentName);
    const QString selectedGroupIcon =
        item.value("icon").toString().trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png")
                                                          : item.value("icon").toString();
    setCurrentChatPeerIcon(selectedGroupIcon);
    emitCurrentDialogUidChangedIfNeeded();

    groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo)
    {
        _message_model.clear();
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (selfInfo && _group_cache_store.isReady())
    {
        const auto localMessages = _group_cache_store.loadRecentMessages(selfInfo->_uid, groupId, 50);
        for (const auto& one : localMessages)
        {
            _gateway.userMgr()->UpsertGroupChatMsg(groupId, one);
        }
    }

    groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo)
    {
        _message_model.clear();
        return;
    }

    _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
    _group_state.historyBeforeSeq = 0;
    _group_state.historyHasMore = true;
    qint64 readTs = 0;
    if (!groupInfo->_chat_msgs.empty() && groupInfo->_chat_msgs.back())
    {
        readTs = groupInfo->_chat_msgs.back()->_created_at;
    }
    sendGroupReadAck(groupId, readTs);
    loadGroupHistory();
    syncCurrentDialogDraft();
}

void AppController::selectGroupByDialogUid(int dialogUid, qint64 groupId)
{
    if (dialogUid >= 0 || groupId <= 0)
    {
        return;
    }

    QVariantMap item;
    const int groupIndex = _group_list_model.indexOfUid(dialogUid);
    if (groupIndex >= 0)
    {
        item = _group_list_model.get(groupIndex);
    }
    if (item.isEmpty())
    {
        const int dialogIndex = _dialog_list_model.indexOfUid(dialogUid);
        if (dialogIndex >= 0)
        {
            item = _dialog_list_model.get(dialogIndex);
        }
    }

    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    QString groupName = item.value("name").toString();
    QString groupIcon = item.value("icon").toString();
    QString groupCode;
    if (groupInfo)
    {
        if (groupName.trimmed().isEmpty())
        {
            groupName = groupInfo->_name;
        }
        if (groupIcon.trimmed().isEmpty())
        {
            groupIcon = groupInfo->_icon;
        }
        groupCode = groupInfo->_group_code;
    }
    if (groupName.trimmed().isEmpty())
    {
        groupName = QString("群聊%1").arg(groupId);
    }
    if (!groupInfo)
    {
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
    _dialog_state.mentionMap.remove(dialogUid);
    _chat_state.uid = 0;
    _group_state.historyBeforeSeq = 0;
    _group_state.historyHasMore = true;
    _loading_state.groupHistoryLoading = false;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    setCurrentChatPeerName(_group_state.currentName);
    setCurrentChatPeerIcon(groupIcon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : groupIcon);
    emitCurrentDialogUidChangedIfNeeded();

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (groupInfo && selfInfo && _group_cache_store.isReady())
    {
        const auto localMessages = _group_cache_store.loadRecentMessages(selfInfo->_uid, groupId, 50);
        for (const auto& one : localMessages)
        {
            _gateway.userMgr()->UpsertGroupChatMsg(groupId, one);
        }
        groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    }

    if (groupInfo)
    {
        _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
        qint64 readTs = 0;
        if (!groupInfo->_chat_msgs.empty() && groupInfo->_chat_msgs.back())
        {
            readTs = groupInfo->_chat_msgs.back()->_created_at;
        }
        sendGroupReadAck(groupId, readTs);
    }
    else
    {
        _message_model.clear();
    }
    loadGroupHistory();
    syncCurrentDialogDraft();
}
