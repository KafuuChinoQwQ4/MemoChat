#include "AuthenticatedMediaCache.h"
#include "HttpMgrRequestUtils.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace
{

QString cacheRoot()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.isEmpty())
    {
        root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    if (root.isEmpty())
    {
        root = QDir::tempPath();
    }
    return QDir(root).filePath(QStringLiteral("authenticated-media"));
}

QString cachePathForUrl(const QString& remoteUrl)
{
    const QByteArray digest = QCryptographicHash::hash(remoteUrl.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QDir(cacheRoot()).filePath(QString::fromLatin1(digest) + QStringLiteral(".media"));
}

bool hasUsableCachedFile(const QString& path)
{
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.size() > 0;
}

bool writeCacheFile(const QString& path, const QByteArray& bytes)
{
    if (bytes.isEmpty())
    {
        return false;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    if (file.write(bytes) != bytes.size())
    {
        return false;
    }
    return file.commit();
}

} // namespace

namespace memochat::media
{

QString resolveAuthenticatedMediaDownloadUrl(const QString& remoteUrl, const QString& accessToken)
{
    const QUrl url(remoteUrl);
    const QString token = accessToken.trimmed();
    if (!url.isValid() || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0 || token.isEmpty() ||
                                               QCoreApplication::instance() == nullptr)
    {
        return remoteUrl;
    }

    const QString cachePath = cachePathForUrl(url.toString());
    if (hasUsableCachedFile(cachePath))
    {
        return QUrl::fromLocalFile(cachePath).toString();
    }

    QNetworkRequest request(url);
    request.setRawHeader(QByteArrayLiteral("Authorization"), QByteArrayLiteral("Bearer ") + token.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    configureSecureNetworkRequest(request);

    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(4000);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool ok = reply->error() == QNetworkReply::NoError && status >= 200 && status < 300;
    const QByteArray bytes = ok ? reply->readAll() : QByteArray();
    reply->deleteLater();

    if (!ok || !writeCacheFile(cachePath, bytes))
    {
        return remoteUrl;
    }
    return QUrl::fromLocalFile(cachePath).toString();
}

} // namespace memochat::media
