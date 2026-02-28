#include "MessageContentCodec.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QUrl>

namespace {
const QString kImagePrefix = "__memochat_img__:";
const QString kFilePrefix = "__memochat_file__:";
const QString kCallPrefix = "__memochat_call__:";
}

QString MessageContentCodec::encodeImage(const QString &fileUrl)
{
    return kImagePrefix + fileUrl;
}

QString MessageContentCodec::encodeFile(const QString &fileUrl, const QString &fileName)
{
    if (!fileName.isEmpty()) {
        QJsonObject fileObj;
        fileObj["url"] = fileUrl;
        fileObj["name"] = fileName;
        const QByteArray compact = QJsonDocument(fileObj).toJson(QJsonDocument::Compact);
        return kFilePrefix + QString("json:") + QString::fromLatin1(compact.toBase64());
    }
    return kFilePrefix + fileUrl;
}

QString MessageContentCodec::encodeCallInvite(const QString &callType, const QString &joinUrl)
{
    QJsonObject callObj;
    callObj["type"] = callType;
    callObj["url"] = joinUrl;
    const QByteArray compact = QJsonDocument(callObj).toJson(QJsonDocument::Compact);
    return kCallPrefix + QString::fromLatin1(compact.toBase64());
}

DecodedMessageContent MessageContentCodec::decode(const QString &rawContent)
{
    DecodedMessageContent decoded;
    decoded.type = "text";
    decoded.content = rawContent;

    if (rawContent.startsWith(kImagePrefix)) {
        decoded.type = "image";
        decoded.content = rawContent.mid(kImagePrefix.size());
        return decoded;
    }

    if (rawContent.startsWith(kFilePrefix)) {
        decoded.type = "file";
        const QString payload = rawContent.mid(kFilePrefix.size());
        if (payload.startsWith("json:")) {
            const QByteArray jsonBytes = QByteArray::fromBase64(payload.mid(5).toLatin1());
            const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
            if (doc.isObject()) {
                const QJsonObject obj = doc.object();
                decoded.content = obj.value("url").toString();
                decoded.fileName = obj.value("name").toString();
            }
        }

        if (decoded.content.isEmpty()) {
            decoded.content = payload;
        }

        const QUrl url(decoded.content);
        if (url.isLocalFile()) {
            decoded.fileName = QFileInfo(url.toLocalFile()).fileName();
        } else if (decoded.fileName.isEmpty()) {
            decoded.fileName = QFileInfo(decoded.content).fileName();
        }
        if (decoded.fileName.isEmpty()) {
            decoded.fileName = "文件";
        }
        return decoded;
    }

    if (rawContent.startsWith(kCallPrefix)) {
        decoded.type = "call";
        const QString encodedPayload = rawContent.mid(kCallPrefix.size());
        const QByteArray payload = QByteArray::fromBase64(encodedPayload.toLatin1());
        const QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            const QString callType = obj.value("type").toString();
            decoded.content = obj.value("url").toString();
            decoded.fileName = (callType == "video") ? "视频通话邀请" : "语音通话邀请";
            if (decoded.content.isEmpty()) {
                decoded.content = rawContent;
                decoded.type = "text";
                decoded.fileName.clear();
            }
            return decoded;
        }

        decoded.type = "text";
        decoded.content = rawContent;
        decoded.fileName.clear();
        return decoded;
    }

    return decoded;
}

QString MessageContentCodec::toPreviewText(const QString &rawContent)
{
    const DecodedMessageContent decoded = decode(rawContent);
    if (decoded.type == "image") {
        return "[图片]";
    }
    if (decoded.type == "file") {
        if (!decoded.fileName.isEmpty()) {
            return QString("[文件] %1").arg(decoded.fileName);
        }
        return "[文件]";
    }
    if (decoded.type == "call") {
        return decoded.fileName.isEmpty() ? "[通话邀请]" : QString("[%1]").arg(decoded.fileName);
    }
    return decoded.content;
}
