#ifndef PETASSETSETTINGSPRIVATE_H
#define PETASSETSETTINGSPRIVATE_H

#include <QDir>
#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QVariantMap>
#include <QtGlobal>

namespace memochat::pet_asset_settings
{
constexpr int kSchemaVersion = 1;
constexpr int kLanguageOptionCount = 6;

inline QString clientSourcePath(const QString& relativePath)
{
#ifdef MEMOCHAT_QML_SOURCE_DIR
    const QString root = QString::fromUtf8(MEMOCHAT_QML_SOURCE_DIR);
#else
    const QString root = QDir::currentPath();
#endif
    return QDir(root).absoluteFilePath(relativePath);
}

inline QString stringValue(const QVariantMap& values, const QString& key, const QString& fallback)
{
    const QVariant value = values.value(key);
    return value.canConvert<QString>() ? value.toString() : fallback;
}

inline int intValue(const QVariantMap& values, const QString& key, int fallback, int minimum, int maximum)
{
    bool ok = false;
    const int parsed = values.value(key).toInt(&ok);
    return qBound(minimum, ok ? parsed : fallback, maximum);
}

inline qreal realValue(const QVariantMap& values, const QString& key, qreal fallback, qreal minimum, qreal maximum)
{
    bool ok = false;
    const qreal parsed = values.value(key).toDouble(&ok);
    return qBound(minimum, ok ? parsed : fallback, maximum);
}

inline bool boolValue(const QVariantMap& values, const QString& key, bool fallback)
{
    const QVariant value = values.value(key);
    if (value.userType() == QMetaType::Bool)
    {
        return value.toBool();
    }
    if (value.canConvert<QString>())
    {
        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("true") || text == QStringLiteral("1") || text == QStringLiteral("yes"))
        {
            return true;
        }
        if (text == QStringLiteral("false") || text == QStringLiteral("0") || text == QStringLiteral("no"))
        {
            return false;
        }
    }
    return fallback;
}

} // namespace memochat::pet_asset_settings

#endif // PETASSETSETTINGSPRIVATE_H
