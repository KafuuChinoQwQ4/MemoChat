#include "ChatDialogSelectionPortFactory.h"

#include <utility>

namespace memochat::app
{

ChatDialogSelectionPort makeChatDialogSelectionPort(ChatDialogSelectionActions actions)
{
    ChatDialogSelectionPort port;
    port.groupDialogs = std::move(actions.groupDialogs);
    port.ensureGroupsInitialized = std::move(actions.ensureGroupsInitialized);
    port.friendById = std::move(actions.friendById);
    port.addFriend = std::move(actions.addFriend);
    port.upsertContact = std::move(actions.upsertContact);
    port.groupById = std::move(actions.groupById);
    port.upsertGroup = std::move(actions.upsertGroup);
    port.dialogDecorationState = std::move(actions.dialogDecorationState);
    port.setPendingReplyContext = std::move(actions.setPendingReplyContext);
    port.setCurrentPrivatePeerUid = std::move(actions.setCurrentPrivatePeerUid);
    port.setCurrentGroup = std::move(actions.setCurrentGroup);
    port.setCurrentChatPeerName = std::move(actions.setCurrentChatPeerName);
    port.setCurrentChatPeerIcon = std::move(actions.setCurrentChatPeerIcon);
    port.clearMessageModel = std::move(actions.clearMessageModel);
    port.clearPrivateHistoryState = std::move(actions.clearPrivateHistoryState);
    port.resetGroupConversationState = std::move(actions.resetGroupConversationState);
    port.setGroupHistoryLoading = std::move(actions.setGroupHistoryLoading);
    port.setPrivateHistoryLoading = std::move(actions.setPrivateHistoryLoading);
    port.setCanLoadMorePrivateHistory = std::move(actions.setCanLoadMorePrivateHistory);
    port.removeMentionForDialog = std::move(actions.removeMentionForDialog);
    port.emitCurrentDialogUidChangedIfNeeded = std::move(actions.emitCurrentDialogUidChangedIfNeeded);
    port.loadCurrentPrivateMessages = std::move(actions.loadCurrentPrivateMessages);
    port.syncCurrentDialogDraft = std::move(actions.syncCurrentDialogDraft);
    port.openGroupConversation = std::move(actions.openGroupConversation);
    port.sendGroupReadAck = std::move(actions.sendGroupReadAck);
    port.loadGroupHistory = std::move(actions.loadGroupHistory);
    return port;
}

} // namespace memochat::app
