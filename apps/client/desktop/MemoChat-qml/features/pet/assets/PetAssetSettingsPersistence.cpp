#include "PetAssetSettings.h"
#include "PetAssetSettingsPrivate.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>

using namespace memochat::pet_asset_settings;

bool PetAssetSettings::load()
{
    QFile file(_storage_path);
    if (!file.exists())
    {
        applyDefaults(false);
        _status_text = QStringLiteral("暂无本地草稿");
        emit settingsChanged();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        _status_text = QStringLiteral("无法读取本地草稿：%1").arg(file.errorString());
        emit settingsChanged();
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
    {
        applyDefaults(false);
        _status_text = QStringLiteral("本地草稿格式错误，已使用默认值");
        emit settingsChanged();
        return false;
    }

    const QJsonObject object = document.object();
    applyObject(object.toVariantMap(), false);
    const bool migrated = migrateLegacyBundledLive2DPaths();
    normalizeVoiceTrainingState();
    if (migrated)
    {
        _dirty = true;
        const bool saved = save();
        _status_text = saved ? QStringLiteral("已载入本地草稿，已迁移内置 Live2D 资源路径")
                             : QStringLiteral("已载入本地草稿，内置 Live2D 资源路径已在内存中迁移");
    }
    else
    {
        _status_text = QStringLiteral("已载入本地草稿");
    }
    emit settingsChanged();
    return true;
}

bool PetAssetSettings::save()
{
    const QFileInfo info(_storage_path);
    QDir dir(info.absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
    {
        _status_text = QStringLiteral("无法创建草稿目录：%1").arg(info.absolutePath());
        emit settingsChanged();
        return false;
    }

    QFile file(_storage_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        _status_text = QStringLiteral("无法写入本地草稿：%1").arg(file.errorString());
        emit settingsChanged();
        return false;
    }

    const QJsonDocument document(QJsonObject::fromVariantMap(toVariantMap()));
    file.write(document.toJson(QJsonDocument::Indented));
    _dirty = false;
    _status_text = QStringLiteral("本地草稿已保存");
    emit settingsChanged();
    return true;
}

QVariantMap PetAssetSettings::toVariantMap() const
{
    QVariantMap values;
    values[QStringLiteral("schema_version")] = kSchemaVersion;
    values[QStringLiteral("characterName")] = _character_name;
    values[QStringLiteral("roleIdentity")] = _role_identity;
    values[QStringLiteral("modelRoot")] = _model_root;
    values[QStringLiteral("modelJson")] = _model_json;
    values[QStringLiteral("motionDirectory")] = _motion_directory;
    values[QStringLiteral("expressionDirectory")] = _expression_directory;
    values[QStringLiteral("voiceDirectory")] = _voice_directory;
    values[QStringLiteral("defaultVoice")] = _default_voice;
    values[QStringLiteral("idleMotion")] = _idle_motion;
    values[QStringLiteral("speakingMotion")] = _speaking_motion;
    values[QStringLiteral("fallbackExpression")] = _fallback_expression;
    values[QStringLiteral("personalityTags")] = _personality_tags;
    values[QStringLiteral("relationshipStyle")] = _relationship_style;
    values[QStringLiteral("worldSetting")] = _world_setting;
    values[QStringLiteral("speechRules")] = _speech_rules;
    values[QStringLiteral("catchphrases")] = _catchphrases;
    values[QStringLiteral("forbiddenRules")] = _forbidden_rules;
    values[QStringLiteral("toneIndex")] = _tone_index;
    values[QStringLiteral("responseLengthIndex")] = _response_length_index;
    values[QStringLiteral("languageIndex")] = _language_index;
    values[QStringLiteral("emotionLevel")] = _emotion_level;
    values[QStringLiteral("creativityLevel")] = _creativity_level;
    values[QStringLiteral("voiceSpeed")] = _voice_speed;
    values[QStringLiteral("lipSyncSensitivity")] = _lip_sync_sensitivity;
    values[QStringLiteral("voiceLipSyncEnabled")] = _voice_lip_sync_enabled;
    values[QStringLiteral("emotionSoundEnabled")] = _emotion_sound_enabled;
    values[QStringLiteral("idleMotionEnabled")] = _idle_motion_enabled;
    values[QStringLiteral("gazeFollowEnabled")] = _gaze_follow_enabled;
    values[QStringLiteral("memoryEnabled")] = _memory_enabled;
    values[QStringLiteral("interruptEnabled")] = _interrupt_enabled;
    values[QStringLiteral("cameraEnabled")] = _camera_enabled;
    values[QStringLiteral("cloudVisionEnabled")] = _cloud_vision_enabled;
    values[QStringLiteral("autoStartPetOnClientStart")] = _auto_start_pet_on_client_start;
    values[QStringLiteral("voiceTrainingConsent")] = _voice_training_consent;
    values[QStringLiteral("voiceTrainingConsentScope")] = _voice_training_consent_scope;
    values[QStringLiteral("voiceTrainingJobId")] = _voice_training_job_id;
    values[QStringLiteral("voiceTrainingStatus")] = _voice_training_status;
    values[QStringLiteral("voiceTrainingStage")] = _voice_training_stage;
    values[QStringLiteral("voiceTrainingProgress")] = _voice_training_progress;
    values[QStringLiteral("voiceTrainingArtifactPath")] = _voice_training_artifact_path;
    values[QStringLiteral("voiceTrainingMessage")] = _voice_training_message;
    return values;
}
