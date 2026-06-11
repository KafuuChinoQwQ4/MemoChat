#include "AppController.h"

#include "AppCoordinators.h"
#include "ChatDialogSelectionPortFactory.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <utility>

void AppController::bindChatFeatureDialogPorts()
{
    ChatDialogNavigationPort navigationPort;

    ChatListPagingPort listPagingPort;
    listPagingPort.snapshot = [this]()
    {
        ChatListPagingSnapshot snapshot;
        snapshot.initialized = _shell_state.bootstrapState().chatListInitialized;
        snapshot.loading = _shell_state.loadingState().chatLoadingMore;
        return snapshot;
    };
    listPagingPort.nextPage = [this]()
    {
        return _gateway.userMgr()->GetChatListPerPage();
    };
    listPagingPort.loadFinished = [this]()
    {
        return _gateway.userMgr()->IsLoadChatFin();
    };
    listPagingPort.markPageLoaded = [this]()
    {
        _gateway.userMgr()->UpdateChatLoadedCount();
    };
    listPagingPort.setInitialized = [this](bool initialized)
    {
        _shell_state.bootstrapState().chatListInitialized = initialized;
    };
    listPagingPort.setLoading = [this](bool loading)
    {
        setChatLoadingMore(loading);
    };
    listPagingPort.setCanLoadMore = [this](bool canLoadMore)
    {
        if (_shell_state.loadingState().canLoadMoreChats == canLoadMore)
        {
            return;
        }
        _shell_state.loadingState().canLoadMoreChats = canLoadMore;
        syncChatViewModelState();
    };
    listPagingPort.flushIncomingMessages = [this]()
    {
        flushIncomingMessageRouter();
    };

    ChatDialogRuntimePort runtimePort;
    runtimePort.syncCurrentDraftText = [this](const QString& text)
    {
        setCurrentDraftText(text);
    };
    runtimePort.syncCurrentDialogPinned = [this](bool pinned)
    {
        setCurrentDialogPinned(pinned);
    };
    runtimePort.syncCurrentDialogMuted = [this](bool muted)
    {
        setCurrentDialogMuted(muted);
    };

    ChatDialogCommandPort commandPort;
    commandPort.snapshot = [this]()
    {
        ChatDialogCommandSnapshot snapshot;
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        snapshot.selfUid = selfInfo ? selfInfo->_uid : 0;
        snapshot.ownerUid = _gateway.userMgr()->GetUid();
        snapshot.currentDialogUid = currentDialogUid();
        snapshot.currentPrivatePeerUid = _chat_state.uid;
        snapshot.currentGroupId = currentGroupId();
        return snapshot;
    };
    commandPort.targetForDialogUid = [this](int dialogUid)
    {
        ChatDialogTarget target;
        if (dialogUid > 0)
        {
            target.dialogType = QStringLiteral("private");
            target.peerUid = dialogUid;
            return target;
        }
        const qint64 groupId = groupIdForDialogUid(dialogUid);
        if (groupId > 0)
        {
            target.dialogType = QStringLiteral("group");
            target.groupId = groupId;
        }
        return target;
    };
    commandPort.privatePeerById = [this](int peerUid)
    {
        return _gateway.userMgr()->GetFriendById(peerUid);
    };
    commandPort.groupById = [this](qint64 groupId)
    {
        return _gateway.userMgr()->GetGroupById(groupId);
    };
    commandPort.dispatchPayload = [this](int reqId, const QByteArray& payload)
    {
        if (const auto transport = _gateway.chatTransport())
        {
            transport->slot_send_data(static_cast<ReqId>(reqId), payload);
        }
    };
    commandPort.savePersistentDialogStore = [this](int ownerUid)
    {
        saveDraftStore(ownerUid);
    };
    commandPort.refreshDialogModel = [this]()
    {
        refreshDialogModel();
    };
    commandPort.groupListModel = _features.groupController.groupListModel();

    ChatShellIntentPort shellIntentPort;
    shellIntentPort.startVoiceChat = [this]()
    {
        _call_coordinator->startVoiceChat();
    };
    shellIntentPort.startVideoChat = [this]()
    {
        _call_coordinator->startVideoChat();
    };

    _features.chatFeatureController.setChatListPagingPort(std::move(listPagingPort));
    _features.chatFeatureController.setDialogSelectionPort(
        memochat::app::makeChatDialogSelectionPort(chatDialogSelectionActions()));
    _features.chatFeatureController.setDialogNavigationPort(std::move(navigationPort));
    _features.chatFeatureController.setDialogRuntimePort(std::move(runtimePort));
    _features.chatFeatureController.setDialogCommandPort(std::move(commandPort));
    _features.chatFeatureController.setShellIntentPort(std::move(shellIntentPort));
}
