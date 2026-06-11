#include "GroupManagementEffectApplier.h"

#include <utility>

namespace memochat::group
{
namespace
{
template <typename Callback, typename... Args> void callIfPresent(const Callback& callback, Args&&... args)
{
    if (callback)
    {
        callback(std::forward<Args>(args)...);
    }
}

template <typename Callback> void callForPositiveGroupIds(const std::vector<qint64>& groupIds, const Callback& callback)
{
    for (const qint64 groupId : groupIds)
    {
        if (groupId > 0)
        {
            callIfPresent(callback, groupId);
        }
    }
}
} // namespace

void GroupManagementEffectApplier::apply(const GroupManagementEffect& effect,
                                         const GroupManagementEventEffectPort& port)
{
    if (!effect.handled)
    {
        return;
    }

    if (effect.hasStatus)
    {
        callIfPresent(port.setStatus, effect.statusText, effect.statusIsError);
    }
    callForPositiveGroupIds(effect.removeGroupIds, port.removeGroup);
    callForPositiveGroupIds(effect.clearGroupConversationIds, port.clearGroupConversation);
    if (effect.refreshGroupModel)
    {
        callIfPresent(port.refreshGroupModel);
    }
    if (effect.refreshDialogModel)
    {
        callIfPresent(port.refreshDialogModel);
    }
    if (effect.requestDialogList)
    {
        callIfPresent(port.requestDialogList);
    }
    if (effect.selectCurrentGroup)
    {
        callIfPresent(port.selectCurrentGroup,
                      effect.selectedGroupId,
                      effect.selectedGroupName,
                      effect.selectedGroupCode);
    }
    if (effect.updateCurrentChatPeerIcon)
    {
        callIfPresent(port.setCurrentChatPeerIcon, effect.currentChatPeerIcon);
    }
    if (effect.openGroupConversation)
    {
        callIfPresent(port.openGroupConversation, effect.openGroupId, effect.openGroupResetHistory);
    }
    if (effect.syncCurrentDialogDraft)
    {
        callIfPresent(port.syncCurrentDialogDraft);
    }
    if (effect.clearCurrentGroup)
    {
        callIfPresent(port.clearCurrentGroup);
    }
    if (effect.loadCurrentChatMessages)
    {
        callIfPresent(port.loadCurrentChatMessages);
    }
    if (effect.clearMessageModel)
    {
        callIfPresent(port.clearMessageModel);
    }
    if (effect.resetCurrentChatProjection)
    {
        callIfPresent(port.resetCurrentChatProjection);
    }
    if (effect.requestGroupList)
    {
        callIfPresent(port.refreshGroupList);
    }
}

void GroupManagementEffectApplier::apply(const GroupManagementResponseEffect& effect,
                                         const GroupManagementResponseEffectPort& port)
{
    if (!effect.handled)
    {
        return;
    }

    if (effect.hasStatus)
    {
        callIfPresent(port.setStatus, effect.statusText, effect.statusIsError);
    }
    callForPositiveGroupIds(effect.removeGroupIds, port.removeGroup);
    callForPositiveGroupIds(effect.clearGroupConversationIds, port.clearGroupConversation);
    if (effect.refreshGroupModel)
    {
        callIfPresent(port.refreshGroupModel);
    }
    if (effect.refreshDialogModel)
    {
        callIfPresent(port.refreshDialogModel);
    }
    if (effect.selectDialogUid)
    {
        callIfPresent(port.selectDialogByUid, effect.dialogUid);
    }
    if (effect.emitGroupCreatedId)
    {
        callIfPresent(port.notifyGroupCreated, effect.createdGroupId);
    }
    if (effect.updateCurrentChatPeerIcon)
    {
        callIfPresent(port.setCurrentChatPeerIcon, effect.currentChatPeerIcon);
    }
    if (effect.refreshGroupList)
    {
        callIfPresent(port.refreshGroupList);
    }
}

} // namespace memochat::group
