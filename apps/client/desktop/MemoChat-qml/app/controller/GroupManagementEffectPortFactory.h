#pragma once

#include "GroupManagementEffectPorts.h"

#include <functional>

namespace memochat::app
{

struct GroupManagementEventEffectActions
{
    std::function<void(qint64)> removeGroup;
    std::function<void(qint64)> clearGroupConversation;
    std::function<void()> refreshGroupModel;
    std::function<void()> refreshDialogModel;
    std::function<void()> requestDialogList;
    std::function<void(qint64, const QString&, const QString&)> selectCurrentGroup;
    std::function<void()> clearCurrentGroup;
    std::function<void(const QString&)> setCurrentChatPeerIcon;
    std::function<void(qint64, bool)> openGroupConversation;
    std::function<void()> syncCurrentDialogDraft;
    std::function<void()> loadCurrentChatMessages;
    std::function<void()> clearMessageModel;
    std::function<void()> resetCurrentChatProjection;
};

struct GroupManagementResponseEffectActions
{
    std::function<void(qint64)> removeGroup;
    std::function<void(qint64)> clearGroupConversation;
    std::function<void()> refreshGroupModel;
    std::function<void()> refreshDialogModel;
    std::function<void(int)> selectDialogByUid;
    std::function<void(const QString&)> setCurrentChatPeerIcon;
};

memochat::group::GroupManagementEventEffectPort
makeGroupManagementEventEffectPort(GroupManagementEventEffectActions actions);

memochat::group::GroupManagementResponseEffectPort
makeGroupManagementResponseEffectPort(GroupManagementResponseEffectActions actions);

} // namespace memochat::app
