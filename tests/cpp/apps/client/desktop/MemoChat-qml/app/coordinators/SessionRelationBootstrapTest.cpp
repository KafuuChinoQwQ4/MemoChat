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
    int setContactsFromSnapshotCalls = 0;
    int refreshContactLoadMoreStateCalls = 0;
    int setContactsReadyCalls = 0;
    int refreshApplySnapshotCalls = 0;
    int setChatListInitializedCalls = 0;
    int ensureChatListInitializedCalls = 0;
    int friendListSnapshotCalls = 0;
    int upsertChatListFriendCalls = 0;
    int refreshDialogModelIncrementalCalls = 0;
    int flushIncomingMessageRouterCalls = 0;
    std::vector<std::shared_ptr<FriendInfo>> friendSnapshotResult;
    int lastSetContactsFromSnapshotSize = 0;

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
        p.setContactsFromSnapshot = [this](const std::vector<std::shared_ptr<FriendInfo>>& contacts)
        {
            ++setContactsFromSnapshotCalls;
            lastSetContactsFromSnapshotSize = static_cast<int>(contacts.size());
        };
        p.refreshContactProfiles = [this]()
        {
            ++refreshContactProfilesCalls;
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

    SessionRelationBootstrap bootstrap(harness.port());
    bootstrap.onRelationBootstrapUpdated();

    EXPECT_EQ(harness.refreshContactProfilesCalls, 1);
    EXPECT_EQ(harness.friendListSnapshotCalls, 1);
    EXPECT_EQ(harness.setContactsFromSnapshotCalls, 1);
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

    SessionRelationBootstrap bootstrap(harness.port());
    bootstrap.onRelationBootstrapUpdated();

    EXPECT_EQ(harness.refreshContactProfilesCalls, 1);
    EXPECT_EQ(harness.setChatListInitializedCalls, 1);
    EXPECT_EQ(harness.ensureChatListInitializedCalls, 1);
    EXPECT_EQ(harness.friendListSnapshotCalls, 1);
    EXPECT_EQ(harness.upsertChatListFriendCalls, 2);
    EXPECT_EQ(harness.setContactsFromSnapshotCalls, 1);
    EXPECT_EQ(harness.lastSetContactsFromSnapshotSize, 2);
    EXPECT_EQ(harness.refreshDialogModelIncrementalCalls, 1);
    EXPECT_EQ(harness.flushIncomingMessageRouterCalls, 1);
}

TEST(SessionRelationBootstrapTest, ContactModelUsesFullFriendSnapshotInsteadOfNextPage)
{
    RelationBootstrapHarness harness;
    harness.snapshot.isChatPage = false;
    harness.snapshot.contactsReady = true;
    harness.friendSnapshotResult = {makeFriend(401), makeFriend(402), makeFriend(403)};

    SessionRelationBootstrap bootstrap(harness.port());
    bootstrap.onRelationBootstrapUpdated();

    EXPECT_EQ(harness.friendListSnapshotCalls, 1);
    EXPECT_EQ(harness.setContactsFromSnapshotCalls, 1);
    EXPECT_EQ(harness.lastSetContactsFromSnapshotSize, 3);
    EXPECT_EQ(harness.setContactsReadyCalls, 1);
}
