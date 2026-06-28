#include "AppCoordinators.h"

#include "userdata.h"

#include <gtest/gtest.h>

#include <memory>
#include <utility>

namespace
{
class ChatEntryHarness
{
public:
    PostLoginBootstrapSnapshot snapshot;
    int delayedCalls = 0;
    int setBootstrapStartedCalls = 0;
    int ensureContactsCalls = 0;
    int requestRelationCalls = 0;
    int bootstrapDialogsCalls = 0;
    int ensureApplyCalls = 0;
    int heartbeatStartCalls = 0;
    int heartbeatSendCalls = 0;
    int resetRuntimeCalls = 0;
    int setChatLoginCompletedCalls = 0;
    bool lastBootstrapStarted = false;
    bool lastChatLoginCompleted = false;

    PostLoginBootstrapPort port()
    {
        PostLoginBootstrapPort p;
        p.snapshot = [this]()
        {
            return snapshot;
        };
        p.currentUserInfo = []()
        {
            return std::shared_ptr<UserInfo>{};
        };
        p.setIgnoreNextLoginDisconnect = [](bool) {};
        p.stopLoginTimeoutTimer = []() {};
        p.resetReconnectState = []() {};
        p.resetHeartbeatTracking = []() {};
        p.setLastHeartbeatAckMs = [](qint64) {};
        p.switchToChatPage = [this]()
        {
            snapshot.isChatPage = true;
        };
        p.resetChatEntryRuntime = [this]()
        {
            ++resetRuntimeCalls;
            snapshot.chatLoginCompleted = false;
        };
        p.applyLoggedInUserSession = [](const std::shared_ptr<UserInfo>&, const QString&) {};
        p.clearMissingUserDialogState = []() {};
        p.setPostLoginBootstrapStarted = [this](bool started)
        {
            ++setBootstrapStartedCalls;
            lastBootstrapStarted = started;
            snapshot.postLoginBootstrapStarted = started;
        };
        p.setChatLoginCompleted = [this](bool completed)
        {
            ++setChatLoginCompletedCalls;
            lastChatLoginCompleted = completed;
            snapshot.chatLoginCompleted = completed;
        };
        p.runDelayed = [this](int, std::function<void()> callback)
        {
            ++delayedCalls;
            callback();
        };
        p.openCachesAndDraftsForUser = [](int) {};
        p.bootstrapDialogs = [this]()
        {
            ++bootstrapDialogsCalls;
        };
        p.ensureContactsInitialized = [this]()
        {
            ++ensureContactsCalls;
        };
        p.ensureApplyInitialized = [this]()
        {
            ++ensureApplyCalls;
        };
        p.requestRelationBootstrap = [this]()
        {
            ++requestRelationCalls;
        };
        p.startHeartbeatTimer = [this](int)
        {
            ++heartbeatStartCalls;
        };
        p.sendHeartbeatNow = [this]()
        {
            ++heartbeatSendCalls;
        };
        p.ensureChatListInitialized = []() {};
        return p;
    }
};
} // namespace

TEST(SessionChatEntryCoordinatorTest, BeginPostLoginBootstrapWaitsForChatLoginSuccess)
{
    ChatEntryHarness harness;
    harness.snapshot.isChatPage = true;
    harness.snapshot.chatTransportReady = true;
    harness.snapshot.chatLoginCompleted = false;

    SessionChatEntryCoordinator coordinator(harness.port());
    coordinator.beginPostLoginBootstrap();

    EXPECT_EQ(harness.setBootstrapStartedCalls, 0);
    EXPECT_EQ(harness.delayedCalls, 0);
    EXPECT_EQ(harness.ensureContactsCalls, 0);
    EXPECT_EQ(harness.requestRelationCalls, 0);
}

TEST(SessionChatEntryCoordinatorTest, ChatLoginSuccessStartsContactAndRelationBootstrap)
{
    ChatEntryHarness harness;
    harness.snapshot.isChatPage = true;
    harness.snapshot.chatTransportReady = true;
    harness.snapshot.chatLoginCompleted = false;

    SessionChatEntryCoordinator coordinator(harness.port());
    coordinator.onSwitchToChat();

    EXPECT_EQ(harness.resetRuntimeCalls, 1);
    EXPECT_EQ(harness.setChatLoginCompletedCalls, 1);
    EXPECT_TRUE(harness.lastChatLoginCompleted);
    EXPECT_EQ(harness.setBootstrapStartedCalls, 2);
    EXPECT_TRUE(harness.lastBootstrapStarted);
    EXPECT_EQ(harness.delayedCalls, 2);
    EXPECT_EQ(harness.bootstrapDialogsCalls, 1);
    EXPECT_EQ(harness.ensureContactsCalls, 1);
    EXPECT_EQ(harness.ensureApplyCalls, 1);
    EXPECT_EQ(harness.requestRelationCalls, 1);
    EXPECT_EQ(harness.heartbeatStartCalls, 1);
    EXPECT_EQ(harness.heartbeatSendCalls, 1);
}
