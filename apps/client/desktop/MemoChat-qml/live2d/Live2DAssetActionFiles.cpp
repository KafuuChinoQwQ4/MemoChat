#include "Live2DAssetActionFiles.h"

#include "Live2DAssetPaths.h"
#include "Live2DModelAssetParser.h"
#include "Live2DMotionCatalog.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QSet>
#include <QtGlobal>

namespace Live2DAssetActionFiles
{

Result collect(const QJsonObject& refs,
               const QString& modelDirectory,
               const QString& resolvedMotionDirectory,
               const QString& resolvedExpressionDirectory,
               QStringList& warnings,
               QStringList& packageFiles)
{
    Result result;
    QSet<QString> actionKeys;
    QSet<QString> actionFiles;

    const QJsonObject motions = refs.value(QStringLiteral("Motions")).toObject();
    for (auto category = motions.constBegin(); category != motions.constEnd(); ++category)
    {
        const QJsonArray items = category.value().toArray();
        int motionIndex = 0;
        for (const QJsonValue& itemValue : items)
        {
            const QJsonObject item = itemValue.toObject();
            const QString file = item.value(QStringLiteral("File")).toString().trimmed();
            if (file.isEmpty())
            {
                ++motionIndex;
                continue;
            }
            ++result.motionCount;
            const QString group = category.key();
            const QString resolvedMotion = Live2DAssetPaths::resolveModelReference(modelDirectory, file);
            const QString displayName = Live2DMotionCatalog::actionDisplayName(
                item.value(QStringLiteral("Name")).toString(),
                           resolvedMotion,
                           QStringLiteral("%1 %2").arg(group).arg(motionIndex + 1));
            if (!Live2DModelAssetParser::fileExists(resolvedMotion))
            {
                warnings.append(QStringLiteral("动作文件不存在：%1").arg(file));
                ++motionIndex;
                continue;
            }
            Live2DMotionCatalog::appendActionItem(result.actionItems,
                                                  actionKeys,
                                                  actionFiles,
                                                  QStringLiteral("motion"),
                                                                 displayName,
                                                                 QStringLiteral("%1#%2").arg(group).arg(motionIndex),
                                                                                group,
                                                                                motionIndex,
                                                                                resolvedMotion,
                                                                                QStringLiteral("model3"));
            Live2DModelAssetParser::appendExistingPackageFile(packageFiles, resolvedMotion);
            ++motionIndex;
        }
    }

    const QJsonArray expressions = refs.value(QStringLiteral("Expressions")).toArray();
    for (const QJsonValue& itemValue : expressions)
    {
        const QJsonObject item = itemValue.toObject();
        const QString file = item.value(QStringLiteral("File")).toString().trimmed();
        if (file.isEmpty())
        {
            continue;
        }
        ++result.expressionCount;
        const QString resolvedExpression = Live2DAssetPaths::resolveModelReference(modelDirectory, file);
        const QString displayName = Live2DMotionCatalog::actionDisplayName(
            item.value(QStringLiteral("Name")).toString(), resolvedExpression, QFileInfo(file).completeBaseName());
        if (!Live2DModelAssetParser::fileExists(resolvedExpression))
        {
            warnings.append(QStringLiteral("表情文件不存在：%1").arg(file));
            continue;
        }
        Live2DMotionCatalog::appendActionItem(result.actionItems,
                                              actionKeys,
                                              actionFiles,
                                              QStringLiteral("expression"),
                                                             displayName,
                                                             displayName,
                                                             {},
                                                             -1,
                                                             resolvedExpression,
                                                             QStringLiteral("model3"));
        Live2DModelAssetParser::appendExistingPackageFile(packageFiles, resolvedExpression);
    }

    Live2DMotionCatalog::appendDirectoryActionItems(
        result.actionItems,
        actionKeys,
        actionFiles,
        QStringLiteral("motion"), resolvedMotionDirectory, {QStringLiteral("*.motion3.json")});
    Live2DMotionCatalog::appendDirectoryActionItems(
        result.actionItems,
        actionKeys,
        actionFiles,
        QStringLiteral("expression"), resolvedExpressionDirectory, {QStringLiteral("*.exp3.json")});

    result.motionCount =
        qMax(result.motionCount,
             Live2DAssetPaths::countFilesWithSuffixes(resolvedMotionDirectory, {QStringLiteral(".motion3.json")}));
    result.expressionCount =
        qMax(result.expressionCount,
             Live2DAssetPaths::countFilesWithSuffixes(resolvedExpressionDirectory, {QStringLiteral(".exp3.json")}));
    return result;
}

} // namespace Live2DAssetActionFiles
