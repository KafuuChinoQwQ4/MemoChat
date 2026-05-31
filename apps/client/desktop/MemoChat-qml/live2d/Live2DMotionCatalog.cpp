#include "Live2DMotionCatalog.h"

#include "Live2DModelAssetParser.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QUrl>
#include <QVariantMap>

namespace
{

QString normalizedKey(QString value)
{
    return value.trimmed().toLower();
}

QString actionFileKey(const QString& kind, const QString& file)
{
    const QString cleaned = file.trimmed();
    if (cleaned.isEmpty())
    {
        return {};
    }
    QString normalizedFile;
    if (cleaned.startsWith(QStringLiteral("qrc:/")))
    {
        normalizedFile = QStringLiteral("qrc:") + QUrl(cleaned).path();
    }
    else if (cleaned.startsWith(QStringLiteral(":/")))
    {
        normalizedFile = QDir::cleanPath(cleaned);
    }
    else if (QFileInfo(cleaned).isAbsolute())
    {
        normalizedFile = QDir::cleanPath(QFileInfo(cleaned).absoluteFilePath());
    }
    else
    {
        normalizedFile = QDir::cleanPath(cleaned);
    }
    return kind + QStringLiteral("|file|") + normalizedKey(normalizedFile);
}

} // namespace

namespace Live2DMotionCatalog
{

QString actionDisplayName(const QString& name, const QString& file, const QString& fallback)
{
    const QString trimmedName = name.trimmed();
    if (!trimmedName.isEmpty())
    {
        return trimmedName;
    }
    const QString fileName = QFileInfo(file).fileName();
    const QString lowerName = fileName.toLower();
    QString baseName;
    if (lowerName.endsWith(QStringLiteral(".motion3.json")))
    {
        baseName = fileName.left(fileName.size() - QStringLiteral(".motion3.json").size());
    }
    else if (lowerName.endsWith(QStringLiteral(".exp3.json")))
    {
        baseName = fileName.left(fileName.size() - QStringLiteral(".exp3.json").size());
    }
    else
    {
        baseName = QFileInfo(file).completeBaseName();
    }
    baseName = baseName.trimmed();
    if (!baseName.isEmpty())
    {
        return baseName;
    }
    return fallback.trimmed();
}

void appendActionItem(QVariantList& items,
                      QSet<QString>& actionKeys,
                      QSet<QString>& actionFiles,
                      const QString& kind,
                      const QString& name,
                      const QString& trigger,
                      const QString& group,
                      int index,
                      const QString& file,
                      const QString& source)
{
    const QString cleanKind = kind.trimmed();
    QString cleanName = name.trimmed();
    QString cleanTrigger = trigger.trimmed();
    if (cleanTrigger.isEmpty())
    {
        cleanTrigger = cleanName;
    }
    if (cleanName.isEmpty())
    {
        cleanName = cleanTrigger;
    }
    if (cleanKind.isEmpty() || cleanName.isEmpty() || cleanTrigger.isEmpty())
    {
        return;
    }

    const QString fileKey = actionFileKey(cleanKind, file);
    if (!fileKey.isEmpty())
    {
        if (actionFiles.contains(fileKey))
        {
            return;
        }
    }
    else
    {
        const QString itemKey = cleanKind + QLatin1Char('|') + normalizedKey(cleanTrigger);
        if (actionKeys.contains(itemKey))
        {
            return;
        }
    }

    QVariantMap item;
    item[QStringLiteral("kind")] = cleanKind;
    item[QStringLiteral("name")] = cleanName;
    item[QStringLiteral("trigger")] = cleanTrigger;
    item[QStringLiteral("group")] = group.trimmed();
    item[QStringLiteral("index")] = index;
    item[QStringLiteral("file")] = file.trimmed();
    item[QStringLiteral("source")] = source.trimmed();
    items.append(item);
    if (fileKey.isEmpty())
    {
        actionKeys.insert(cleanKind + QLatin1Char('|') + normalizedKey(cleanTrigger));
    }
    else
    {
        actionFiles.insert(fileKey);
    }
}

void appendDirectoryActionItems(QVariantList& items,
                                QSet<QString>& actionKeys,
                                QSet<QString>& actionFiles,
                                const QString& kind,
                                const QString& directoryPath,
                                const QStringList& nameFilters)
{
    if (directoryPath.isEmpty() || Live2DModelAssetParser::isQrcPath(directoryPath))
    {
        return;
    }
    const QDir dir(directoryPath);
    if (!dir.exists())
    {
        return;
    }

    QDirIterator it(dir.absolutePath(), nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString path = it.next();
        const QString name = actionDisplayName({}, path, {});
        appendActionItem(items, actionKeys, actionFiles, kind, name, name, {}, -1, path, QStringLiteral("directory"));
    }
}

} // namespace Live2DMotionCatalog
