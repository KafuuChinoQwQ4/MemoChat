#pragma once

#include "ContactEventService.h"

#include <functional>
#include <memory>

namespace memochat::app
{

struct ContactEventActions
{
    std::function<void(std::shared_ptr<AuthInfo>)> addAuthFriendToStore;
    std::function<void(std::shared_ptr<AuthRsp>)> addAuthRspToStore;
    std::function<void(int)> removeFriendFromStore;
    std::function<std::shared_ptr<UserInfo>()> currentUser;
    std::function<bool(int)> isFriend;
    std::function<bool(int)> alreadyApplied;
    std::function<void(int)> markApplyApprovedInStore;
    std::function<void(std::shared_ptr<ApplyInfo>)> addApplyToStore;

    std::function<void(std::shared_ptr<AuthInfo>)> upsertChatContactFromAuthInfo;
    std::function<void(std::shared_ptr<AuthRsp>)> upsertChatContactFromAuthRsp;
    std::function<void(int)> removeChatContact;
    std::function<void(int)> openPrivateChat;
    std::function<void()> switchToChatTab;
    std::function<void()> refreshDialogModel;
    std::function<void()> refreshChatLoadMoreState;
    std::function<void()> refreshContactLoadMoreState;
};

ContactEventDependencies makeContactEventDependencies(ContactEventActions actions);

} // namespace memochat::app
