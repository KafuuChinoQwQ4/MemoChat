#ifndef ICONPATHUTILS_H
#define ICONPATHUTILS_H

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QUrl>

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
        return icon;
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
