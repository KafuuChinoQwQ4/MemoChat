#include "Live2DAssetRequiredFiles.h"

#include "Live2DAssetPaths.h"
#include "Live2DModelAssetParser.h"

namespace Live2DAssetRequiredFiles
{

Result collect(const QJsonObject& refs,
               const QString& modelDirectory,
               QStringList& errors,
               QStringList& warnings,
               QStringList& packageFiles)
{
    Result result;

    const QString moc = refs.value(QStringLiteral("Moc")).toString().trimmed();
    if (moc.isEmpty())
    {
        errors.append(QStringLiteral("model3.json 缺少 FileReferences.Moc"));
    }
    else
    {
        const QString resolvedMoc = Live2DAssetPaths::resolveModelReference(modelDirectory, moc);
        if (!Live2DModelAssetParser::fileExists(resolvedMoc))
        {
            errors.append(QStringLiteral("Moc 文件不存在：%1").arg(moc));
        }
        else
        {
            Live2DModelAssetParser::appendExistingPackageFile(packageFiles, resolvedMoc);
        }
    }

    const QStringList textures = Live2DAssetPaths::jsonStringArray(refs.value(QStringLiteral("Textures")));
    result.textureCount = textures.size();
    if (textures.isEmpty())
    {
        warnings.append(QStringLiteral("model3.json 未声明贴图"));
    }
    for (const QString& texture : textures)
    {
        const QString resolvedTexture = Live2DAssetPaths::resolveModelReference(modelDirectory, texture);
        if (!Live2DModelAssetParser::fileExists(resolvedTexture))
        {
            errors.append(QStringLiteral("贴图文件不存在：%1").arg(texture));
        }
        else
        {
            Live2DModelAssetParser::appendExistingPackageFile(packageFiles, resolvedTexture);
        }
    }

    return result;
}

} // namespace Live2DAssetRequiredFiles
