#ifndef GROUPMANAGEMENTEFFECTPORTS_H
#define GROUPMANAGEMENTEFFECTPORTS_H

#include <QString>
#include <QtGlobal>
#include <functional>

namespace memochat::group
{

struct GroupManagementEventEffectPort
{
    std::function<void(const QString&, bool)> setStatus;
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
    std::function<void()> refreshGroupList;
};

struct GroupManagementResponseEffectPort
{
    std::function<void(const QString&, bool)> setStatus;
    std::function<void(qint64)> removeGroup;
    std::function<void(qint64)> clearGroupConversation;
    std::function<void()> refreshGroupModel;
    std::function<void()> refreshDialogModel;
    std::function<void(int)> selectDialogByUid;
    std::function<void(qint64)> notifyGroupCreated;
    std::function<void(const QString&)> setCurrentChatPeerIcon;
    std::function<void()> refreshGroupList;
};

} // namespace memochat::group

#endif // GROUPMANAGEMENTEFFECTPORTS_H
