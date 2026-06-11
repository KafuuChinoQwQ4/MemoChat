#include "GroupManagementEffectPortFactory.h"

#include <utility>

namespace memochat::app
{

memochat::group::GroupManagementEventEffectPort
makeGroupManagementEventEffectPort(GroupManagementEventEffectActions actions)
{
    memochat::group::GroupManagementEventEffectPort port;
    port.removeGroup = std::move(actions.removeGroup);
    port.clearGroupConversation = std::move(actions.clearGroupConversation);
    port.refreshGroupModel = std::move(actions.refreshGroupModel);
    port.refreshDialogModel = std::move(actions.refreshDialogModel);
    port.requestDialogList = std::move(actions.requestDialogList);
    port.selectCurrentGroup = std::move(actions.selectCurrentGroup);
    port.clearCurrentGroup = std::move(actions.clearCurrentGroup);
    port.setCurrentChatPeerIcon = std::move(actions.setCurrentChatPeerIcon);
    port.openGroupConversation = std::move(actions.openGroupConversation);
    port.syncCurrentDialogDraft = std::move(actions.syncCurrentDialogDraft);
    port.loadCurrentChatMessages = std::move(actions.loadCurrentChatMessages);
    port.clearMessageModel = std::move(actions.clearMessageModel);
    port.resetCurrentChatProjection = std::move(actions.resetCurrentChatProjection);
    return port;
}

memochat::group::GroupManagementResponseEffectPort
makeGroupManagementResponseEffectPort(GroupManagementResponseEffectActions actions)
{
    memochat::group::GroupManagementResponseEffectPort port;
    port.removeGroup = std::move(actions.removeGroup);
    port.clearGroupConversation = std::move(actions.clearGroupConversation);
    port.refreshGroupModel = std::move(actions.refreshGroupModel);
    port.refreshDialogModel = std::move(actions.refreshDialogModel);
    port.selectDialogByUid = std::move(actions.selectDialogByUid);
    port.setCurrentChatPeerIcon = std::move(actions.setCurrentChatPeerIcon);
    return port;
}

} // namespace memochat::app
