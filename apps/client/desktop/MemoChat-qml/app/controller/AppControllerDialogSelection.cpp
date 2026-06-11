#include "AppController.h"

#include "ChatEventDependenciesFactory.h"
#include "ChatDialogSelectionPortFactory.h"
#include "IChatTransport.h"
#include "usermgr.h"

memochat::app::ChatDialogSelectionActions AppController::chatDialogSelectionActions()
{
    memochat::app::ChatDialogSelectionActions actions;
    actions.groupDialogs.groupDialogByIndex = [this](int index)
    {
        if (const auto model = _features.groupController.groupListModel())
        {
            return model->get(index);
        }
        return QVariantMap();
    };
    actions.groupDialogs.indexOfGroupDialog = [this](int dialogUid)
    {
        if (const auto model = _features.groupController.groupListModel())
        {
            return model->indexOfUid(dialogUid);
        }
        return -1;
    };
    actions.groupDialogs.groupDialogItem = [this](int dialogUid)
    {
        if (const auto model = _features.groupController.groupListModel())
        {
            const int index = model->indexOfUid(dialogUid);
            return index >= 0 ? model->get(index) : QVariantMap();
        }
        return QVariantMap();
    };
    actions.groupDialogs.groupIdForDialogUid = [this](int dialogUid)
    {
        return groupIdForDialogUid(dialogUid);
    };
    actions.groupDialogs.clearGroupUnread = [this](int dialogUid)
    {
        if (const auto model = _features.groupController.groupListModel())
        {
            model->clearUnread(dialogUid);
        }
    };
    actions.ensureGroupsInitialized = [this]()
    {
        ensureGroupsInitialized();
    };
    actions.friendById = [this](int uid)
    {
        return _gateway.userMgr()->GetFriendById(uid);
    };
    actions.addFriend = [this](std::shared_ptr<AuthInfo> authInfo)
    {
        _gateway.userMgr()->AddFriend(authInfo);
    };
    actions.upsertContact = [this](std::shared_ptr<AuthInfo> authInfo)
    {
        _features.contactController.upsertContact(authInfo);
    };
    actions.groupById = [this](qint64 groupId)
    {
        return _gateway.userMgr()->GetGroupById(groupId);
    };
    actions.upsertGroup = [this](std::shared_ptr<GroupInfoData> groupInfo)
    {
        _gateway.userMgr()->UpsertGroup(groupInfo);
    };
    actions.dialogDecorationState = [this]()
    {
        return _features.chatFeatureController.dialogDecorationState();
    };
    actions.setPendingReplyContext = [this](const QString& msgId, const QString& senderName, const QString& preview)
    {
        setPendingReplyContext(msgId, senderName, preview);
    };
    actions.setCurrentPrivatePeerUid = [this](int uid)
    {
        _chat_state.uid = uid;
    };
    actions.setCurrentGroup = [this](qint64 groupId, const QString& name, const QString& groupCode)
    {
        setCurrentGroup(groupId, name, groupCode);
    };
    actions.setCurrentChatPeerName = [this](const QString& name)
    {
        setCurrentChatPeerName(name);
    };
    actions.setCurrentChatPeerIcon = [this](const QString& icon)
    {
        setCurrentChatPeerIcon(icon);
    };
    actions.clearMessageModel = [this]()
    {
        _features.chatFeatureController.clearMessageModel();
    };
    actions.clearPrivateHistoryState = [this]()
    {
        clearPrivateHistoryFeatureState();
    };
    actions.resetGroupConversationState = [this]()
    {
        resetGroupConversationFeatureState();
    };
    actions.setGroupHistoryLoading = [this](bool loading)
    {
        _shell_state.loadingState().groupHistoryLoading = loading;
    };
    actions.setPrivateHistoryLoading = [this](bool loading)
    {
        setPrivateHistoryLoading(loading);
    };
    actions.setCanLoadMorePrivateHistory = [this](bool canLoad)
    {
        setCanLoadMorePrivateHistory(canLoad);
    };
    actions.removeMentionForDialog = [this](int dialogUid)
    {
        _features.chatFeatureController.removeMentionForDialog(dialogUid);
    };
    actions.emitCurrentDialogUidChangedIfNeeded = [this]()
    {
        emitCurrentDialogUidChangedIfNeeded();
    };
    actions.loadCurrentPrivateMessages = [this]()
    {
        loadCurrentChatMessages();
    };
    actions.syncCurrentDialogDraft = [this]()
    {
        syncCurrentDialogDraft();
    };
    actions.openGroupConversation = [this](qint64 groupId)
    {
        GroupOpenRequest request;
        request.context = groupConversationContext(_gateway.userMgr()->GetUid());
        request.groupId = groupId;
        const GroupOpenResult result = _features.chatFeatureController.openGroupConversation(
            request,
            memochat::app::makeGroupConversationDependencies(
                _features.chatFeatureController,
                _gateway.userMgr(),
                _features.groupController.groupListModel(),
                [this](int reqId, const QByteArray& payload)
                {
                    if (const auto transport = _gateway.chatTransport())
                    {
                        transport->slot_send_data(static_cast<ReqId>(reqId), payload);
                    }
                }));
        return result.requestedReadAckTs;
    };
    actions.sendGroupReadAck = [this](qint64 groupId, qint64 readTs)
    {
        _features.chatFeatureController.sendGroupReadAckForGroup(groupId, readTs);
    };
    actions.loadGroupHistory = [this]()
    {
        _features.groupController.loadGroupHistory();
    };
    return actions;
}

void AppController::selectDialogByUid(int uid)
{
    _features.chatFeatureController.selectDialogByUid(uid);
}

void AppController::jumpChatWithCurrentContact()
{
    const int contactUid = _features.contactController.currentContactUid();
    if (contactUid <= 0)
    {
        return;
    }

    selectChatByUid(contactUid);
    switchChatTab(ChatTabPage);
}
