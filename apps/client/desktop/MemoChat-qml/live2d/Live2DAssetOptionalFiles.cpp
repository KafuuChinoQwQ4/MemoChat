#include "Live2DAssetOptionalFiles.h"

#include "Live2DAssetPaths.h"
#include "Live2DModelAssetParser.h"

#include <QFileInfo>

namespace
{

QString collectOptionalReference(const QJsonObject& refs,
                                 const QString& key,
                                 const QString& label,
                                 const QString& modelDirectory,
                                 QStringList& warnings,
                                 QStringList& packageFiles)
{
    const QString reference = refs.value(key).toString().trimmed();
    if (reference.isEmpty())
    {
        return {};
    }

    const QString resolved = Live2DAssetPaths::resolveModelReference(modelDirectory, reference);
    Live2DModelAssetParser::appendJsonReferenceWarning(warnings, label, reference, resolved);
    if (Live2DModelAssetParser::fileExists(resolved) && !QFileInfo(resolved).isDir())
    {
        Live2DModelAssetParser::appendJsonParseWarning(warnings, label, resolved, &packageFiles);
        return resolved;
    }
    return reference;
}

} // namespace

namespace Live2DAssetOptionalFiles
{

Result collect(const QJsonObject& refs,
               const QString& modelDirectory,
               const QString& vtubeMappingInput,
               QStringList& warnings,
               QStringList& packageFiles)
{
    Result result;
    result.physicsFile = collectOptionalReference(
        refs,
        QStringLiteral("Physics"), QStringLiteral("物理文件"), modelDirectory, warnings, packageFiles);
    result.poseFile = collectOptionalReference(
        refs,
        QStringLiteral("Pose"), QStringLiteral("姿势文件"), modelDirectory, warnings, packageFiles);
    result.userDataFile = collectOptionalReference(
        refs,
        QStringLiteral("UserData"), QStringLiteral("用户数据文件"), modelDirectory, warnings, packageFiles);

    result.vtubeMappingFile = Live2DAssetPaths::cleanedInput(vtubeMappingInput).isEmpty()
                                  ? Live2DModelAssetParser::findSiblingVtubeMapping(modelDirectory)
                                  : Live2DAssetPaths::resolveInputPath(vtubeMappingInput, modelDirectory);
    if (!result.vtubeMappingFile.isEmpty())
    {
        Live2DModelAssetParser::appendJsonParseWarning(
            warnings,
            QStringLiteral("VTube 映射"), result.vtubeMappingFile, &packageFiles);
    }
    return result;
}

} // namespace Live2DAssetOptionalFiles
