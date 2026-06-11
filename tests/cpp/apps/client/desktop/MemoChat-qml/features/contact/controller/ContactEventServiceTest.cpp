#include "ApplyRequestModel.h"
#include "ContactController.h"
#include "ContactEventService.h"
#include "FriendListModel.h"
#include "SearchResultModel.h"
#include "userdata.h"

#include <gtest/gtest.h>

#include <QString>
#include <memory>

namespace
{
std::shared_ptr<AuthRsp> makeAuthRsp(int uid, const QString& name)
{
    return std::make_shared<AuthRsp>(
        uid,
        name,
        name + QStringLiteral("Nick"), QStringLiteral("res/head.png"), 1, QStringLiteral("u123456789"));
}

std::shared_ptr<AuthInfo> makeAuthInfo(int uid, const QString& name)
{
    return std::make_shared<AuthInfo>(
        uid,
        name,
        name + QStringLiteral("Nick"), QStringLiteral("res/head.png"), 1, QStringLiteral("u123456789"));
}

std::shared_ptr<FriendInfo> makeFriend(int uid, const QString& name)
{
    auto friendInfo = std::make_shared<FriendInfo>(
        uid,
        name,
        name + QStringLiteral("Nick"),
                              QStringLiteral("res/head.png"),
                                             1,
                                             QStringLiteral("desc"),
                                                            QStringLiteral("back"),
                                                                           QStringLiteral("last"),
                                                                                          QStringLiteral("u123456789"));
    friendInfo->_dialog_type = QStringLiteral("private");
    return friendInfo;
}

std::shared_ptr<ApplyInfo> makeApply(int uid, const QString& name, int status = 0)
{
    return std::make_shared<ApplyInfo>(
        uid,
        name,
        QStringLiteral("desc"),
                       QStringLiteral("res/head.png"),
                                      name + QStringLiteral("Nick"), 1, status, QStringLiteral("u123456789"));
}

std::shared_ptr<SearchInfo> makeSearchInfo(int uid, const QString& name)
{
    return std::make_shared<SearchInfo>(
        uid,
        name,
        name + QStringLiteral("Nick"),
                              QStringLiteral("desc"), 1, QStringLiteral("res/head.png"), QStringLiteral("u123456789"));
}

std::shared_ptr<AddFriendApply> makeFriendApply(int uid, const QString& name)
{
    return std::make_shared<AddFriendApply>(
        uid,
        name,
        QStringLiteral("desc"),
                       QStringLiteral("res/head.png"), name + QStringLiteral("Nick"), 1, QStringLiteral("u123456789"));
}

QVariantMap row(FriendListModel* model, int index)
{
    return model->get(index);
}

QVariantMap row(ApplyRequestModel* model, int index)
{
    return model->get(index);
}
} // namespace

TEST(ContactEventServiceTest, AuthResponseUpdatesContactStateAndUsesProjectionAdapters)
{
    ContactController contact(nullptr);
    contact.setApplies({makeApply(701, QStringLiteral("Alice"))});
    contact.setCurrentContact(
        701,
        QStringLiteral("Old"),
                       QStringLiteral("OldNick"),
                                      QStringLiteral("qrc:/res/head_1.png"),
                                                     QStringLiteral("old back"), 0, QStringLiteral("old"));

    bool stored = false;
    bool chatUpserted = false;
    int markedApplyUid = 0;
    int dialogRefreshes = 0;
    int chatRefreshes = 0;
    int contactRefreshes = 0;

    ContactEventDependencies dependencies;
    dependencies.addAuthRspToStore = [&](std::shared_ptr<AuthRsp> authRsp)
    {
        stored = authRsp && authRsp->_uid == 701;
    };
    dependencies.upsertChatContactFromAuthRsp = [&](std::shared_ptr<AuthRsp> authRsp)
    {
        chatUpserted = authRsp && authRsp->_uid == 701;
    };
    dependencies.markApplyApprovedInStore = [&](int uid)
    {
        markedApplyUid = uid;
    };
    dependencies.refreshDialogModel = [&]()
    {
        ++dialogRefreshes;
    };
    dependencies.refreshChatLoadMoreState = [&]()
    {
        ++chatRefreshes;
    };
    dependencies.refreshContactLoadMoreState = [&]()
    {
        ++contactRefreshes;
    };

    ContactEventService::handleAuthRsp(contact, makeAuthRsp(701, QStringLiteral("Alice")), dependencies);

    EXPECT_TRUE(stored);
    EXPECT_TRUE(chatUpserted);
    EXPECT_EQ(markedApplyUid, 701);
    EXPECT_EQ(dialogRefreshes, 1);
    EXPECT_EQ(chatRefreshes, 1);
    EXPECT_EQ(contactRefreshes, 1);
    EXPECT_EQ(contact.authStatusText(), QStringLiteral("好友添加成功"));
    EXPECT_FALSE(contact.authStatusError());
    ASSERT_EQ(contact.contactListModel()->rowCount(), 1);
    EXPECT_EQ(row(contact.contactListModel(), 0).value(QStringLiteral("uid")).toInt(), 701);
    EXPECT_EQ(contact.currentContactName(), QStringLiteral("Alice"));

    const int applyIndex = contact.applyRequestModel()->indexOfUid(701);
    ASSERT_GE(applyIndex, 0);
    EXPECT_TRUE(row(contact.applyRequestModel(), applyIndex).value(QStringLiteral("approved")).toBool());
}

TEST(ContactEventServiceTest, AddAuthFriendDoesNotSetSuccessStatusButApprovesApply)
{
    ContactController contact(nullptr);
    contact.setApplies({makeApply(702, QStringLiteral("Bob"))});

    bool stored = false;
    bool chatUpserted = false;

    ContactEventDependencies dependencies;
    dependencies.addAuthFriendToStore = [&](std::shared_ptr<AuthInfo> authInfo)
    {
        stored = authInfo && authInfo->_uid == 702;
    };
    dependencies.upsertChatContactFromAuthInfo = [&](std::shared_ptr<AuthInfo> authInfo)
    {
        chatUpserted = authInfo && authInfo->_uid == 702;
    };

    ContactEventService::handleAddAuthFriend(contact, makeAuthInfo(702, QStringLiteral("Bob")), dependencies);

    EXPECT_TRUE(stored);
    EXPECT_TRUE(chatUpserted);
    EXPECT_TRUE(contact.authStatusText().isEmpty());
    ASSERT_EQ(contact.contactListModel()->rowCount(), 1);
    EXPECT_EQ(row(contact.contactListModel(), 0).value(QStringLiteral("uid")).toInt(), 702);

    const int applyIndex = contact.applyRequestModel()->indexOfUid(702);
    ASSERT_GE(applyIndex, 0);
    EXPECT_TRUE(row(contact.applyRequestModel(), applyIndex).value(QStringLiteral("approved")).toBool());
}

TEST(ContactEventServiceTest, DeleteFriendResponseRemovesContactAndCallsAdapters)
{
    ContactController contact(nullptr);
    contact.setContacts({makeFriend(703, QStringLiteral("Cora"))});
    contact.setCurrentContact(
        703,
        QStringLiteral("Cora"),
                       QStringLiteral("CoraNick"),
                                      QStringLiteral("qrc:/res/head_1.png"),
                                                     QStringLiteral("back"), 1, QStringLiteral("u123456789"));
    contact.setContactPane(1);

    int removedFromStore = 0;
    int removedFromChat = 0;

    ContactEventDependencies dependencies;
    dependencies.removeFriendFromStore = [&](int uid)
    {
        removedFromStore = uid;
    };
    dependencies.removeChatContact = [&](int uid)
    {
        removedFromChat = uid;
    };

    ContactEventService::handleDeleteFriendRsp(contact, ErrorCodes::SUCCESS, 703, dependencies);

    EXPECT_EQ(removedFromStore, 703);
    EXPECT_EQ(removedFromChat, 703);
    EXPECT_EQ(contact.contactListModel()->rowCount(), 0);
    EXPECT_FALSE(contact.hasCurrentContact());
    EXPECT_EQ(contact.contactPane(), 0);
    EXPECT_EQ(contact.authStatusText(), QStringLiteral("联系人已删除"));
    EXPECT_FALSE(contact.authStatusError());
}

TEST(ContactEventServiceTest, DeleteFriendResponseReportsErrorWithoutMutation)
{
    ContactController contact(nullptr);
    contact.setContacts({makeFriend(704, QStringLiteral("Dion"))});

    bool removed = false;
    ContactEventDependencies dependencies;
    dependencies.removeFriendFromStore = [&](int)
    {
        removed = true;
    };

    ContactEventService::handleDeleteFriendRsp(contact, ErrorCodes::ERR_NETWORK, 704, dependencies);

    EXPECT_FALSE(removed);
    EXPECT_EQ(contact.contactListModel()->rowCount(), 1);
    EXPECT_TRUE(contact.authStatusError());
    EXPECT_EQ(contact.authStatusText(), QStringLiteral("删除联系人失败（错误码:%1）").arg(ErrorCodes::ERR_NETWORK));
}

TEST(ContactEventServiceTest, SearchExistingFriendClearsResultAndRequestsChatSwitch)
{
    ContactController contact(nullptr);
    contact.setSearchPending(true);
    contact.setSearchResult(makeSearchInfo(705, QStringLiteral("Eli")));

    int openedChatUid = 0;
    bool switchedTab = false;

    ContactEventDependencies dependencies;
    dependencies.currentUser = []()
    {
        return std::make_shared<UserInfo>(1, QStringLiteral("Self"), QStringLiteral("SelfNick"), QString(), 0);
    };
    dependencies.isFriend = [](int uid)
    {
        return uid == 705;
    };
    dependencies.openPrivateChat = [&](int uid)
    {
        openedChatUid = uid;
    };
    dependencies.switchToChatTab = [&]()
    {
        switchedTab = true;
    };

    ContactEventService::handleUserSearch(contact, makeSearchInfo(705, QStringLiteral("Eli")), dependencies);

    EXPECT_FALSE(contact.searchPending());
    EXPECT_EQ(contact.searchResultModel()->rowCount(), 0);
    EXPECT_EQ(contact.searchStatusText(), QStringLiteral("已是好友，已切换到会话"));
    EXPECT_FALSE(contact.searchStatusError());
    EXPECT_EQ(openedChatUid, 705);
    EXPECT_TRUE(switchedTab);
}

TEST(ContactEventServiceTest, SearchSelfAndMissingUserProduceFeatureOwnedStatus)
{
    ContactController contact(nullptr);

    ContactEventDependencies dependencies;
    dependencies.currentUser = []()
    {
        return std::make_shared<UserInfo>(706, QStringLiteral("Self"), QStringLiteral("SelfNick"), QString(), 0);
    };

    ContactEventService::handleUserSearch(contact, makeSearchInfo(706, QStringLiteral("Self")), dependencies);
    EXPECT_EQ(contact.searchStatusText(), QStringLiteral("不能搜索自己"));
    EXPECT_TRUE(contact.searchStatusError());
    EXPECT_EQ(contact.searchResultModel()->rowCount(), 0);

    ContactEventService::handleUserSearch(contact, nullptr, dependencies);
    EXPECT_EQ(contact.searchStatusText(), QStringLiteral("未找到该用户"));
    EXPECT_TRUE(contact.searchStatusError());
}

TEST(ContactEventServiceTest, FriendApplyAddsApplyUnlessDuplicate)
{
    ContactController contact(nullptr);

    int storedApplyUid = 0;
    ContactEventDependencies dependencies;
    dependencies.alreadyApplied = [](int uid)
    {
        return uid == 708;
    };
    dependencies.addApplyToStore = [&](std::shared_ptr<ApplyInfo> apply)
    {
        storedApplyUid = apply ? apply->_uid : 0;
    };

    ContactEventService::handleFriendApply(contact, makeFriendApply(707, QStringLiteral("Faye")), dependencies);

    EXPECT_EQ(storedApplyUid, 707);
    EXPECT_EQ(contact.applyRequestModel()->rowCount(), 1);
    EXPECT_EQ(contact.authStatusText(), QStringLiteral("收到来自 Faye 的好友申请"));

    ContactEventService::handleFriendApply(contact, makeFriendApply(708, QStringLiteral("Gail")), dependencies);
    EXPECT_EQ(contact.applyRequestModel()->rowCount(), 1);
}
