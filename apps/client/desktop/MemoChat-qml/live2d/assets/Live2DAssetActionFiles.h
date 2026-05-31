#ifndef LIVE2DASSETACTIONFILES_H
#define LIVE2DASSETACTIONFILES_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace Live2DAssetActionFiles
{

struct Result
{
    int motionCount = 0;
    int expressionCount = 0;
    QVariantList actionItems;
};

Result collect(const QJsonObject& refs,
               const QString& modelDirectory,
               const QString& resolvedMotionDirectory,
               const QString& resolvedExpressionDirectory,
               QStringList& warnings,
               QStringList& packageFiles);

} // namespace Live2DAssetActionFiles

#endif // LIVE2DASSETACTIONFILES_H
