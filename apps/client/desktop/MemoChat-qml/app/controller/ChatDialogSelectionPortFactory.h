#pragma once

#include "ChatDialogSelectionService.h"

#include <functional>
#include <memory>

namespace memochat::app
{

struct ChatDialogSelectionActions
{
    ChatGroupDialogLookupPort groupDialogs;
    std::function<void()> ensureGroupsInitialized;
    std::function<std::shared_ptr<FriendInfo>(int)> friendById;
    std::function<void(std::shared_ptr<AuthInfo>)> addFriend;
    std::function<void(std::shared_ptr<AuthInfo>)> upsertContact;
    std::function<std::shared_ptr<GroupInfoData>(qint64)> groupById;
    std::function<void(std::shared_ptr<GroupInfoData>)> upsertGroup;
    std::function<DialogDecorationState()> dialogDecorationState;
    std::function<void(const QString&, const QString&, const QString&)> setPendingReplyContext;
    std::function<void(int)> setCurrentPrivatePeerUid;
    std::function<void(qint64, const QString&, const QString&)> setCurrentGroup;
    std::function<void(const QString&)> setCurrentChatPeerName;
    std::function<void(const QString&)> setCurrentChatPeerIcon;
    std::function<void()> clearMessageModel;
    std::function<void()> clearPrivateHistoryState;
    std::function<void()> resetGroupConversationState;
    std::function<void(bool)> setGroupHistoryLoading;
    std::function<void(bool)> setPrivateHistoryLoading;
    std::function<void(bool)> setCanLoadMorePrivateHistory;
    std::function<void(int)> removeMentionForDialog;
    std::function<void()> emitCurrentDialogUidChangedIfNeeded;
    std::function<void()> loadCurrentPrivateMessages;
    std::function<void()> syncCurrentDialogDraft;
    std::function<qint64(qint64)> openGroupConversation;
    std::function<void(qint64, qint64)> sendGroupReadAck;
    std::function<void()> loadGroupHistory;
};

ChatDialogSelectionPort makeChatDialogSelectionPort(ChatDialogSelectionActions actions);

} // namespace memochat::app
