#include "MediaPendingAttachmentRunner.h"

#include <gtest/gtest.h>

#include <QStringList>

namespace
{
QVariantMap attachment(const QString& id = QStringLiteral("att-1"), const QString& type = QStringLiteral("image"))
{
    QVariantMap item;
    item.insert(QStringLiteral("attachmentId"), id);
    item.insert(QStringLiteral("type"), type);
    item.insert(QStringLiteral("fileUrl"), QStringLiteral("file:///tmp/%1.bin").arg(id));
    item.insert(QStringLiteral("fileName"), QStringLiteral("%1.bin").arg(id));
    return item;
}

class RunnerHarness
{
public:
    ChatPendingSendQueueSnapshot snapshot;
    MediaPendingAttachmentSessionSnapshot session{42, QStringLiteral("token")};
    int currentDialogUid = 100;
    bool uploadOk = true;
    QString uploadError;
    bool dispatchOk = true;
    QString dispatchError;
    bool progress = false;
    QStringList progressTexts;
    QStringList tips;
    QList<bool> inProgressValues;
    int resetCount = 0;
    int uploadCount = 0;
    int dispatchCount = 0;
    int advanceCount = 0;
    int removeCount = 0;
    QString removedAttachmentId;
    int removedDialogUid = 0;
    MediaUploadRequest lastRequest;
    UploadedAttachmentDestination lastDestination;

    MediaPendingAttachmentRunnerPort port()
    {
        MediaPendingAttachmentRunnerPort p;
        p.queueSnapshot = [this]()
        {
            return snapshot;
        };
        p.sessionSnapshot = [this]()
        {
            return session;
        };
        p.currentDialogUid = [this]()
        {
            return currentDialogUid;
        };
        p.uploadLocalFile = [this](const MediaUploadRequest& request,
                                   const MediaPendingAttachmentRunnerPort::UploadProgressCallback& callback)
        {
            ++uploadCount;
            lastRequest = request;
            if (progress && callback)
            {
                callback(120, QStringLiteral("上传中"));
            }

            MediaUploadResult result;
            result.ok = uploadOk;
            result.errorText = uploadError;
            result.info.remoteUrl = QStringLiteral("https://cdn.example/%1").arg(request.fallbackName);
            result.info.fileName = request.fallbackName;
            result.info.mimeType = QStringLiteral("application/octet-stream");
            result.info.sizeBytes = 123;
            return result;
        };
        p.dispatchUploadedAttachment =
            [this](const QVariantMap&, const UploadedMediaInfo&, const UploadedAttachmentDestination& destination)
        {
            ++dispatchCount;
            lastDestination = destination;
            UploadedAttachmentDispatchResult result;
            result.success = dispatchOk;
            result.errorText = dispatchError;
            return result;
        };
        p.setUploadInProgress = [this](bool value)
        {
            inProgressValues.push_back(value);
        };
        p.setProgressText = [this](const QString& value)
        {
            progressTexts.push_back(value);
        };
        p.setTip = [this](const QString& value, bool error)
        {
            tips.push_back(error ? QStringLiteral("error:%1").arg(value) : value);
        };
        p.resetQueue = [this]()
        {
            ++resetCount;
        };
        p.removePendingAttachmentById = [this](const QString& attachmentId, int dialogUid)
        {
            ++removeCount;
            removedAttachmentId = attachmentId;
            removedDialogUid = dialogUid;
        };
        p.advanceQueue = [this]()
        {
            ++advanceCount;
            return false;
        };
        return p;
    }

    void setCurrent(const QString& id = QStringLiteral("att-1"))
    {
        snapshot.hasCurrent = true;
        snapshot.currentAttachment = attachment(id);
        snapshot.dialogUid = 100;
        snapshot.chatUid = 200;
        snapshot.groupId = 0;
        snapshot.totalCount = 2;
        snapshot.remainingCount = 2;
        snapshot.currentIndex = 1;
    }
};
} // namespace

TEST(MediaPendingAttachmentRunnerTest, EmptyQueueResetsAndClearsProgress)
{
    RunnerHarness harness;

    const MediaPendingAttachmentProcessResult result = MediaPendingAttachmentRunner::processNext(harness.port());

    EXPECT_FALSE(result.processedAttachment);
    EXPECT_FALSE(result.shouldContinue);
    EXPECT_EQ(harness.resetCount, 1);
    ASSERT_FALSE(harness.inProgressValues.isEmpty());
    EXPECT_FALSE(harness.inProgressValues.last());
    ASSERT_FALSE(harness.progressTexts.isEmpty());
    EXPECT_TRUE(harness.progressTexts.last().isEmpty());
    EXPECT_EQ(harness.uploadCount, 0);
    EXPECT_EQ(harness.dispatchCount, 0);
}

TEST(MediaPendingAttachmentRunnerTest, MissingSelfOrTokenResetsWithLoginTip)
{
    RunnerHarness harness;
    harness.setCurrent();
    harness.session = {};

    const MediaPendingAttachmentProcessResult result = MediaPendingAttachmentRunner::processNext(harness.port());

    EXPECT_FALSE(result.processedAttachment);
    EXPECT_EQ(harness.resetCount, 1);
    ASSERT_EQ(harness.tips.size(), 1);
    EXPECT_EQ(harness.tips.first(), QStringLiteral("error:用户状态异常，请重新登录"));
    ASSERT_FALSE(harness.inProgressValues.isEmpty());
    EXPECT_FALSE(harness.inProgressValues.last());
    ASSERT_FALSE(harness.progressTexts.isEmpty());
    EXPECT_TRUE(harness.progressTexts.last().isEmpty());
    EXPECT_EQ(harness.uploadCount, 0);
}

TEST(MediaPendingAttachmentRunnerTest, UploadFailureResetsAndShowsFallbackTip)
{
    RunnerHarness harness;
    harness.setCurrent();
    harness.uploadOk = false;

    const MediaPendingAttachmentProcessResult result = MediaPendingAttachmentRunner::processNext(harness.port());

    EXPECT_TRUE(result.processedAttachment);
    EXPECT_FALSE(result.shouldContinue);
    EXPECT_EQ(harness.uploadCount, 1);
    EXPECT_EQ(harness.dispatchCount, 0);
    EXPECT_EQ(harness.resetCount, 1);
    ASSERT_FALSE(harness.tips.isEmpty());
    EXPECT_EQ(harness.tips.last(), QStringLiteral("error:图片上传失败"));
    ASSERT_FALSE(harness.inProgressValues.isEmpty());
    EXPECT_FALSE(harness.inProgressValues.last());
}

TEST(MediaPendingAttachmentRunnerTest, DispatchFailureResetsAndShowsDispatchTip)
{
    RunnerHarness harness;
    harness.setCurrent();
    harness.dispatchOk = false;
    harness.dispatchError = QStringLiteral("dispatch failed");

    const MediaPendingAttachmentProcessResult result = MediaPendingAttachmentRunner::processNext(harness.port());

    EXPECT_TRUE(result.processedAttachment);
    EXPECT_FALSE(result.shouldContinue);
    EXPECT_EQ(harness.uploadCount, 1);
    EXPECT_EQ(harness.dispatchCount, 1);
    EXPECT_EQ(harness.resetCount, 1);
    ASSERT_FALSE(harness.tips.isEmpty());
    EXPECT_EQ(harness.tips.last(), QStringLiteral("error:dispatch failed"));
    EXPECT_EQ(harness.removeCount, 0);
    ASSERT_FALSE(harness.inProgressValues.isEmpty());
    EXPECT_FALSE(harness.inProgressValues.last());
}

TEST(MediaPendingAttachmentRunnerTest, SuccessRemovesAndAdvances)
{
    RunnerHarness harness;
    harness.setCurrent(QStringLiteral("att-2"));
    harness.currentDialogUid = 100;
    harness.snapshot.chatUid = 300;
    harness.snapshot.groupId = 0;
    harness.snapshot.totalCount = 3;
    harness.snapshot.currentIndex = 2;

    MediaPendingAttachmentRunnerPort port = harness.port();
    port.advanceQueue = [&harness]()
    {
        ++harness.advanceCount;
        return true;
    };

    const MediaPendingAttachmentProcessResult result = MediaPendingAttachmentRunner::processNext(port);

    EXPECT_TRUE(result.processedAttachment);
    EXPECT_TRUE(result.shouldContinue);
    EXPECT_EQ(harness.uploadCount, 1);
    EXPECT_EQ(harness.dispatchCount, 1);
    EXPECT_EQ(harness.removeCount, 1);
    EXPECT_EQ(harness.removedAttachmentId, QStringLiteral("att-2"));
    EXPECT_EQ(harness.removedDialogUid, 100);
    EXPECT_EQ(harness.advanceCount, 1);
    EXPECT_EQ(harness.lastRequest.uid, 42);
    EXPECT_EQ(harness.lastRequest.token, QStringLiteral("token"));
    ASSERT_EQ(harness.lastRequest.grantUids.size(), 1);
    EXPECT_EQ(harness.lastRequest.grantUids.first(), 300);
    EXPECT_EQ(harness.lastRequest.grantGroupId, 0);
    EXPECT_EQ(harness.lastDestination.selfUid, 42);
    EXPECT_EQ(harness.lastDestination.dialogUid, 100);
    EXPECT_EQ(harness.lastDestination.currentDialogUid, 100);
    ASSERT_FALSE(harness.inProgressValues.isEmpty());
    EXPECT_TRUE(harness.inProgressValues.last());
}

TEST(MediaPendingAttachmentRunnerTest, GroupChatUploadGrantsGroupAudience)
{
    RunnerHarness harness;
    harness.setCurrent(QStringLiteral("group-att"));
    harness.snapshot.chatUid = 0;
    harness.snapshot.groupId = 9001;

    const MediaPendingAttachmentProcessResult result = MediaPendingAttachmentRunner::processNext(harness.port());

    EXPECT_TRUE(result.processedAttachment);
    EXPECT_EQ(harness.uploadCount, 1);
    EXPECT_TRUE(harness.lastRequest.grantUids.isEmpty());
    EXPECT_EQ(harness.lastRequest.grantGroupId, 9001);
}

TEST(MediaPendingAttachmentRunnerTest, LastItemClearsProgressAndUploadState)
{
    RunnerHarness harness;
    harness.setCurrent();
    harness.snapshot.totalCount = 1;
    harness.snapshot.remainingCount = 1;
    harness.progress = true;

    const MediaPendingAttachmentProcessResult result = MediaPendingAttachmentRunner::processNext(harness.port());

    EXPECT_TRUE(result.processedAttachment);
    EXPECT_FALSE(result.shouldContinue);
    EXPECT_EQ(harness.removeCount, 1);
    EXPECT_EQ(harness.advanceCount, 1);
    ASSERT_GE(harness.progressTexts.size(), 3);
    EXPECT_EQ(harness.progressTexts.at(0), QStringLiteral("正在发送附件 1/1"));
    EXPECT_EQ(harness.progressTexts.at(1), QStringLiteral("正在发送附件 1/1 上传中 100%"));
    EXPECT_TRUE(harness.progressTexts.last().isEmpty());
    ASSERT_FALSE(harness.inProgressValues.isEmpty());
    EXPECT_FALSE(harness.inProgressValues.last());
}
