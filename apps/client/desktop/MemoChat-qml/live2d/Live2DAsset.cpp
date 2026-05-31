#include "Live2DAsset.h"

#include "Live2DAssetValidation.h"

namespace
{
constexpr auto kStatusEmpty = "empty";
constexpr auto kStatusReady = "ready";
} // namespace

Live2DAsset::Live2DAsset(QObject* parent)
    : QObject(parent)
{
}

void Live2DAsset::setModelRoot(const QString& value)
{
    if (!assignIfChanged(_model_root, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setModelJson(const QString& value)
{
    if (!assignIfChanged(_model_json, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setMotionDirectory(const QString& value)
{
    if (!assignIfChanged(_motion_directory, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setExpressionDirectory(const QString& value)
{
    if (!assignIfChanged(_expression_directory, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setVoiceDirectory(const QString& value)
{
    if (!assignIfChanged(_voice_directory, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setVtubeMappingFile(const QString& value)
{
    if (!assignIfChanged(_vtube_mapping_file, value))
    {
        return;
    }
    _resolved_vtube_mapping_file.clear();
    emit assetInputsChanged();
    emit validationChanged();
}

void Live2DAsset::validate()
{
    Live2DAssetValidation::Inputs inputs;
    inputs.modelRoot = _model_root;
    inputs.modelJson = _model_json;
    inputs.motionDirectory = _motion_directory;
    inputs.expressionDirectory = _expression_directory;
    inputs.voiceDirectory = _voice_directory;
    inputs.vtubeMappingFile = _vtube_mapping_file;

    const Live2DAssetValidation::Result result = Live2DAssetValidation::validate(inputs);
    publishResult(result.status,
                  result.statusText,
                  result.errors,
                  result.warnings,
                  result.motionCount,
                  result.expressionCount,
                  result.textureCount,
                  result.voiceCount,
                  result.physicsFile,
                  result.poseFile,
                  result.userDataFile,
                  result.vtubeMappingFile,
                  result.packageChecksum,
                  result.referencedFileCount,
                  result.actionItems);
}

void Live2DAsset::clear()
{
    _model_root.clear();
    _model_json.clear();
    _motion_directory.clear();
    _expression_directory.clear();
    _voice_directory.clear();
    _vtube_mapping_file.clear();
    emit assetInputsChanged();
    publishResult(QString::fromLatin1(kStatusEmpty),
                  QStringLiteral("等待选择 model3.json"), {}, {}, 0, 0, 0, 0, {}, {}, {}, {}, {}, 0);
}

bool Live2DAsset::assignIfChanged(QString& target, const QString& value)
{
    if (target == value)
    {
        return false;
    }
    target = value;
    return true;
}

void Live2DAsset::publishResult(const QString& status,
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
                                const QVariantList& actionItems)
{
    _status = status;
    _valid = status == QString::fromLatin1(kStatusReady);
    _status_text = statusText;
    _errors = errors;
    _warnings = warnings;
    _motion_count = motionCount;
    _expression_count = expressionCount;
    _texture_count = textureCount;
    _voice_count = voiceCount;
    _physics_file = physicsFile;
    _pose_file = poseFile;
    _user_data_file = userDataFile;
    _resolved_vtube_mapping_file = vtubeMappingFile;
    _package_checksum = packageChecksum;
    _referenced_file_count = referencedFileCount;
    _action_items = actionItems;
    emit validationChanged();
}
