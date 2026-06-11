#include "AppCoordinators.h"

#include "LocalFilePickerService.h"

#include <gtest/gtest.h>

bool LocalFilePickerService::pickImageUrl(QString*, QString*)
{
    return false;
}

bool LocalFilePickerService::pickFileUrl(QString*, QString*, QString*)
{
    return false;
}

bool LocalFilePickerService::pickImageUrls(QVariantList*, QString*)
{
    return false;
}

bool LocalFilePickerService::pickMomentMediaUrls(QVariantList*, QString*)
{
    return false;
}

bool LocalFilePickerService::pickFileUrls(QVariantList*, QString*)
{
    return false;
}

bool LocalFilePickerService::pickAvatarUrl(QString*, QString*)
{
    return false;
}

bool LocalFilePickerService::pickAvatarFromScreen(QString*, QString*)
{
    return false;
}

bool LocalFilePickerService::pickAvatarFromWebcam(QString*, QString*)
{
    return false;
}

bool LocalFilePickerService::openUrl(const QString&, QString*)
{
    return false;
}

namespace
{
QVariantMap attachment(const QString& id = QStringLiteral("att-1"))
{
    QVariantMap item;
    item.insert(QStringLiteral("attachmentId"), id);
    item.insert(QStringLiteral("type"), QStringLiteral("file"));
    item.insert(QStringLiteral("fileUrl"), QStringLiteral("file:///tmp/%1.bin").arg(id));
    item.insert(QStringLiteral("fileName"), QStringLiteral("%1.bin").arg(id));
    return item;
}

class MediaCoordinatorHarness
{
public:
    MediaSendSnapshot snapshot;
    int privateDispatchCount = 0;
    int groupDispatchCount = 0;
    int beginAttachmentSendCount = 0;
    int startRunnerCount = 0;
    int cancelReplyCount = 0;
    QString privateContent;
    QString privatePreview;
    QString groupContent;
    QString groupPreview;
    int beginDialogUid = 0;
    int beginChatUid = 0;
    qint64 beginGroupId = 0;
    QVariantList beginAttachments;
    QStringList tips;

    MediaSendPort port()
    {
        MediaSendPort p;
        p.snapshot = [this]()
        {
            return snapshot;
        };
        p.setTip = [this](const QString& tip, bool isError)
        {
            tips.push_back(isError ? QStringLiteral("error:%1").arg(tip) : tip);
        };
        p.dispatchPrivateContent = [this](const QString& content, const QString& preview)
        {
            ++privateDispatchCount;
            privateContent = content;
            privatePreview = preview;
            return true;
        };
        p.dispatchGroupContent = [this](const QString& content, const QString& preview)
        {
            ++groupDispatchCount;
            groupContent = content;
            groupPreview = preview;
            return true;
        };
        p.cancelReply = [this]()
        {
            ++cancelReplyCount;
        };
        p.beginPendingAttachmentSend =
            [this](int dialogUid, int chatUid, qint64 groupId, const QVariantList& attachments)
        {
            ++beginAttachmentSendCount;
            beginDialogUid = dialogUid;
            beginChatUid = chatUid;
            beginGroupId = groupId;
            beginAttachments = attachments;
        };
        p.startPendingAttachmentRunner = [this]()
        {
            ++startRunnerCount;
        };
        return p;
    }
};
} // namespace

TEST(MediaCoordinatorTest, TextAndPendingAttachmentsSendTextBeforeStartingAttachmentQueue)
{
    MediaCoordinatorHarness harness;
    harness.snapshot.currentChatUid = 2002;
    harness.snapshot.currentDialogUid = 42;
    harness.snapshot.pendingAttachments = QVariantList{attachment()};

    MediaCoordinator coordinator(harness.port());
    coordinator.sendCurrentComposerPayload(QStringLiteral("hello with file"));

    EXPECT_EQ(harness.privateDispatchCount, 1);
    EXPECT_EQ(harness.privateContent, QStringLiteral("hello with file"));
    EXPECT_EQ(harness.privatePreview, QStringLiteral("hello with file"));
    EXPECT_EQ(harness.beginAttachmentSendCount, 1);
    EXPECT_EQ(harness.beginDialogUid, 42);
    EXPECT_EQ(harness.beginChatUid, 2002);
    EXPECT_EQ(harness.beginGroupId, 0);
    EXPECT_EQ(harness.beginAttachments.size(), 1);
    EXPECT_EQ(harness.startRunnerCount, 1);
}

TEST(MediaCoordinatorTest, PendingAttachmentsWithoutTextOnlyStartsAttachmentQueue)
{
    MediaCoordinatorHarness harness;
    harness.snapshot.currentChatUid = 2002;
    harness.snapshot.currentDialogUid = 42;
    harness.snapshot.pendingAttachments = QVariantList{attachment()};

    MediaCoordinator coordinator(harness.port());
    coordinator.sendCurrentComposerPayload(QStringLiteral("  "));

    EXPECT_EQ(harness.privateDispatchCount, 0);
    EXPECT_EQ(harness.beginAttachmentSendCount, 1);
    EXPECT_EQ(harness.startRunnerCount, 1);
}

TEST(MediaCoordinatorTest, UploadInProgressBlocksTextAndAttachmentSend)
{
    MediaCoordinatorHarness harness;
    harness.snapshot.currentChatUid = 2002;
    harness.snapshot.currentDialogUid = 42;
    harness.snapshot.uploadInProgress = true;
    harness.snapshot.pendingAttachments = QVariantList{attachment()};

    MediaCoordinator coordinator(harness.port());
    coordinator.sendCurrentComposerPayload(QStringLiteral("hello"));

    EXPECT_EQ(harness.privateDispatchCount, 0);
    EXPECT_EQ(harness.beginAttachmentSendCount, 0);
    EXPECT_EQ(harness.startRunnerCount, 0);
    ASSERT_FALSE(harness.tips.isEmpty());
    EXPECT_EQ(harness.tips.last(), QStringLiteral("error:已有文件上传中，请稍后"));
}
