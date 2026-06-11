#include "CallController.h"
#include "CallSessionModel.h"
#include "LivekitBridge.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QString>

namespace
{
void ensureCoreApplication()
{
    if (QCoreApplication::instance())
    {
        return;
    }
    static int argc = 1;
    static char appName[] = "call_controller_gtest";
    static char* argv[] = {appName, nullptr};
    static QCoreApplication app(argc, argv);
}
} // namespace

TEST(CallControllerTest, OwnsNonNullCallSurfaceObjects)
{
    ensureCoreApplication();
    CallController controller(nullptr);

    ASSERT_NE(controller.callSession(), nullptr);
    ASSERT_NE(controller.livekitBridge(), nullptr);
    EXPECT_EQ(controller.callSession()->parent(), &controller);
    EXPECT_EQ(controller.livekitBridge()->parent(), &controller);
    EXPECT_FALSE(controller.callVisible());
    EXPECT_TRUE(controller.cameraEnabled());
}

TEST(CallControllerTest, SyncSurfaceDoesNotReplaceFeatureOwnedPointers)
{
    ensureCoreApplication();
    CallController controller(nullptr);
    CallSessionModel externalSession;
    LivekitBridge externalBridge;
    auto* ownedSession = controller.callSession();
    auto* ownedBridge = controller.livekitBridge();
    QSignalSpy spy(&controller, &CallController::callSurfaceChanged);

    controller.syncSurface(&externalSession, &externalBridge);

    EXPECT_EQ(controller.callSession(), ownedSession);
    EXPECT_EQ(controller.livekitBridge(), ownedBridge);
    EXPECT_EQ(spy.count(), 1);
}

TEST(CallControllerTest, WrapsBasicModelTransitions)
{
    ensureCoreApplication();
    CallController controller(nullptr);

    controller.startOutgoing(QStringLiteral("call-1"),
        QStringLiteral("video"),
                       QStringLiteral("Peer"), QStringLiteral("peer.png"), QStringLiteral("Dialing"), 1000, 60000);
    EXPECT_TRUE(controller.callVisible());
    EXPECT_FALSE(controller.callIncoming());
    EXPECT_FALSE(controller.callActive());
    EXPECT_EQ(controller.callId(), QStringLiteral("call-1"));
    EXPECT_EQ(controller.callType(), QStringLiteral("video"));
    EXPECT_TRUE(controller.cameraEnabled());

    controller.markAccepted(QStringLiteral("Connected"), QStringLiteral("room-a"), QStringLiteral("wss://lk"), 2000);
    EXPECT_TRUE(controller.callActive());
    EXPECT_EQ(controller.callSession()->roomName(), QStringLiteral("room-a"));
    EXPECT_EQ(controller.callSession()->livekitUrl(), QStringLiteral("wss://lk"));

    controller.setMediaStatusText(QStringLiteral("Preparing media"));
    controller.setMediaLaunchJson(QStringLiteral("{\"room\":\"room-a\"}"));
    controller.markTokenReady(QStringLiteral("Ready"));
    EXPECT_TRUE(controller.callSession()->tokenReady());
    EXPECT_EQ(controller.callSession()->mediaStatusText(), QStringLiteral("Ready"));
    EXPECT_EQ(controller.callSession()->mediaLaunchJson(), QStringLiteral("{\"room\":\"room-a\"}"));

    controller.setMuted(true);
    controller.setCameraEnabled(false);
    EXPECT_TRUE(controller.muted());
    EXPECT_FALSE(controller.cameraEnabled());

    controller.markEnded(QStringLiteral("Ended"));
    EXPECT_FALSE(controller.callActive());
    EXPECT_EQ(controller.callSession()->stateText(), QStringLiteral("Ended"));

    controller.resetCallSurface();
    EXPECT_FALSE(controller.callVisible());
    EXPECT_TRUE(controller.callId().isEmpty());
}

TEST(CallControllerTest, WrapsLivekitBridgeCommands)
{
    ensureCoreApplication();
    CallController controller(nullptr);
    QSignalSpy leaveSpy(controller.livekitBridge(), &LivekitBridge::leaveRequested);
    QSignalSpy micSpy(controller.livekitBridge(), &LivekitBridge::micToggleRequested);
    QSignalSpy cameraSpy(controller.livekitBridge(), &LivekitBridge::cameraToggleRequested);
    QSignalSpy joinSpy(controller.livekitBridge(), &LivekitBridge::joinRequested);

    controller.leaveRoom();
    controller.toggleMic();
    controller.toggleCamera();
    controller.requestJoinRoom(QStringLiteral("wss://lk"), QStringLiteral("token"), QStringLiteral("{\"m\":1}"));
    controller.livekitBridge()->setPageReady(true);

    EXPECT_EQ(leaveSpy.count(), 1);
    EXPECT_EQ(micSpy.count(), 1);
    EXPECT_EQ(cameraSpy.count(), 1);
    ASSERT_EQ(joinSpy.count(), 1);
    const QList<QVariant> args = joinSpy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("wss://lk"));
    EXPECT_EQ(args.at(1).toString(), QStringLiteral("token"));
    EXPECT_EQ(args.at(2).toString(), QStringLiteral("{\"m\":1}"));
}
