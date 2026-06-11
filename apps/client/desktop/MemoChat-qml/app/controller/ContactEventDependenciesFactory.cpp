#include "ContactEventDependenciesFactory.h"

#include <utility>

namespace memochat::app
{

ContactEventDependencies makeContactEventDependencies(ContactEventActions actions)
{
    ContactEventDependencies dependencies;
    dependencies.addAuthFriendToStore = std::move(actions.addAuthFriendToStore);
    dependencies.addAuthRspToStore = std::move(actions.addAuthRspToStore);
    dependencies.removeFriendFromStore = std::move(actions.removeFriendFromStore);
    dependencies.currentUser = std::move(actions.currentUser);
    dependencies.isFriend = std::move(actions.isFriend);
    dependencies.alreadyApplied = std::move(actions.alreadyApplied);
    dependencies.markApplyApprovedInStore = std::move(actions.markApplyApprovedInStore);
    dependencies.addApplyToStore = std::move(actions.addApplyToStore);
    dependencies.upsertChatContactFromAuthInfo = std::move(actions.upsertChatContactFromAuthInfo);
    dependencies.upsertChatContactFromAuthRsp = std::move(actions.upsertChatContactFromAuthRsp);
    dependencies.removeChatContact = std::move(actions.removeChatContact);
    dependencies.openPrivateChat = std::move(actions.openPrivateChat);
    dependencies.switchToChatTab = std::move(actions.switchToChatTab);
    dependencies.refreshDialogModel = std::move(actions.refreshDialogModel);
    dependencies.refreshChatLoadMoreState = std::move(actions.refreshChatLoadMoreState);
    dependencies.refreshContactLoadMoreState = std::move(actions.refreshContactLoadMoreState);
    return dependencies;
}

} // namespace memochat::app
