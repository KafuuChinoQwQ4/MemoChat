#include "ChatFeatureControllerDialogRuntimeInternal.h"

using namespace memochat::chat::dialog_runtime;

ChatPrivateConversationClearResult
ChatFeatureController::clearPrivateConversationIfSelected(const ChatPrivateConversationClearRequest& request,
                                                          const ChatPrivateConversationClearPort& port)
{
    ChatPrivateConversationClearResult result;
    result.friendUid = request.friendUid;
    if (request.friendUid <= 0)
    {
        result.skipped = true;
        return result;
    }

    const int currentPrivatePeerUid = port.currentPrivatePeerUid ? port.currentPrivatePeerUid() : 0;
    if (currentPrivatePeerUid != request.friendUid)
    {
        result.skipped = true;
        return result;
    }

    if (port.setCurrentPrivatePeerUid)
    {
        port.setCurrentPrivatePeerUid(0);
        result.currentPrivateConversationCleared = true;
    }
    if (port.clearMessageModel)
    {
        port.clearMessageModel();
        result.messageModelCleared = true;
    }
    if (port.setCurrentPeerName)
    {
        port.setCurrentPeerName(QString());
    }
    if (port.setCurrentPeerIcon)
    {
        port.setCurrentPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
    }
    result.peerProjectionReset =
        static_cast<bool>(port.setCurrentPeerName) || static_cast<bool>(port.setCurrentPeerIcon);

    if (port.setCurrentDraftText)
    {
        port.setCurrentDraftText(QString());
    }
    if (port.setCurrentDialogPinned)
    {
        port.setCurrentDialogPinned(false);
    }
    if (port.setCurrentDialogMuted)
    {
        port.setCurrentDialogMuted(false);
    }
    result.runtimeProjectionReset = static_cast<bool>(port.setCurrentDraftText) ||
                                    static_cast<bool>(port.setCurrentDialogPinned) ||
                                    static_cast<bool>(port.setCurrentDialogMuted);

    if (port.emitCurrentDialogUidChangedIfNeeded)
    {
        port.emitCurrentDialogUidChangedIfNeeded();
        result.dialogUidEmitted = true;
    }
    result.success = result.currentPrivateConversationCleared || result.messageModelCleared ||
                     result.peerProjectionReset || result.runtimeProjectionReset || result.dialogUidEmitted;
    return result;
}

void ChatFeatureController::upsertContactFromAuthInfo(const std::shared_ptr<AuthInfo>& authInfo)
{
    _chatListModel.upsertFriend(authInfo);
}

void ChatFeatureController::upsertContactFromAuthRsp(const std::shared_ptr<AuthRsp>& authRsp)
{
    _chatListModel.upsertFriend(authRsp);
}

void ChatFeatureController::upsertContactFriendInfo(const std::shared_ptr<FriendInfo>& friendInfo)
{
    _chatListModel.upsertFriend(friendInfo);
}

ChatPrivateConversationClearResult
ChatFeatureController::removeContactConversation(int friendUid, const ChatPrivateConversationClearPort& port)
{
    _chatListModel.removeByUid(friendUid);
    _dialogListModel.removeByUid(friendUid);
    removeDialogDecoration(friendUid);

    ChatPrivateConversationClearRequest request;
    request.friendUid = friendUid;
    return clearPrivateConversationIfSelected(request, port);
}
