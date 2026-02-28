#include "MediaUploadService.h"
#include "global.h"
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace {
QString resolveLocalPath(const QString &localFileUrl)
{
    QUrl url(localFileUrl);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return localFileUrl;
}
}

bool MediaUploadService::uploadLocalFile(const QString &localFileUrl,
                                         const QString &mediaType,
                                         int uid,
                                         UploadedMediaInfo *outInfo,
                                         QString *errorText)
{
    const QString localPath = resolveLocalPath(localFileUrl);
    QFile file(localPath);
    if (!file.exists()) {
        if (errorText) {
            *errorText = "文件不存在";
        }
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorText) {
            *errorText = "文件读取失败";
        }
        return false;
    }

    const QByteArray fileBytes = file.readAll();
    file.close();

    if (fileBytes.isEmpty()) {
        if (errorText) {
            *errorText = "文件内容为空";
        }
        return false;
    }

    if (fileBytes.size() > 8 * 1024 * 1024) {
        if (errorText) {
            *errorText = "文件过大，请控制在 8MB 以内";
        }
        return false;
    }

    const QFileInfo fileInfo(localPath);
    const QMimeDatabase mimeDb;
    const QString mimeType = mimeDb.mimeTypeForFile(fileInfo).name();

    QJsonObject payload;
    payload["uid"] = uid;
    payload["media_type"] = mediaType;
    payload["file_name"] = fileInfo.fileName();
    payload["mime"] = mimeType;
    payload["data_base64"] = QString::fromLatin1(fileBytes.toBase64());

    QNetworkRequest request(QUrl(gate_url_prefix + "/upload_media"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (!reply) {
        if (errorText) {
            *errorText = "上传请求创建失败";
        }
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(20000);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    const QNetworkReply::NetworkError netErr = reply->error();
    const QByteArray response = reply->readAll();
    reply->deleteLater();
    if (netErr != QNetworkReply::NoError) {
        if (errorText) {
            *errorText = "上传失败：网络异常";
        }
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(response);
    if (!doc.isObject()) {
        if (errorText) {
            *errorText = "上传失败：响应解析错误";
        }
        return false;
    }
    const QJsonObject obj = doc.object();
    if (obj.value("error").toInt(1) != 0) {
        if (errorText) {
            *errorText = obj.value("message").toString("上传失败");
        }
        return false;
    }

    QString remoteUrl = obj.value("url").toString();
    if (remoteUrl.startsWith("/")) {
        remoteUrl = gate_url_prefix + remoteUrl;
    }
    if (remoteUrl.isEmpty()) {
        if (errorText) {
            *errorText = "上传失败：无可用地址";
        }
        return false;
    }

    if (outInfo) {
        outInfo->remoteUrl = remoteUrl;
        outInfo->fileName = obj.value("file_name").toString(fileInfo.fileName());
        outInfo->mimeType = obj.value("mime").toString(mimeType);
    }
    return true;
}
