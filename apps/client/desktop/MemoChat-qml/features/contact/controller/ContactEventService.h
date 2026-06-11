#ifndef CONTACTEVENTSERVICE_H
#define CONTACTEVENTSERVICE_H

#include "global.h"
#include "userdata.h"

#include <QString>
#include <functional>
#include <memory>

class ContactController;

struct ContactEventDependencies
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

class ContactEventService
{
public:
    static void handleAddAuthFriend(ContactController& contact,
                                    std::shared_ptr<AuthInfo> authInfo,
                                    const ContactEventDependencies& dependencies);
    static void handleAuthRsp(ContactController& contact,
                              std::shared_ptr<AuthRsp> authRsp,
                              const ContactEventDependencies& dependencies);
    static void handleDeleteFriendRsp(ContactController& contact,
                                      int error,
                                      int friendUid,
                                      const ContactEventDependencies& dependencies);
    static void handleUserSearch(ContactController& contact,
                                 std::shared_ptr<SearchInfo> searchInfo,
                                 const ContactEventDependencies& dependencies);
    static void handleFriendApply(ContactController& contact,
                                  std::shared_ptr<AddFriendApply> applyInfo,
                                  const ContactEventDependencies& dependencies);
};

#endif // CONTACTEVENTSERVICE_H
