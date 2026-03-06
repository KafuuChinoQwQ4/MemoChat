#ifndef ICONPATHUTILS_H
#define ICONPATHUTILS_H

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

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
