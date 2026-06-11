#include "AppController.h"

#include "ContactEventDependenciesFactory.h"
#include "usermgr.h"

memochat::app::ContactEventActions AppController::contactEventActions()
{
    memochat::app::ContactEventActions actions;
    actions.addAuthFriendToStore = [this](std::shared_ptr<AuthInfo> authInfo)
    {
        _gateway.userMgr()->AddFriend(authInfo);
    };
    actions.addAuthRspToStore = [this](std::shared_ptr<AuthRsp> authRsp)
    {
        _gateway.userMgr()->AddFriend(authRsp);
    };
    actions.removeFriendFromStore = [this](int friendUid)
    {
        _gateway.userMgr()->RemoveFriend(friendUid);
    };
    actions.currentUser = [this]()
    {
        return _gateway.userMgr()->GetUserInfo();
    };
    actions.isFriend = [this](int uid)
    {
        return _gateway.userMgr()->CheckFriendById(uid);
    };
    actions.alreadyApplied = [this](int uid)
    {
        return _gateway.userMgr()->AlreadyApply(uid);
    };
    actions.markApplyApprovedInStore = [this](int uid)
    {
        _gateway.userMgr()->MarkApplyStatus(uid, 1);
    };
    actions.addApplyToStore = [this](std::shared_ptr<ApplyInfo> apply)
    {
        _gateway.userMgr()->AddApplyList(apply);
    };
    actions.upsertChatContactFromAuthInfo = [this](std::shared_ptr<AuthInfo> authInfo)
    {
        _features.chatFeatureController.upsertContactFromAuthInfo(authInfo);
    };
    actions.upsertChatContactFromAuthRsp = [this](std::shared_ptr<AuthRsp> authRsp)
    {
        _features.chatFeatureController.upsertContactFromAuthRsp(authRsp);
    };
    actions.removeChatContact = [this](int friendUid)
    {
        ChatPrivateConversationClearPort port;
        port.currentPrivatePeerUid = [this]()
        {
            return _chat_state.uid;
        };
        port.setCurrentPrivatePeerUid = [this](int uid)
        {
            _chat_state.uid = uid;
        };
        port.clearMessageModel = [this]()
        {
            _features.chatFeatureController.clearMessageModel();
        };
        port.setCurrentPeerName = [this](const QString& name)
        {
            setCurrentChatPeerName(name);
        };
        port.setCurrentPeerIcon = [this](const QString& icon)
        {
            setCurrentChatPeerIcon(icon);
        };
        port.setCurrentDraftText = [this](const QString& text)
        {
            setCurrentDraftText(text);
        };
        port.setCurrentDialogPinned = [this](bool pinned)
        {
            setCurrentDialogPinned(pinned);
        };
        port.setCurrentDialogMuted = [this](bool muted)
        {
            setCurrentDialogMuted(muted);
        };
        port.emitCurrentDialogUidChangedIfNeeded = [this]()
        {
            emitCurrentDialogUidChangedIfNeeded();
        };
        _features.chatFeatureController.removeContactConversation(friendUid, port);
    };
    actions.openPrivateChat = [this](int uid)
    {
        selectChatByUid(uid);
    };
    actions.switchToChatTab = [this]()
    {
        switchChatTab(ChatTabPage);
    };
    actions.refreshDialogModel = [this]()
    {
        refreshDialogModel();
    };
    actions.refreshChatLoadMoreState = [this]()
    {
        refreshChatLoadMoreState();
    };
    actions.refreshContactLoadMoreState = [this]()
    {
        refreshContactLoadMoreState();
    };
    return actions;
}
