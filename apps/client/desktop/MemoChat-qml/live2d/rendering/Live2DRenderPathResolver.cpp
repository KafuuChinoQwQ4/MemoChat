#include "Live2DRenderPathResolver.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QUrl>

namespace Live2DRenderPathResolver
{

QString resolveModelPath(const QString& modelRoot, const QString& modelJson, const QString& defaultModelPath)
{
    Q_UNUSED(defaultModelPath);

    const QString cleaned = modelJson.trimmed();
    if (cleaned.isEmpty())
    {
        return {};
    }
    if (cleaned.startsWith(QStringLiteral("qrc:/")))
    {
        return QStringLiteral(":") + QUrl(cleaned).path();
    }
    if (cleaned.startsWith(QStringLiteral(":/")))
    {
        return QDir::cleanPath(cleaned);
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

    QStringList candidates;
    const QString root = modelRoot.trimmed();
    if (!root.isEmpty())
    {
        candidates << QDir(root).absoluteFilePath(cleaned);
    }
    candidates << QDir::current().absoluteFilePath(cleaned);
    for (const QString& candidate : candidates)
    {
        if (QFileInfo::exists(candidate))
        {
            return QDir::cleanPath(candidate);
        }
    }
    return QDir::cleanPath(candidates.value(0, QDir::current().absoluteFilePath(cleaned)));
}

} // namespace Live2DRenderPathResolver
