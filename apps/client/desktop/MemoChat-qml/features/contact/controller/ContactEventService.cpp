#include "ContactEventService.h"

#include "ContactController.h"

#include <utility>

namespace
{
void refreshContactProjections(const ContactEventDependencies& dependencies)
{
    if (dependencies.refreshDialogModel)
    {
        dependencies.refreshDialogModel();
    }
    if (dependencies.refreshChatLoadMoreState)
    {
        dependencies.refreshChatLoadMoreState();
    }
    if (dependencies.refreshContactLoadMoreState)
    {
        dependencies.refreshContactLoadMoreState();
    }
}

void selectCurrentContact(ContactController& contact,
                          int uid,
                          const QString& name,
                          const QString& nick,
                          const QString& icon,
                          int sex,
                          const QString& userId)
{
    contact.setCurrentContact(uid, name, nick, icon, nick, sex, userId);
}
} // namespace

void ContactEventService::handleAddAuthFriend(ContactController& contact,
                                              std::shared_ptr<AuthInfo> authInfo,
                                              const ContactEventDependencies& dependencies)
{
    if (authInfo && dependencies.addAuthFriendToStore)
    {
        dependencies.addAuthFriendToStore(authInfo);
    }
    if (dependencies.upsertChatContactFromAuthInfo)
    {
        dependencies.upsertChatContactFromAuthInfo(authInfo);
    }
    contact.upsertContact(authInfo);
    refreshContactProjections(dependencies);

    if (!authInfo)
    {
        return;
    }

    if (dependencies.markApplyApprovedInStore)
    {
        dependencies.markApplyApprovedInStore(authInfo->_uid);
    }
    contact.markApplyApproved(authInfo->_uid);
    if (contact.currentContactUid() == authInfo->_uid)
    {
        selectCurrentContact(contact,
                             authInfo->_uid,
                             authInfo->_name,
                             authInfo->_nick,
                             authInfo->_icon,
                             authInfo->_sex,
                             authInfo->_user_id);
    }
}

void ContactEventService::handleAuthRsp(ContactController& contact,
                                        std::shared_ptr<AuthRsp> authRsp,
                                        const ContactEventDependencies& dependencies)
{
    if (authRsp && dependencies.addAuthRspToStore)
    {
        dependencies.addAuthRspToStore(authRsp);
    }
    if (dependencies.upsertChatContactFromAuthRsp)
    {
        dependencies.upsertChatContactFromAuthRsp(authRsp);
    }
    contact.upsertContact(authRsp);
    refreshContactProjections(dependencies);

    if (!authRsp)
    {
        return;
    }

    if (dependencies.markApplyApprovedInStore)
    {
        dependencies.markApplyApprovedInStore(authRsp->_uid);
    }
    contact.markApplyApproved(authRsp->_uid);
    contact.setAuthStatus(QStringLiteral("好友添加成功"), false);
    if (contact.currentContactUid() == authRsp->_uid)
    {
        selectCurrentContact(contact,
                             authRsp->_uid,
                             authRsp->_name,
                             authRsp->_nick,
                             authRsp->_icon,
                             authRsp->_sex,
                             authRsp->_user_id);
    }
}

void ContactEventService::handleDeleteFriendRsp(ContactController& contact,
                                                int error,
                                                int friendUid,
                                                const ContactEventDependencies& dependencies)
{
    if (error != ErrorCodes::SUCCESS || friendUid <= 0)
    {
        contact.setAuthStatus(QStringLiteral("删除联系人失败（错误码:%1）").arg(error), true);
        return;
    }

    const bool wasCurrentContact = contact.currentContactUid() == friendUid;
    if (dependencies.removeFriendFromStore)
    {
        dependencies.removeFriendFromStore(friendUid);
    }
    if (dependencies.removeChatContact)
    {
        dependencies.removeChatContact(friendUid);
    }
    contact.removeContactByUid(friendUid);

    if (wasCurrentContact)
    {
        contact.clearCurrentContact();
        contact.setContactPane(0);
    }

    if (dependencies.refreshChatLoadMoreState)
    {
        dependencies.refreshChatLoadMoreState();
    }
    if (dependencies.refreshContactLoadMoreState)
    {
        dependencies.refreshContactLoadMoreState();
    }
    contact.setAuthStatus(QStringLiteral("联系人已删除"), false);
}

void ContactEventService::handleUserSearch(ContactController& contact,
                                           std::shared_ptr<SearchInfo> searchInfo,
                                           const ContactEventDependencies& dependencies)
{
    contact.setSearchPending(false);

    if (!searchInfo)
    {
        contact.clearSearchResultOnly();
        contact.setSearchStatus(QStringLiteral("未找到该用户"), true);
        return;
    }

    const auto selfInfo = dependencies.currentUser ? dependencies.currentUser() : nullptr;
    if (!selfInfo)
    {
        contact.clearSearchResultOnly();
        contact.setSearchStatus(QStringLiteral("用户状态异常，请重新登录"), true);
        return;
    }

    if (searchInfo->_uid == selfInfo->_uid)
    {
        contact.clearSearchResultOnly();
        contact.setSearchStatus(QStringLiteral("不能搜索自己"), true);
        return;
    }

    const bool isFriend = dependencies.isFriend ? dependencies.isFriend(searchInfo->_uid) : false;
    if (isFriend)
    {
        contact.clearSearchResultOnly();
        contact.setSearchStatus(QStringLiteral("已是好友，已切换到会话"), false);
        if (dependencies.openPrivateChat)
        {
            dependencies.openPrivateChat(searchInfo->_uid);
        }
        if (dependencies.switchToChatTab)
        {
            dependencies.switchToChatTab();
        }
        return;
    }

    contact.setSearchResult(searchInfo);
    contact.setSearchStatus(QStringLiteral("已找到用户"), false);
}

void ContactEventService::handleFriendApply(ContactController& contact,
                                            std::shared_ptr<AddFriendApply> applyInfo,
                                            const ContactEventDependencies& dependencies)
{
    if (!applyInfo)
    {
        return;
    }

    const bool alreadyApplied = dependencies.alreadyApplied ? dependencies.alreadyApplied(applyInfo->_from_uid) : false;
    if (alreadyApplied)
    {
        return;
    }

    auto apply = std::make_shared<ApplyInfo>(applyInfo);
    if (dependencies.addApplyToStore)
    {
        dependencies.addApplyToStore(apply);
    }
    contact.upsertApply(apply);
    contact.setAuthStatus(QStringLiteral("收到来自 %1 的好友申请").arg(applyInfo->_name), false);
}
