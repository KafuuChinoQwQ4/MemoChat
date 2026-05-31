#include "Live2DAssetPaths.h"

#include "Live2DModelAssetParser.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QSet>
#include <QUrl>

namespace Live2DAssetPaths
{

QString cleanedInput(const QString& value)
{
    return value.trimmed();
}

QString resolveInputPath(const QString& value, const QString& baseDirectory)
{
    const QString cleaned = cleanedInput(value);
    if (cleaned.isEmpty())
    {
        return {};
    }

    const QUrl url(cleaned);
    if (url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (cleaned.startsWith(QStringLiteral("qrc:/")) || cleaned.startsWith(QStringLiteral(":/")))
    {
        return cleaned;
    }
    if (QDir::isAbsolutePath(cleaned))
    {
        return QDir::cleanPath(cleaned);
    }

    QStringList candidates;
    if (!baseDirectory.isEmpty() && !cleaned.contains(QLatin1Char('/')) && !cleaned.contains(QLatin1Char('\\')))
    {
        candidates.append(QDir(baseDirectory).absoluteFilePath(cleaned));
    }
    candidates.append(QDir::current().absoluteFilePath(cleaned));
    if (!baseDirectory.isEmpty())
    {
        candidates.append(QDir(baseDirectory).absoluteFilePath(cleaned));
    }

    QSet<QString> seen;
    for (const QString& candidate : candidates)
    {
        const QString path = QDir::cleanPath(candidate);
        if (seen.contains(path))
        {
            continue;
        }
        seen.insert(path);
        if (Live2DModelAssetParser::fileExists(path))
        {
            return path;
        }
    }
    return QDir::cleanPath(candidates.value(0, QDir::current().absoluteFilePath(cleaned)));
}

QString resolveModelReference(const QString& modelDirectory, const QString& reference)
{
    const QString cleaned = cleanedInput(reference);
    if (cleaned.isEmpty())
    {
        return {};
    }
    if (cleaned.startsWith(QStringLiteral("qrc:/")) || cleaned.startsWith(QStringLiteral(":/")))
    {
        return cleaned;
    }
    const QUrl url(cleaned);
    if (url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (QDir::isAbsolutePath(cleaned))
    {
        return QDir::cleanPath(cleaned);
    }
    if (Live2DModelAssetParser::isQrcPath(modelDirectory))
    {
        return QDir::cleanPath(modelDirectory + QLatin1Char('/') + cleaned);
    }
    return QDir(modelDirectory).absoluteFilePath(cleaned);
}

int countFilesWithSuffixes(const QString& directoryPath, const QStringList& suffixes)
{
    if (directoryPath.isEmpty() || Live2DModelAssetParser::isQrcPath(directoryPath))
    {
        return 0;
    }
    const QDir dir(directoryPath);
    if (!dir.exists())
    {
        return 0;
    }

    int count = 0;
    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& file : files)
    {
        const QString name = file.fileName().toLower();
        for (const QString& suffix : suffixes)
        {
            if (name.endsWith(suffix))
            {
                ++count;
                break;
            }
        }
    }
    return count;
}

QStringList jsonStringArray(const QJsonValue& value)
{
    QStringList items;
    const QJsonArray array = value.toArray();
    for (const QJsonValue& item : array)
    {
        const QString text = item.toString().trimmed();
        if (!text.isEmpty())
        {
            items.append(text);
        }
    }
    return items;
}

} // namespace Live2DAssetPaths
