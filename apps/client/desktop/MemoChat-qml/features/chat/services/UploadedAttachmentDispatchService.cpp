#include "UploadedAttachmentDispatchService.h"

#include "MessageContentCodec.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace
{
constexpr int kGroupChatRequestId = 1044;

UploadedAttachmentDispatchResult failedResult(const QString& errorText)
{
    UploadedAttachmentDispatchResult result;
    result.errorText = errorText;
    return result;
}

QString fallbackError(const QString& attachmentType, bool group)
{
    if (attachmentType == QStringLiteral("image"))
    {
        return group ? QStringLiteral("群图片发送失败") : QStringLiteral("图片发送失败");
    }
    return group ? QStringLiteral("群文件发送失败") : QStringLiteral("文件发送失败");
}

QJsonObject buildMessageObject(const QString& msgId, const QString& encoded)
{
    const DecodedMessageContent decoded = MessageContentCodec::decode(encoded);
    QJsonObject msgObj;
    msgObj.insert(QStringLiteral("msgid"), msgId);
    msgObj.insert(QStringLiteral("content"), encoded);
    msgObj.insert(QStringLiteral("msgtype"), decoded.type.isEmpty() ? QStringLiteral("text") : decoded.type);
    if (!decoded.fileName.isEmpty())
    {
        msgObj.insert(QStringLiteral("file_name"), decoded.fileName);
    }
    if (!decoded.mimeType.isEmpty())
    {
        msgObj.insert(QStringLiteral("mime"), decoded.mimeType);
    }
    if (decoded.sizeBytes > 0)
    {
        msgObj.insert(QStringLiteral("size"), static_cast<qint64>(decoded.sizeBytes));
    }
    return msgObj;
}

QByteArray buildGroupPayload(int selfUid, qint64 groupId, const QString& encoded)
{
    QJsonObject payloadObj;
    payloadObj.insert(QStringLiteral("fromuid"), selfUid);
    payloadObj.insert(QStringLiteral("groupid"), static_cast<qint64>(groupId));
    payloadObj.insert(QStringLiteral("msg"), buildMessageObject(QUuid::createUuid().toString(), encoded));
    return QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
}

QString encodedAttachmentContent(const QVariantMap& attachment, const UploadedMediaInfo& uploaded)
{
    const QString attachmentType = attachment.value(QStringLiteral("type")).toString();
    if (attachmentType == QStringLiteral("image"))
    {
        return MessageContentCodec::encodeImage(uploaded.remoteUrl);
    }

    const QString fallbackName = attachment.value(QStringLiteral("fileName")).toString();
    const QString remoteName = uploaded.fileName.isEmpty() ? fallbackName : uploaded.fileName;
    return MessageContentCodec::encodeFile(uploaded.remoteUrl, remoteName, uploaded.mimeType, uploaded.sizeBytes);
}

QString attachmentPreviewText(const QVariantMap& attachment, const UploadedMediaInfo& uploaded)
{
    const QString attachmentType = attachment.value(QStringLiteral("type")).toString();
    if (attachmentType == QStringLiteral("image"))
    {
        return QStringLiteral("[图片]");
    }

    const QString fallbackName = attachment.value(QStringLiteral("fileName")).toString();
    const QString remoteName = uploaded.fileName.isEmpty() ? fallbackName : uploaded.fileName;
    return remoteName.isEmpty() ? QStringLiteral("[文件]") : QStringLiteral("[文件] %1").arg(remoteName);
}
} // namespace

UploadedAttachmentDispatchResult
UploadedAttachmentDispatchService::dispatch(const QVariantMap& attachment,
                                            const UploadedMediaInfo& uploaded,
                                            const UploadedAttachmentDestination& destination,
                                            const UploadedAttachmentDispatchPort& port)
{
    if (destination.selfUid <= 0)
    {
        return failedResult(QStringLiteral("用户状态异常，请重新登录"));
    }

    const QString attachmentType = attachment.value(QStringLiteral("type")).toString();
    if (attachmentType != QStringLiteral("image") && attachmentType != QStringLiteral("file"))
    {
        return failedResult(QStringLiteral("不支持的附件类型"));
    }

    UploadedAttachmentDispatchResult result;
    result.encodedContent = encodedAttachmentContent(attachment, uploaded);
    result.previewText = attachmentPreviewText(attachment, uploaded);

    const bool groupDestination = destination.groupId > 0;
    const bool sameDialog = destination.currentDialogUid == destination.dialogUid;
    if (groupDestination)
    {
        if (sameDialog)
        {
            if (!port.dispatchCurrentGroupContent ||
                !port.dispatchCurrentGroupContent(result.encodedContent, result.previewText))
            {
                return failedResult(fallbackError(attachmentType, true));
            }
            result.dispatchedCurrentDialog = true;
            result.success = true;
            return result;
        }

        if (!port.dispatchGroupPayload)
        {
            return failedResult(fallbackError(attachmentType, true));
        }
        result.requestId = kGroupChatRequestId;
        port.dispatchGroupPayload(result.requestId,
                                  buildGroupPayload(destination.selfUid, destination.groupId, result.encodedContent));
        result.dispatchedGroupPayload = true;
        result.success = true;
        return result;
    }

    if (sameDialog)
    {
        if (!port.dispatchCurrentPrivateContent ||
            !port.dispatchCurrentPrivateContent(result.encodedContent, result.previewText))
        {
            return failedResult(fallbackError(attachmentType, false));
        }
        result.dispatchedCurrentDialog = true;
        result.success = true;
        return result;
    }

    if (destination.chatUid <= 0 || !port.dispatchPrivatePacket)
    {
        return failedResult(fallbackError(attachmentType, false));
    }

    OutgoingChatPacket packet;
    packet.fromUid = destination.selfUid;
    packet.toUid = destination.chatUid;
    packet.msgId = QUuid::createUuid().toString();
    packet.content = result.encodedContent;
    packet.createdAt = QDateTime::currentMSecsSinceEpoch();

    QString errorText;
    if (!port.dispatchPrivatePacket(packet, &errorText))
    {
        return failedResult(errorText.isEmpty() ? fallbackError(attachmentType, false) : errorText);
    }
    result.dispatchedPrivatePacket = true;
    result.success = true;
    return result;
}
