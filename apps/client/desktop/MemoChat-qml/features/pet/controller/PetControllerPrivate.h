#ifndef PETCONTROLLERPRIVATE_H
#define PETCONTROLLERPRIVATE_H

#include "PetNetworkRequestUtils.h"
#include "PetVisionFrameUtils.h"
#include "PetWindowsBridgeUtils.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>

#include <initializer_list>

namespace memochat::pet
{

constexpr int kMaxRememberedPetControlEvents = 256;

inline QString stringFromVariant(const QVariantMap& settings, const QString& key)
{
    return settings.value(key).toString().trimmed();
}

inline QString firstStringFromVariant(const QVariantMap& settings, std::initializer_list<const char*> keys)
{
    for (const char* key : keys)
    {
        const QString value = stringFromVariant(settings, QString::fromLatin1(key));
        if (!value.isEmpty())
        {
            return value;
        }
    }
    return {};
}

inline QString localPathFromUrlText(const QString& value)
{
    QString text = value.trimmed();
    if (text.startsWith(QStringLiteral("file://")))
    {
        text = QUrl(text).toLocalFile();
    }
    return text.trimmed();
}

inline QString joinedPath(const QString& directory, const QString& fileName)
{
    const QString dir = localPathFromUrlText(directory);
    const QString file = localPathFromUrlText(fileName);
    if (dir.isEmpty() || file.isEmpty())
    {
        return {};
    }
    if (QDir::isAbsolutePath(file))
    {
        return file;
    }
    return QDir(dir).filePath(file);
}

inline bool looksLikeAudioPath(const QString& path)
{
    const QString suffix = QFileInfo(path.trimmed()).suffix().toLower();
    return suffix ==
           QStringLiteral("wav") ||
               suffix == QStringLiteral("mp3") ||
                             suffix == QStringLiteral("ogg") ||
                                           suffix == QStringLiteral("flac") ||
                                                                    suffix == QStringLiteral("m4a") ||
                                                                                             suffix ==
                                                                                                 QStringLiteral("aac");
}

inline QString voiceTextLanguage(const QString& language)
{
    const QString normalized = language.trimmed().toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (normalized == QStringLiteral("ja-jp") || normalized == QStringLiteral("jp") ||
                                                                              normalized == QStringLiteral("ja"))
    {
        return QStringLiteral("ja");
    }
    if (normalized == QStringLiteral("en-us") || normalized == QStringLiteral("en-gb") ||
                                                                              normalized == QStringLiteral("en"))
    {
        return QStringLiteral("en");
    }
    if (normalized == QStringLiteral("ko-kr") || normalized == QStringLiteral("ko"))
    {
        return QStringLiteral("ko");
    }
    if (normalized == QStringLiteral("fr-fr") || normalized == QStringLiteral("fr"))
    {
        return QStringLiteral("fr");
    }
    if (normalized == QStringLiteral("es-es") || normalized == QStringLiteral("es"))
    {
        return QStringLiteral("es");
    }
    return QStringLiteral("zh");
}

} // namespace memochat::pet

#endif // PETCONTROLLERPRIVATE_H
