#include "AppController.h"

#include "ChatDialogListResponseService.h"
#include "usermgr.h"

#include <utility>

ChatDialogListResponseContext AppController::dialogListResponseContext() const
{
    ChatDialogListResponseContext context;
    context.bootstrappingDialog = _shell_state.bootstrapState().dialogBootstrapLoading;
    context.hasCurrentDialog = _chat_state.uid > 0 || currentGroupId() > 0;
    context.currentPrivatePeerUid = _chat_state.uid;
    context.currentGroupId = currentGroupId();
    context.friendSnapshot = _gateway.userMgr()->GetFriendListSnapshot();
    context.groupSnapshot = _gateway.userMgr()->GetGroupListSnapshot();
    return context;
}

ChatDialogListAppPort AppController::dialogListAppPort()
{
    ChatDialogListAppPort port;
    port.setDialogBootstrapLoading = [this](bool loading)
    {
        _shell_state.bootstrapState().dialogBootstrapLoading = loading;
    };
    port.setDialogsReady = [this](bool ready)
    {
        setDialogsReady(ready);
    };
    port.setChatListInitialized = [this](bool initialized)
    {
        _shell_state.bootstrapState().chatListInitialized = initialized;
    };
    port.upsertGroup = [this](std::shared_ptr<GroupInfoData> group)
    {
        _gateway.userMgr()->UpsertGroup(std::move(group));
    };
    port.upsertGroupDialog = [this](std::shared_ptr<FriendInfo> dialog)
    {
        _features.groupController.upsertGroup(std::move(dialog));
    };
    port.refreshDialogModel = [this]()
    {
        refreshDialogModel();
    };
    port.syncCurrentDialogDraft = [this]()
    {
        syncCurrentDialogDraft();
    };
    port.selectDialogByUid = [this](int dialogUid)
    {
        selectDialogByUid(dialogUid);
    };
    port.selectFirstChat = [this]()
    {
        selectChatIndex(0);
    };
    port.requestPrivateHistoryForBootstrap = [this](int peerUid)
    {
        requestPrivateHistoryForBootstrap(peerUid);
    };
    port.requestCurrentPrivateHistory = [this](int peerUid)
    {
        requestPrivateHistory(peerUid, 0, QString());
    };
    port.requestGroupHistoryForBootstrap = [this](qint64 groupId)
    {
        requestGroupHistoryForBootstrap(groupId);
    };
    port.flushIncomingMessages = [this]()
    {
        flushIncomingMessageRouter();
    };
    return port;
}
