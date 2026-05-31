#ifndef LIVE2DASSETREQUIREDFILES_H
#define LIVE2DASSETREQUIREDFILES_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace Live2DAssetRequiredFiles
{

struct Result
{
    int textureCount = 0;
};

Result collect(const QJsonObject& refs,
               const QString& modelDirectory,
               QStringList& errors,
               QStringList& warnings,
               QStringList& packageFiles);

} // namespace Live2DAssetRequiredFiles

#endif // LIVE2DASSETREQUIREDFILES_H
