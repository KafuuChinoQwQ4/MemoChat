#include "AppCoordinators.h"

#include "userdata.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace
{
std::shared_ptr<FriendInfo> makeFriend(int uid, const QString& userId = QStringLiteral("u123456789"))
{
    const QString name = QStringLiteral("name");
    const QString nick = QStringLiteral("nick");
    const QString icon = QStringLiteral("icon.png");
    const QString desc = QStringLiteral("desc");
    const QString back = QStringLiteral("back");
    return std::make_shared<FriendInfo>(uid, name, nick, icon, 1, desc, back, QString(), userId);
}

class RelationBootstrapHarness
{
public:
    RelationBootstrapSnapshot snapshot;
    int refreshContactProfilesCalls = 0;
    int nextContactPageCalls = 0;
    int setContactsCalls = 0;
    int upsertContactCalls = 0;
    int markContactPageLoadedCalls = 0;
    int refreshContactLoadMoreStateCalls = 0;
    int setContactsReadyCalls = 0;
    int refreshApplySnapshotCalls = 0;
    int setChatListInitializedCalls = 0;
    int ensureChatListInitializedCalls = 0;
    int friendListSnapshotCalls = 0;
    int upsertChatListFriendCalls = 0;
    int refreshDialogModelIncrementalCalls = 0;
    int flushIncomingMessageRouterCalls = 0;
    std::vector<std::shared_ptr<FriendInfo>> nextContactPageResult;
    std::vector<std::shared_ptr<FriendInfo>> friendSnapshotResult;

    RelationBootstrapPort port()
    {
        RelationBootstrapPort p;
        p.snapshot = [this]()
        {
            return snapshot;
        };
        p.sendRelationBootstrap = [](const QByteArray&) {};
        p.setChatListInitialized = [this](bool)
        {
            ++setChatListInitializedCalls;
        };
        p.ensureChatListInitialized = [this]()
        {
            ++ensureChatListInitializedCalls;
        };
        p.friendListSnapshot = [this]()
        {
            ++friendListSnapshotCalls;
            return friendSnapshotResult;
        };
        p.upsertChatListFriend = [this](const std::shared_ptr<FriendInfo>&)
        {
            ++upsertChatListFriendCalls;
        };
        p.nextContactPage = [this]()
        {
            ++nextContactPageCalls;
            return nextContactPageResult;
        };
        p.setContacts = [this](const std::vector<std::shared_ptr<FriendInfo>>&)
        {
            ++setContactsCalls;
        };
        p.upsertContact = [this](const std::shared_ptr<FriendInfo>&)
        {
            ++upsertContactCalls;
        };
        p.refreshContactProfiles = [this]()
        {
            ++refreshContactProfilesCalls;
        };
        p.markContactPageLoaded = [this]()
        {
            ++markContactPageLoadedCalls;
        };
        p.refreshContactLoadMoreState = [this]()
        {
            ++refreshContactLoadMoreStateCalls;
        };
        p.setContactsReady = [this](bool)
        {
            ++setContactsReadyCalls;
        };
        p.refreshApplySnapshot = [this]()
        {
            ++refreshApplySnapshotCalls;
        };
        p.refreshDialogModelIncremental = [this]()
        {
            ++refreshDialogModelIncrementalCalls;
        };
        p.flushIncomingMessageRouter = [this]()
        {
            ++flushIncomingMessageRouterCalls;
        };
        p.friendById = [](int)
        {
            return nullptr;
        };
        p.setCurrentChatPeerName = [](const QString&) {};
        p.setCurrentChatPeerIcon = [](const QString&) {};
        return p;
    }
};
} // namespace

TEST(SessionRelationBootstrapTest, NonChatPageRefreshesContactProfilesBeforeReturning)
{
    RelationBootstrapHarness harness;
    harness.snapshot.isChatPage = false;
    harness.snapshot.contactsReady = true;
    harness.nextContactPageResult = {};

    SessionRelationBootstrap bootstrap(harness.port());
    bootstrap.onRelationBootstrapUpdated();

    EXPECT_EQ(harness.refreshContactProfilesCalls, 1);
    EXPECT_EQ(harness.nextContactPageCalls, 1);
    EXPECT_EQ(harness.setContactsCalls, 0);
    EXPECT_EQ(harness.upsertContactCalls, 0);
    EXPECT_EQ(harness.markContactPageLoadedCalls, 1);
    EXPECT_EQ(harness.refreshContactLoadMoreStateCalls, 1);
    EXPECT_EQ(harness.setContactsReadyCalls, 1);
    EXPECT_EQ(harness.refreshApplySnapshotCalls, 1);
    EXPECT_EQ(harness.setChatListInitializedCalls, 0);
    EXPECT_EQ(harness.ensureChatListInitializedCalls, 0);
    EXPECT_EQ(harness.refreshDialogModelIncrementalCalls, 0);
    EXPECT_EQ(harness.flushIncomingMessageRouterCalls, 0);
}

TEST(SessionRelationBootstrapTest, ChatPageStillRefreshesChatAndContactModels)
{
    RelationBootstrapHarness harness;
    harness.snapshot.isChatPage = true;
    harness.snapshot.contactsReady = true;
    harness.friendSnapshotResult = {makeFriend(301), makeFriend(302)};
    harness.nextContactPageResult = {makeFriend(303)};

    SessionRelationBootstrap bootstrap(harness.port());
    bootstrap.onRelationBootstrapUpdated();

    EXPECT_EQ(harness.refreshContactProfilesCalls, 1);
    EXPECT_EQ(harness.setChatListInitializedCalls, 1);
    EXPECT_EQ(harness.ensureChatListInitializedCalls, 1);
    EXPECT_EQ(harness.friendListSnapshotCalls, 1);
    EXPECT_EQ(harness.upsertChatListFriendCalls, 2);
    EXPECT_EQ(harness.nextContactPageCalls, 1);
    EXPECT_EQ(harness.upsertContactCalls, 1);
    EXPECT_EQ(harness.setContactsCalls, 0);
    EXPECT_EQ(harness.refreshDialogModelIncrementalCalls, 1);
    EXPECT_EQ(harness.flushIncomingMessageRouterCalls, 1);
}
