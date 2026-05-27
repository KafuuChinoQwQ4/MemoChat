#include "PetAssetSettings.h"
#include "PetAssetSettingsPrivate.h"

#include <QDir>
#include <QStandardPaths>
#include <QtGlobal>

using namespace memochat::pet_asset_settings;

void PetAssetSettings::setCharacterName(const QString &value) { if (assignString(_character_name, value)) markDirty(); }
void PetAssetSettings::setRoleIdentity(const QString &value) { if (assignString(_role_identity, value)) markDirty(); }
void PetAssetSettings::setModelRoot(const QString &value) { if (assignString(_model_root, value)) markDirty(); }
void PetAssetSettings::setModelJson(const QString &value) { if (assignString(_model_json, value)) markDirty(); }
void PetAssetSettings::setMotionDirectory(const QString &value) { if (assignString(_motion_directory, value)) markDirty(); }
void PetAssetSettings::setExpressionDirectory(const QString &value) { if (assignString(_expression_directory, value)) markDirty(); }
void PetAssetSettings::setVoiceDirectory(const QString &value) { if (assignString(_voice_directory, value)) markDirty(); }
void PetAssetSettings::setDefaultVoice(const QString &value) { if (assignString(_default_voice, value)) markDirty(); }
void PetAssetSettings::setIdleMotion(const QString &value) { if (assignString(_idle_motion, value)) markDirty(); }
void PetAssetSettings::setSpeakingMotion(const QString &value) { if (assignString(_speaking_motion, value)) markDirty(); }
void PetAssetSettings::setFallbackExpression(const QString &value) { if (assignString(_fallback_expression, value)) markDirty(); }
void PetAssetSettings::setPersonalityTags(const QString &value) { if (assignString(_personality_tags, value)) markDirty(); }
void PetAssetSettings::setRelationshipStyle(const QString &value) { if (assignString(_relationship_style, value)) markDirty(); }
void PetAssetSettings::setWorldSetting(const QString &value) { if (assignString(_world_setting, value)) markDirty(); }
void PetAssetSettings::setSpeechRules(const QString &value) { if (assignString(_speech_rules, value)) markDirty(); }
void PetAssetSettings::setCatchphrases(const QString &value) { if (assignString(_catchphrases, value)) markDirty(); }
void PetAssetSettings::setForbiddenRules(const QString &value) { if (assignString(_forbidden_rules, value)) markDirty(); }
void PetAssetSettings::setToneIndex(int value) { if (assignInt(_tone_index, value, 0, 3)) markDirty(); }
void PetAssetSettings::setResponseLengthIndex(int value) { if (assignInt(_response_length_index, value, 0, 2)) markDirty(); }
void PetAssetSettings::setLanguageIndex(int value) { if (assignInt(_language_index, value, 0, kLanguageOptionCount - 1)) markDirty(); }
void PetAssetSettings::setEmotionLevel(qreal value) { if (assignReal(_emotion_level, value, 0.0, 1.0)) markDirty(); }
void PetAssetSettings::setCreativityLevel(qreal value) { if (assignReal(_creativity_level, value, 0.0, 1.0)) markDirty(); }
void PetAssetSettings::setVoiceSpeed(qreal value) { if (assignReal(_voice_speed, value, 0.70, 1.35)) markDirty(); }
void PetAssetSettings::setLipSyncSensitivity(qreal value) { if (assignReal(_lip_sync_sensitivity, value, 0.0, 1.0)) markDirty(); }
void PetAssetSettings::setVoiceLipSyncEnabled(bool value) { if (assignBool(_voice_lip_sync_enabled, value)) markDirty(); }
void PetAssetSettings::setEmotionSoundEnabled(bool value) { if (assignBool(_emotion_sound_enabled, value)) markDirty(); }
void PetAssetSettings::setIdleMotionEnabled(bool value) { if (assignBool(_idle_motion_enabled, value)) markDirty(); }
void PetAssetSettings::setGazeFollowEnabled(bool value) { if (assignBool(_gaze_follow_enabled, value)) markDirty(); }
void PetAssetSettings::setMemoryEnabled(bool value) { if (assignBool(_memory_enabled, value)) markDirty(); }
void PetAssetSettings::setInterruptEnabled(bool value) { if (assignBool(_interrupt_enabled, value)) markDirty(); }
void PetAssetSettings::setCameraEnabled(bool value) { if (assignBool(_camera_enabled, value)) markDirty(); }
void PetAssetSettings::setCloudVisionEnabled(bool value) { if (assignBool(_cloud_vision_enabled, value)) markDirty(); }
void PetAssetSettings::setAutoStartPetOnClientStart(bool value) { if (assignBool(_auto_start_pet_on_client_start, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingConsent(bool value) { if (assignBool(_voice_training_consent, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingConsentScope(const QString &value) { if (assignString(_voice_training_consent_scope, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingJobId(const QString &value) { if (assignString(_voice_training_job_id, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingStatus(const QString &value) { if (assignString(_voice_training_status, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingStage(const QString &value) { if (assignString(_voice_training_stage, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingProgress(int value) { if (assignInt(_voice_training_progress, value, 0, 100)) markDirty(); }
void PetAssetSettings::setVoiceTrainingArtifactPath(const QString &value) { if (assignString(_voice_training_artifact_path, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingMessage(const QString &value) { if (assignString(_voice_training_message, value)) markDirty(); }

void PetAssetSettings::normalizeVoiceTrainingState()
{
    const QString normalizedStage = _voice_training_stage.trimmed().toLower();
    const QString normalizedStatus = _voice_training_status.trimmed().toLower();
    if (normalizedStage != QStringLiteral("ready_for_worker")) {
        if (normalizedStatus == QStringLiteral("blocked")
            && (normalizedStage == QStringLiteral("reference_not_visible")
                || normalizedStage == QStringLiteral("reference_unreadable")
                || normalizedStage == QStringLiteral("reference_too_short"))) {
            _voice_training_job_id.clear();
            _voice_training_status = QStringLiteral("idle");
            _voice_training_stage.clear();
            _voice_training_progress = 0;
            _voice_training_artifact_path.clear();
            _voice_training_message = QStringLiteral("等待确认参考音频权限");
        }
        return;
    }

    _voice_training_stage = QStringLiteral("ready_for_gpt_sovits");
    if (_voice_training_status == QStringLiteral("prepared")
        || _voice_training_status == QStringLiteral("idle")
        || _voice_training_status == QStringLiteral("blocked")) {
        _voice_training_status = QStringLiteral("ready");
    }
    if (_voice_training_progress < 70) {
        _voice_training_progress = 70;
    }
    if (_voice_training_message.isEmpty()
        || _voice_training_message.contains(QStringLiteral("worker"), Qt::CaseInsensitive)) {
        _voice_training_message = QStringLiteral("声音参考已就绪，可直接用于 GPT-SoVITS 零样本合成。");
    }
}

bool PetAssetSettings::assignString(QString &target, const QString &value)
{
    if (target == value) {
        return false;
    }
    target = value;
    return true;
}

bool PetAssetSettings::assignInt(int &target, int value, int minimum, int maximum)
{
    const int bounded = qBound(minimum, value, maximum);
    if (target == bounded) {
        return false;
    }
    target = bounded;
    return true;
}

bool PetAssetSettings::assignReal(qreal &target, qreal value, qreal minimum, qreal maximum)
{
    const qreal bounded = qBound(minimum, value, maximum);
    if (qFuzzyCompare(target + 1.0, bounded + 1.0)) {
        return false;
    }
    target = bounded;
    return true;
}

bool PetAssetSettings::assignBool(bool &target, bool value)
{
    if (target == value) {
        return false;
    }
    target = value;
    return true;
}

void PetAssetSettings::applyDefaults(bool dirty)
{
    _character_name = QStringLiteral("香风智乃");
    _role_identity = QStringLiteral("Live2D 桌宠助手");
    _model_root = clientSourcePath(QStringLiteral("src/KafuuChino/香风智乃live2D"));
    _model_json = clientSourcePath(QStringLiteral("src/KafuuChino/香风智乃live2D/香风智乃.model3.json"));
    _motion_directory = _model_root;
    _expression_directory = _model_root;
    _voice_directory = clientSourcePath(QStringLiteral("src/KafuuChino/香风智乃voice"));
    _default_voice = QStringLiteral("Kafuuchino-voice.mp3");
    _idle_motion = QStringLiteral("Idle");
    _speaking_motion = QStringLiteral("Talk");
    _fallback_expression = QStringLiteral("脸红");
    _personality_tags = QStringLiteral("认真, 安静, 可靠, 轻声提醒");
    _relationship_style = QStringLiteral("熟悉但不过界的桌面同伴");
    _world_setting = QStringLiteral("以香风智乃素材作为本地默认角色，在 MemoChat 旁边陪用户聊天、学习和整理资料。");
    _speech_rules = QStringLiteral("使用当前选择的单一语言回复，不混用中文、日语或英语。少说套话，先回应情绪，再给明确建议。");
    _catchphrases = QStringLiteral("收到，我会记住。\n先别急，我们一步一步来。");
    _forbidden_rules = QStringLiteral("不要伪装成真人；不要主动索要隐私；不替用户做高风险决定。");
    _tone_index = 0;
    _response_length_index = 1;
    _language_index = 0;
    _emotion_level = 0.62;
    _creativity_level = 0.48;
    _voice_speed = 1.0;
    _lip_sync_sensitivity = 0.55;
    _voice_lip_sync_enabled = true;
    _emotion_sound_enabled = true;
    _idle_motion_enabled = true;
    _gaze_follow_enabled = true;
    _memory_enabled = true;
    _interrupt_enabled = true;
    _camera_enabled = false;
    _cloud_vision_enabled = false;
    _auto_start_pet_on_client_start = false;
    _voice_training_consent = false;
    _voice_training_consent_scope = QStringLiteral("local_default_reference");
    _voice_training_job_id.clear();
    _voice_training_status = QStringLiteral("idle");
    _voice_training_stage.clear();
    _voice_training_progress = 0;
    _voice_training_artifact_path.clear();
    _voice_training_message = QStringLiteral("等待确认参考音频权限");
    _dirty = dirty;
    _status_text = dirty ? QStringLiteral("默认草稿待保存") : QStringLiteral("默认草稿已载入");
}

void PetAssetSettings::applyObject(const QVariantMap &values, bool dirty)
{
    applyDefaults(dirty);
    _character_name = stringValue(values, QStringLiteral("characterName"), _character_name);
    _role_identity = stringValue(values, QStringLiteral("roleIdentity"), _role_identity);
    _model_root = stringValue(values, QStringLiteral("modelRoot"), _model_root);
    _model_json = stringValue(values, QStringLiteral("modelJson"), _model_json);
    _motion_directory = stringValue(values, QStringLiteral("motionDirectory"), _motion_directory);
    _expression_directory = stringValue(values, QStringLiteral("expressionDirectory"), _expression_directory);
    _voice_directory = stringValue(values, QStringLiteral("voiceDirectory"), _voice_directory);
    _default_voice = stringValue(values, QStringLiteral("defaultVoice"), _default_voice);
    _idle_motion = stringValue(values, QStringLiteral("idleMotion"), _idle_motion);
    _speaking_motion = stringValue(values, QStringLiteral("speakingMotion"), _speaking_motion);
    _fallback_expression = stringValue(values, QStringLiteral("fallbackExpression"), _fallback_expression);
    _personality_tags = stringValue(values, QStringLiteral("personalityTags"), _personality_tags);
    _relationship_style = stringValue(values, QStringLiteral("relationshipStyle"), _relationship_style);
    _world_setting = stringValue(values, QStringLiteral("worldSetting"), _world_setting);
    _speech_rules = stringValue(values, QStringLiteral("speechRules"), _speech_rules);
    _catchphrases = stringValue(values, QStringLiteral("catchphrases"), _catchphrases);
    _forbidden_rules = stringValue(values, QStringLiteral("forbiddenRules"), _forbidden_rules);
    _tone_index = intValue(values, QStringLiteral("toneIndex"), _tone_index, 0, 3);
    _response_length_index = intValue(values, QStringLiteral("responseLengthIndex"), _response_length_index, 0, 2);
    _language_index = intValue(values, QStringLiteral("languageIndex"), _language_index, 0, kLanguageOptionCount - 1);
    _emotion_level = realValue(values, QStringLiteral("emotionLevel"), _emotion_level, 0.0, 1.0);
    _creativity_level = realValue(values, QStringLiteral("creativityLevel"), _creativity_level, 0.0, 1.0);
    _voice_speed = realValue(values, QStringLiteral("voiceSpeed"), _voice_speed, 0.70, 1.35);
    _lip_sync_sensitivity = realValue(values, QStringLiteral("lipSyncSensitivity"), _lip_sync_sensitivity, 0.0, 1.0);
    _voice_lip_sync_enabled = boolValue(values, QStringLiteral("voiceLipSyncEnabled"), _voice_lip_sync_enabled);
    _emotion_sound_enabled = boolValue(values, QStringLiteral("emotionSoundEnabled"), _emotion_sound_enabled);
    _idle_motion_enabled = boolValue(values, QStringLiteral("idleMotionEnabled"), _idle_motion_enabled);
    _gaze_follow_enabled = boolValue(values, QStringLiteral("gazeFollowEnabled"), _gaze_follow_enabled);
    _memory_enabled = boolValue(values, QStringLiteral("memoryEnabled"), _memory_enabled);
    _interrupt_enabled = boolValue(values, QStringLiteral("interruptEnabled"), _interrupt_enabled);
    _camera_enabled = boolValue(values, QStringLiteral("cameraEnabled"), _camera_enabled);
    _cloud_vision_enabled = boolValue(values, QStringLiteral("cloudVisionEnabled"), _cloud_vision_enabled);
    _auto_start_pet_on_client_start = boolValue(values, QStringLiteral("autoStartPetOnClientStart"), _auto_start_pet_on_client_start);
    _voice_training_consent = boolValue(values, QStringLiteral("voiceTrainingConsent"), _voice_training_consent);
    _voice_training_consent_scope = stringValue(values, QStringLiteral("voiceTrainingConsentScope"), _voice_training_consent_scope);
    _voice_training_job_id = stringValue(values, QStringLiteral("voiceTrainingJobId"), _voice_training_job_id);
    _voice_training_status = stringValue(values, QStringLiteral("voiceTrainingStatus"), _voice_training_status);
    _voice_training_stage = stringValue(values, QStringLiteral("voiceTrainingStage"), _voice_training_stage);
    _voice_training_progress = intValue(values, QStringLiteral("voiceTrainingProgress"), _voice_training_progress, 0, 100);
    _voice_training_artifact_path = stringValue(values, QStringLiteral("voiceTrainingArtifactPath"), _voice_training_artifact_path);
    _voice_training_message = stringValue(values, QStringLiteral("voiceTrainingMessage"), _voice_training_message);
    _dirty = dirty;
}

void PetAssetSettings::markDirty()
{
    _dirty = true;
    _status_text = QStringLiteral("本地草稿待保存");
    emit settingsChanged();
}

QString PetAssetSettings::defaultStoragePath() const
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty()) {
        root = QDir::home().absoluteFilePath(QStringLiteral(".memochat"));
    }
    return QDir(root).absoluteFilePath(QStringLiteral("pet/live2d-character-draft.json"));
}
