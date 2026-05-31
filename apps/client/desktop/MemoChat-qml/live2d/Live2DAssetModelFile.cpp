#include "Live2DAssetModelFile.h"

#include "Live2DAssetPaths.h"
#include "Live2DModelAssetParser.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QtGlobal>

namespace
{
constexpr auto kStatusEmpty = "empty";
constexpr auto kStatusMissing = "missing";
constexpr auto kStatusInvalid = "invalid";
} // namespace

namespace Live2DAssetModelFile
{

Result load(const QString& modelRoot,
            const QString& modelJson,
            QStringList& errors,
            QStringList& warnings,
            QStringList& packageFiles)
{
    Result result;
    const QString modelRootPath = Live2DAssetPaths::resolveInputPath(modelRoot);
    if (Live2DAssetPaths::cleanedInput(modelRoot).isEmpty())
    {
        warnings.append(QStringLiteral("模型根目录待填写"));
    }
    else if (!QFileInfo::exists(modelRootPath) || !QFileInfo(modelRootPath).isDir())
    {
        warnings.append(QStringLiteral("模型根目录不存在：%1").arg(modelRoot));
    }

    if (Live2DAssetPaths::cleanedInput(modelJson).isEmpty())
    {
        result.terminalStatus = QString::fromLatin1(kStatusEmpty);
        result.terminalStatusText = QStringLiteral("等待选择 model3.json");
        return result;
    }

    result.modelJsonPath = Live2DAssetPaths::resolveInputPath(modelJson, modelRootPath);
    if (!result.modelJsonPath.endsWith(QStringLiteral(".model3.json"), Qt::CaseInsensitive))
    {
        warnings.append(QStringLiteral("建议选择 .model3.json 文件"));
    }
    if (!Live2DModelAssetParser::fileExists(result.modelJsonPath) || QFileInfo(result.modelJsonPath).isDir())
    {
        errors.append(QStringLiteral("model3.json 不存在：%1").arg(modelJson));
        result.terminalStatus = QString::fromLatin1(kStatusMissing);
        result.terminalStatusText = errors.first();
        return result;
    }

    QFile modelFile(Live2DModelAssetParser::qrcFilePath(result.modelJsonPath));
    if (!modelFile.open(QIODevice::ReadOnly))
    {
        errors.append(QStringLiteral("无法读取 model3.json：%1").arg(modelFile.errorString()));
        result.terminalStatus = QString::fromLatin1(kStatusInvalid);
        result.terminalStatusText = errors.first();
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(modelFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        errors.append(QStringLiteral("model3.json 格式无效：%1").arg(parseError.errorString()));
        result.terminalStatus = QString::fromLatin1(kStatusInvalid);
        result.terminalStatusText = errors.first();
        return result;
    }

    const QFileInfo modelInfo(result.modelJsonPath);
    result.modelDirectory = Live2DModelAssetParser::isQrcPath(result.modelJsonPath)
                                ? QString(result.modelJsonPath).left(result.modelJsonPath.lastIndexOf(QLatin1Char('/')))
                                : modelInfo.absolutePath();
    const QJsonObject model = document.object();
    result.refs = model.value(QStringLiteral("FileReferences")).toObject();
    Live2DModelAssetParser::appendExistingPackageFile(packageFiles, result.modelJsonPath);
    if (result.refs.isEmpty())
    {
        errors.append(QStringLiteral("model3.json 缺少 FileReferences"));
    }
    result.loaded = true;
    return result;
}

} // namespace Live2DAssetModelFile
