#ifndef ICONPATHUTILS_H
#define ICONPATHUTILS_H

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

extern QString gate_url_prefix;
extern QString gate_media_url_prefix;

inline int &iconDownloadAuthUid()
{
    static int uid = 0;
    return uid;
}

inline QString &iconDownloadAuthToken()
{
    static QString token;
    return token;
}

inline void setIconDownloadAuthContext(int uid, const QString &token)
{
    iconDownloadAuthUid() = uid;
    iconDownloadAuthToken() = token.trimmed();
}

inline QString attachMediaDownloadAuth(QString icon)
{
    const int uid = iconDownloadAuthUid();
    const QString token = iconDownloadAuthToken();
    if (uid <= 0 || token.isEmpty()) {
        return icon;
    }

    QUrl url(icon);
    if (!url.isValid() || url.scheme().isEmpty() || url.isLocalFile()) {
        return icon;
    }

    const QString path = url.path();
    if (!path.endsWith(QStringLiteral("/media/download")) && !path.contains(QStringLiteral("/media/download"))) {
        return icon;
    }

    QUrlQuery query(url);
    query.removeAllQueryItems(QStringLiteral("uid"));
    query.removeAllQueryItems(QStringLiteral("token"));
    query.addQueryItem(QStringLiteral("uid"), QString::number(uid));
    query.addQueryItem(QStringLiteral("token"), token);
    url.setQuery(query);
    return url.toString();
}

inline bool isMediaDownloadPath(const QString &path)
{
    return path.endsWith(QStringLiteral("/media/download")) || path.contains(QStringLiteral("/media/download"));
}

inline bool isLocalMediaHost(const QString &host)
{
    const QString normalized = host.trimmed().toLower();
    return normalized == QStringLiteral("localhost")
        || normalized == QStringLiteral("127.0.0.1")
        || normalized == QStringLiteral("::1");
}

inline QString mediaDownloadBaseUrl()
{
    QString baseUrl = gate_media_url_prefix.trimmed();
    if (baseUrl.isEmpty()) {
        baseUrl = gate_url_prefix.trimmed();
    }
    return baseUrl;
}

inline QString normalizeRelativeMediaDownloadUrl(QString icon)
{
    if (icon.startsWith(QStringLiteral("media/download"))) {
        icon.prepend('/');
    }

    if (!icon.startsWith(QStringLiteral("/"))) {
        return icon;
    }

    const QUrl relativeUrl(icon);
    if (!isMediaDownloadPath(relativeUrl.path())) {
        return icon;
    }

    QString baseUrl = mediaDownloadBaseUrl();
    if (baseUrl.isEmpty()) {
        return attachMediaDownloadAuth(icon);
    }

    QString absoluteUrl = baseUrl;
    if (absoluteUrl.endsWith('/') && icon.startsWith('/')) {
        absoluteUrl.chop(1);
    }
    absoluteUrl += icon;
    return attachMediaDownloadAuth(absoluteUrl);
}

inline QString normalizeLocalMediaDownloadUrl(const QString &icon)
{
    const QUrl url(icon);
    if (!url.isValid() || url.scheme().isEmpty() || url.isLocalFile()) {
        return icon;
    }
    if (!isMediaDownloadPath(url.path()) || !isLocalMediaHost(url.host())) {
        return icon;
    }

    const QString baseUrl = mediaDownloadBaseUrl();
    if (baseUrl.isEmpty()) {
        return attachMediaDownloadAuth(icon);
    }

    const QUrl parsedBase(baseUrl);
    if (!parsedBase.isValid() || parsedBase.scheme().isEmpty() || parsedBase.host().isEmpty()) {
        return attachMediaDownloadAuth(icon);
    }

    QUrl normalized = url;
    normalized.setScheme(parsedBase.scheme());
    normalized.setHost(parsedBase.host());
    normalized.setPort(parsedBase.port(-1));
    return attachMediaDownloadAuth(normalized.toString());
}

inline QString normalizeIconForQml(QString icon)
{
    static const QString kDefaultIcon = QStringLiteral("qrc:/res/head_1.jpg");

    icon = icon.trimmed();
    if (icon.isEmpty()) {
        return kDefaultIcon;
    }

    if (icon.startsWith(QStringLiteral("qrc:/"), Qt::CaseInsensitive)) {
        return icon;
    }

    if (icon.startsWith(QStringLiteral(":/"))) {
        return QStringLiteral("qrc") + icon;
    }

    if (icon.startsWith(QStringLiteral("res/"))) {
        return QStringLiteral("qrc:/") + icon;
    }

    if (icon.startsWith(QStringLiteral("/res/"))) {
        return QStringLiteral("qrc:") + icon;
    }

    const QString mediaDownloadUrl = normalizeRelativeMediaDownloadUrl(icon);
    if (mediaDownloadUrl != icon) {
        return mediaDownloadUrl;
    }

    const QString localMediaDownloadUrl = normalizeLocalMediaDownloadUrl(icon);
    if (localMediaDownloadUrl != icon) {
        return localMediaDownloadUrl;
    }

    if (!icon.contains('/') && !icon.contains('\\')
        && icon.startsWith(QStringLiteral("head_"), Qt::CaseInsensitive)) {
        return QStringLiteral("qrc:/res/") + icon;
    }

    const QUrl parsedUrl(icon);
    if (parsedUrl.isValid() && !parsedUrl.scheme().isEmpty()) {
        if (parsedUrl.isLocalFile()) {
            const QString localPath = parsedUrl.toLocalFile();
            if (QFileInfo::exists(localPath)) {
                return QUrl::fromLocalFile(localPath).toString();
            }
            return kDefaultIcon;
        }
        return attachMediaDownloadAuth(icon);
    }

    if (QDir::isAbsolutePath(icon)) {
        const QString normalizedPath = QDir::fromNativeSeparators(icon);
        if (QFileInfo::exists(normalizedPath)) {
            return QUrl::fromLocalFile(normalizedPath).toString();
        }
        return kDefaultIcon;
    }

    return kDefaultIcon;
}

#endif // ICONPATHUTILS_H
