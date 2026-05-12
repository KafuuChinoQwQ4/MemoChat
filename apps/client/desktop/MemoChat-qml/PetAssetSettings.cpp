#include "PetAssetSettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QtGlobal>

namespace {
constexpr int kSchemaVersion = 1;

QString stringValue(const QVariantMap &values, const QString &key, const QString &fallback)
{
    const QVariant value = values.value(key);
    return value.canConvert<QString>() ? value.toString() : fallback;
}

int intValue(const QVariantMap &values, const QString &key, int fallback, int minimum, int maximum)
{
    bool ok = false;
    const int parsed = values.value(key).toInt(&ok);
    return qBound(minimum, ok ? parsed : fallback, maximum);
}

qreal realValue(const QVariantMap &values, const QString &key, qreal fallback, qreal minimum, qreal maximum)
{
    bool ok = false;
    const qreal parsed = values.value(key).toDouble(&ok);
    return qBound(minimum, ok ? parsed : fallback, maximum);
}

bool boolValue(const QVariantMap &values, const QString &key, bool fallback)
{
    const QVariant value = values.value(key);
    if (value.typeId() == QMetaType::Bool) {
        return value.toBool();
    }
    if (value.canConvert<QString>()) {
        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("true") || text == QStringLiteral("1") || text == QStringLiteral("yes")) {
            return true;
        }
        if (text == QStringLiteral("false") || text == QStringLiteral("0") || text == QStringLiteral("no")) {
            return false;
        }
    }
    return fallback;
}
} // namespace

PetAssetSettings::PetAssetSettings(QObject *parent)
    : QObject(parent)
    , _storage_path(defaultStoragePath())
{
    applyDefaults(false);
}

bool PetAssetSettings::load()
{
    QFile file(_storage_path);
    if (!file.exists()) {
        applyDefaults(false);
        _status_text = QStringLiteral("暂无本地草稿");
        emit settingsChanged();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        _status_text = QStringLiteral("无法读取本地草稿：%1").arg(file.errorString());
        emit settingsChanged();
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        applyDefaults(false);
        _status_text = QStringLiteral("本地草稿格式错误，已使用默认值");
        emit settingsChanged();
        return false;
    }

    const QJsonObject object = document.object();
    applyObject(object.toVariantMap(), false);
    _status_text = QStringLiteral("已载入本地草稿");
    emit settingsChanged();
    return true;
}

bool PetAssetSettings::save()
{
    const QFileInfo info(_storage_path);
    QDir dir(info.absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        _status_text = QStringLiteral("无法创建草稿目录：%1").arg(info.absolutePath());
        emit settingsChanged();
        return false;
    }

    QFile file(_storage_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
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

void PetAssetSettings::resetToDefaults()
{
    applyDefaults(true);
    _status_text = QStringLiteral("已恢复默认草稿，资源路径留空");
    emit settingsChanged();
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
    return values;
}

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
void PetAssetSettings::setLanguageIndex(int value) { if (assignInt(_language_index, value, 0, 2)) markDirty(); }
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
    _character_name = QStringLiteral("Memo Pet");
    _role_identity = QStringLiteral("聊天陪伴角色");
    _model_root.clear();
    _model_json.clear();
    _motion_directory.clear();
    _expression_directory.clear();
    _voice_directory.clear();
    _default_voice = QStringLiteral("normal");
    _idle_motion = QStringLiteral("Idle");
    _speaking_motion = QStringLiteral("TapBody");
    _fallback_expression = QStringLiteral("neutral");
    _personality_tags = QStringLiteral("认真, 轻声, 可靠, 适度吐槽");
    _relationship_style = QStringLiteral("熟悉但不过界的同伴");
    _world_setting = QStringLiteral("住在 MemoChat 旁边的小小工作台，会在用户聊天、学习和整理资料时陪伴。");
    _speech_rules = QStringLiteral("用自然中文回复。少说套话，先回应情绪，再给明确建议。");
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
    _language_index = intValue(values, QStringLiteral("languageIndex"), _language_index, 0, 2);
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
