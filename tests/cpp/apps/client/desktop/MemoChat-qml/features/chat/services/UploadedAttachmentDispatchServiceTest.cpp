#include "UploadedAttachmentDispatchService.h"

#include "ChatDispatchService.h"
#include "MessageContentCodec.h"

#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonObject>

QString gate_url_prefix;
QString gate_media_url_prefix;

namespace
{
constexpr int kSelfUid = 1001;
constexpr int kPeerUid = 2002;
constexpr int kDialogUid = 42;
constexpr qint64 kGroupId = 7001;

QVariantMap imageAttachment()
{
    QVariantMap attachment;
    attachment.insert(QStringLiteral("type"), QStringLiteral("image"));
    attachment.insert(QStringLiteral("fileName"), QStringLiteral("photo.png"));
    return attachment;
}

QVariantMap fileAttachment()
{
    QVariantMap attachment;
    attachment.insert(QStringLiteral("type"), QStringLiteral("file"));
    attachment.insert(QStringLiteral("fileName"), QStringLiteral("fallback.txt"));
    return attachment;
}

UploadedMediaInfo imageUpload()
{
    UploadedMediaInfo uploaded;
    uploaded.remoteUrl = QStringLiteral("https://cdn.example.test/photo.png");
    uploaded.fileName = QStringLiteral("photo.png");
    uploaded.mimeType = QStringLiteral("image/png");
    uploaded.sizeBytes = 12;
    return uploaded;
}

UploadedMediaInfo fileUpload()
{
    UploadedMediaInfo uploaded;
    uploaded.remoteUrl = QStringLiteral("https://cdn.example.test/report.pdf");
    uploaded.fileName = QStringLiteral("report.pdf");
    uploaded.mimeType = QStringLiteral("application/pdf");
    uploaded.sizeBytes = 4096;
    return uploaded;
}

UploadedAttachmentDestination privateDestination(int currentDialogUid = kDialogUid)
{
    UploadedAttachmentDestination destination;
    destination.selfUid = kSelfUid;
    destination.dialogUid = kDialogUid;
    destination.currentDialogUid = currentDialogUid;
    destination.chatUid = kPeerUid;
    destination.groupId = 0;
    return destination;
}

UploadedAttachmentDestination groupDestination(int currentDialogUid = kDialogUid)
{
    UploadedAttachmentDestination destination;
    destination.selfUid = kSelfUid;
    destination.dialogUid = kDialogUid;
    destination.currentDialogUid = currentDialogUid;
    destination.chatUid = 0;
    destination.groupId = kGroupId;
    return destination;
}

struct DispatchHarness
{
    int currentPrivateCalls = 0;
    int currentGroupCalls = 0;
    int privatePacketCalls = 0;
    int groupPayloadCalls = 0;
    QString currentContent;
    QString currentPreview;
    OutgoingChatPacket privatePacket;
    QByteArray groupPayload;
    int groupRequestId = 0;
    bool currentPrivateResult = true;
    bool currentGroupResult = true;
    bool privatePacketResult = true;
    QString privatePacketError;

    UploadedAttachmentDispatchPort port()
    {
        UploadedAttachmentDispatchPort dispatchPort;
        dispatchPort.dispatchCurrentPrivateContent = [this](const QString& content, const QString& preview)
        {
            ++currentPrivateCalls;
            currentContent = content;
            currentPreview = preview;
            return currentPrivateResult;
        };
        dispatchPort.dispatchCurrentGroupContent = [this](const QString& content, const QString& preview)
        {
            ++currentGroupCalls;
            currentContent = content;
            currentPreview = preview;
            return currentGroupResult;
        };
        dispatchPort.dispatchPrivatePacket = [this](const OutgoingChatPacket& packet, QString* errorText)
        {
            ++privatePacketCalls;
            privatePacket = packet;
            if (errorText)
            {
                *errorText = privatePacketError;
            }
            return privatePacketResult;
        };
        dispatchPort.dispatchGroupPayload = [this](int reqId, const QByteArray& payload)
        {
            ++groupPayloadCalls;
            groupRequestId = reqId;
            groupPayload = payload;
        };
        return dispatchPort;
    }
};
} // namespace

TEST(UploadedAttachmentDispatchServiceTest, CurrentPrivateImageUsesCurrentPrivateAdapter)
{
    DispatchHarness harness;

    const UploadedAttachmentDispatchResult result = UploadedAttachmentDispatchService::dispatch(imageAttachment(),
                                                                                                imageUpload(),
                                                                                                privateDestination(),
                                                                                                harness.port());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.dispatchedCurrentDialog);
    EXPECT_EQ(harness.currentPrivateCalls, 1);
    EXPECT_EQ(harness.currentGroupCalls, 0);
    EXPECT_EQ(harness.privatePacketCalls, 0);
    EXPECT_EQ(harness.groupPayloadCalls, 0);
    EXPECT_EQ(harness.currentContent, MessageContentCodec::encodeImage(imageUpload().remoteUrl));
    EXPECT_EQ(harness.currentPreview, QStringLiteral("[图片]"));
}

TEST(UploadedAttachmentDispatchServiceTest, BackgroundPrivateFileBuildsOutgoingPacket)
{
    DispatchHarness harness;

    const UploadedAttachmentDispatchResult result = UploadedAttachmentDispatchService::dispatch(fileAttachment(),
                                                                                                fileUpload(),
                                                                                                privateDestination(99),
                                                                                                harness.port());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.dispatchedPrivatePacket);
    EXPECT_EQ(harness.currentPrivateCalls, 0);
    EXPECT_EQ(harness.privatePacketCalls, 1);
    EXPECT_EQ(harness.privatePacket.fromUid, kSelfUid);
    EXPECT_EQ(harness.privatePacket.toUid, kPeerUid);
    EXPECT_FALSE(harness.privatePacket.msgId.isEmpty());
    EXPECT_GT(harness.privatePacket.createdAt, 0);
    EXPECT_EQ(harness.privatePacket.content,
              MessageContentCodec::encodeFile(fileUpload().remoteUrl,
                                              fileUpload().fileName,
                                              fileUpload().mimeType,
                                              fileUpload().sizeBytes));
    EXPECT_EQ(result.previewText, QStringLiteral("[文件] report.pdf"));
}

TEST(UploadedAttachmentDispatchServiceTest, CurrentGroupFileUsesCurrentGroupAdapter)
{
    DispatchHarness harness;

    const UploadedAttachmentDispatchResult result =
        UploadedAttachmentDispatchService::dispatch(fileAttachment(), fileUpload(), groupDestination(), harness.port());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.dispatchedCurrentDialog);
    EXPECT_EQ(harness.currentGroupCalls, 1);
    EXPECT_EQ(harness.groupPayloadCalls, 0);
    EXPECT_EQ(harness.currentPreview, QStringLiteral("[文件] report.pdf"));
    EXPECT_EQ(harness.currentContent,
              MessageContentCodec::encodeFile(fileUpload().remoteUrl,
                                              fileUpload().fileName,
                                              fileUpload().mimeType,
                                              fileUpload().sizeBytes));
}

TEST(UploadedAttachmentDispatchServiceTest, BackgroundGroupImageBuildsGroupPayload)
{
    DispatchHarness harness;

    const UploadedAttachmentDispatchResult result = UploadedAttachmentDispatchService::dispatch(imageAttachment(),
                                                                                                imageUpload(),
                                                                                                groupDestination(99),
                                                                                                harness.port());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.dispatchedGroupPayload);
    EXPECT_EQ(result.requestId, 1044);
    EXPECT_EQ(harness.currentGroupCalls, 0);
    EXPECT_EQ(harness.groupPayloadCalls, 1);
    EXPECT_EQ(harness.groupRequestId, 1044);

    const QJsonObject payload = QJsonDocument::fromJson(harness.groupPayload).object();
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), kGroupId);
    const QJsonObject msg = payload.value(QStringLiteral("msg")).toObject();
    EXPECT_FALSE(msg.value(QStringLiteral("msgid")).toString().isEmpty());
    EXPECT_EQ(msg.value(QStringLiteral("content")).toString(),
                        MessageContentCodec::encodeImage(imageUpload().remoteUrl));
    EXPECT_EQ(msg.value(QStringLiteral("msgtype")).toString(), QStringLiteral("image"));
}

TEST(UploadedAttachmentDispatchServiceTest, DispatchFailureReturnsAdapterError)
{
    DispatchHarness harness;
    harness.privatePacketResult = false;
    harness.privatePacketError = QStringLiteral("transport failed");

    const UploadedAttachmentDispatchResult result = UploadedAttachmentDispatchService::dispatch(imageAttachment(),
                                                                                                imageUpload(),
                                                                                                privateDestination(99),
                                                                                                harness.port());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorText, QStringLiteral("transport failed"));
    EXPECT_EQ(harness.privatePacketCalls, 1);
}

TEST(UploadedAttachmentDispatchServiceTest, MissingSelfRejectsBeforeDispatch)
{
    DispatchHarness harness;
    UploadedAttachmentDestination destination = privateDestination();
    destination.selfUid = 0;

    const UploadedAttachmentDispatchResult result =
        UploadedAttachmentDispatchService::dispatch(imageAttachment(), imageUpload(), destination, harness.port());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorText, QStringLiteral("用户状态异常，请重新登录"));
    EXPECT_EQ(harness.currentPrivateCalls, 0);
    EXPECT_EQ(harness.privatePacketCalls, 0);
    EXPECT_EQ(harness.groupPayloadCalls, 0);
}
