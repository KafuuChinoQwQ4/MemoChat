#include "Live2DAssetValidation.h"

#include "Live2DAssetActionFiles.h"
#include "Live2DAssetModelFile.h"
#include "Live2DAssetOptionalFiles.h"
#include "Live2DAssetPaths.h"
#include "Live2DAssetRequiredFiles.h"
#include "Live2DAssetVoiceFiles.h"
#include "Live2DModelAssetParser.h"

namespace
{
constexpr auto kStatusInvalid = "invalid";
constexpr auto kStatusReady = "ready";

using Live2DAssetPaths::cleanedInput;
using Live2DAssetPaths::resolveInputPath;
using Live2DModelAssetParser::appendMissingDirectoryWarning;
using Live2DModelAssetParser::computePackageChecksum;

Live2DAssetValidation::Result makeResult(const QString& status,
                                         const QString& statusText,
                                         const QStringList& errors,
                                         const QStringList& warnings,
                                         int motionCount,
                                         int expressionCount,
                                         int textureCount,
                                         int voiceCount,
                                         const QString& physicsFile,
                                         const QString& poseFile,
                                         const QString& userDataFile,
                                         const QString& vtubeMappingFile,
                                         const QString& packageChecksum,
                                         int referencedFileCount,
                                         const QVariantList& actionItems = QVariantList())
{
    Live2DAssetValidation::Result result;
    result.status = status;
    result.statusText = statusText;
    result.errors = errors;
    result.warnings = warnings;
    result.motionCount = motionCount;
    result.expressionCount = expressionCount;
    result.textureCount = textureCount;
    result.voiceCount = voiceCount;
    result.physicsFile = physicsFile;
    result.poseFile = poseFile;
    result.userDataFile = userDataFile;
    result.vtubeMappingFile = vtubeMappingFile;
    result.packageChecksum = packageChecksum;
    result.referencedFileCount = referencedFileCount;
    result.actionItems = actionItems;
    return result;
}

int countVoiceFiles(const Live2DAssetValidation::Inputs& inputs)
{
    return Live2DAssetVoiceFiles::count(inputs.voiceDirectory);
}

} // namespace

namespace Live2DAssetValidation
{

Result validate(const Inputs& inputs)
{
    QStringList errors;
    QStringList warnings;
    QStringList packageFiles;
    int motionCount = 0;
    int expressionCount = 0;
    int textureCount = 0;
    Live2DAssetOptionalFiles::Result optionalFiles;

    const Live2DAssetModelFile::Result modelFile =
        Live2DAssetModelFile::load(inputs.modelRoot, inputs.modelJson, errors, warnings, packageFiles);
    if (!modelFile.loaded)
    {
        return makeResult(modelFile.terminalStatus,
                          modelFile.terminalStatusText,
                          errors,
                          warnings,
                          motionCount,
                          expressionCount,
                          textureCount,
                          countVoiceFiles(inputs),
                          {},
                          {},
                          {},
                          {},
                          {},
                          0);
    }

    const QString& modelDirectory = modelFile.modelDirectory;
    const QJsonObject& refs = modelFile.refs;

    const Live2DAssetRequiredFiles::Result requiredFiles =
        Live2DAssetRequiredFiles::collect(refs, modelDirectory, errors, warnings, packageFiles);
    textureCount = requiredFiles.textureCount;

    optionalFiles =
        Live2DAssetOptionalFiles::collect(refs, modelDirectory, inputs.vtubeMappingFile, warnings, packageFiles);

    const QString resolvedMotionDirectory = cleanedInput(inputs.motionDirectory).isEmpty()
                                                ? modelDirectory
                                                : resolveInputPath(inputs.motionDirectory, modelDirectory);
    const QString resolvedExpressionDirectory = cleanedInput(inputs.expressionDirectory).isEmpty()
                                                    ? modelDirectory
                                                    : resolveInputPath(inputs.expressionDirectory, modelDirectory);
    const QString resolvedVoiceDirectory = resolveInputPath(inputs.voiceDirectory);

    const Live2DAssetActionFiles::Result actionFiles = Live2DAssetActionFiles::collect(refs,
                                                                                       modelDirectory,
                                                                                       resolvedMotionDirectory,
                                                                                       resolvedExpressionDirectory,
                                                                                       warnings,
                                                                                       packageFiles);
    motionCount = actionFiles.motionCount;
    expressionCount = actionFiles.expressionCount;

    appendMissingDirectoryWarning(warnings,
                                  QStringLiteral("动作目录"), inputs.motionDirectory, resolvedMotionDirectory);
    appendMissingDirectoryWarning(warnings,
                                  QStringLiteral("表情目录"), inputs.expressionDirectory, resolvedExpressionDirectory);
    appendMissingDirectoryWarning(warnings, QStringLiteral("语音目录"), inputs.voiceDirectory, resolvedVoiceDirectory);

    const int voiceCount = Live2DAssetVoiceFiles::count(resolvedVoiceDirectory);
    Live2DAssetVoiceFiles::appendPackageFiles(packageFiles, resolvedVoiceDirectory);

    const QString status = errors.isEmpty() ? QString::fromLatin1(kStatusReady) : QString::fromLatin1(kStatusInvalid);
    const QString statusText = errors.isEmpty() ? QStringLiteral("资源校验通过：%1 动作 / %2 表情 / %3 贴图 / %4 语音")
                                                                     .arg(motionCount)
                                                                     .arg(expressionCount)
                                                                     .arg(textureCount)
                                                                     .arg(voiceCount)
                                                : errors.first();
    const QString checksum = errors.isEmpty() ? computePackageChecksum(modelDirectory, packageFiles) : QString();
    return makeResult(status,
                      statusText,
                      errors,
                      warnings,
                      motionCount,
                      expressionCount,
                      textureCount,
                      voiceCount,
                      optionalFiles.physicsFile,
                      optionalFiles.poseFile,
                      optionalFiles.userDataFile,
                      optionalFiles.vtubeMappingFile,
                      checksum,
                      checksum.isEmpty() ? 0 : packageFiles.size(),
                      actionFiles.actionItems);
}

} // namespace Live2DAssetValidation
