#ifndef LIVE2DASSETOPTIONALFILES_H
#define LIVE2DASSETOPTIONALFILES_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace Live2DAssetOptionalFiles
{

struct Result
{
    QString physicsFile;
    QString poseFile;
    QString userDataFile;
    QString vtubeMappingFile;
};

Result collect(const QJsonObject& refs,
               const QString& modelDirectory,
               const QString& vtubeMappingInput,
               QStringList& warnings,
               QStringList& packageFiles);

} // namespace Live2DAssetOptionalFiles

#endif // LIVE2DASSETOPTIONALFILES_H
