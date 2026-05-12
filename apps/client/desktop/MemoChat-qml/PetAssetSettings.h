#ifndef PETASSETSETTINGS_H
#define PETASSETSETTINGS_H

#include <QObject>
#include <QString>
#include <QVariantMap>

class PetAssetSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString characterName READ characterName WRITE setCharacterName NOTIFY settingsChanged)
    Q_PROPERTY(QString roleIdentity READ roleIdentity WRITE setRoleIdentity NOTIFY settingsChanged)
    Q_PROPERTY(QString modelRoot READ modelRoot WRITE setModelRoot NOTIFY settingsChanged)
    Q_PROPERTY(QString modelJson READ modelJson WRITE setModelJson NOTIFY settingsChanged)
    Q_PROPERTY(QString motionDirectory READ motionDirectory WRITE setMotionDirectory NOTIFY settingsChanged)
    Q_PROPERTY(QString expressionDirectory READ expressionDirectory WRITE setExpressionDirectory NOTIFY settingsChanged)
    Q_PROPERTY(QString voiceDirectory READ voiceDirectory WRITE setVoiceDirectory NOTIFY settingsChanged)
    Q_PROPERTY(QString defaultVoice READ defaultVoice WRITE setDefaultVoice NOTIFY settingsChanged)
    Q_PROPERTY(QString idleMotion READ idleMotion WRITE setIdleMotion NOTIFY settingsChanged)
    Q_PROPERTY(QString speakingMotion READ speakingMotion WRITE setSpeakingMotion NOTIFY settingsChanged)
    Q_PROPERTY(QString fallbackExpression READ fallbackExpression WRITE setFallbackExpression NOTIFY settingsChanged)
    Q_PROPERTY(QString personalityTags READ personalityTags WRITE setPersonalityTags NOTIFY settingsChanged)
    Q_PROPERTY(QString relationshipStyle READ relationshipStyle WRITE setRelationshipStyle NOTIFY settingsChanged)
    Q_PROPERTY(QString worldSetting READ worldSetting WRITE setWorldSetting NOTIFY settingsChanged)
    Q_PROPERTY(QString speechRules READ speechRules WRITE setSpeechRules NOTIFY settingsChanged)
    Q_PROPERTY(QString catchphrases READ catchphrases WRITE setCatchphrases NOTIFY settingsChanged)
    Q_PROPERTY(QString forbiddenRules READ forbiddenRules WRITE setForbiddenRules NOTIFY settingsChanged)
    Q_PROPERTY(int toneIndex READ toneIndex WRITE setToneIndex NOTIFY settingsChanged)
    Q_PROPERTY(int responseLengthIndex READ responseLengthIndex WRITE setResponseLengthIndex NOTIFY settingsChanged)
    Q_PROPERTY(int languageIndex READ languageIndex WRITE setLanguageIndex NOTIFY settingsChanged)
    Q_PROPERTY(qreal emotionLevel READ emotionLevel WRITE setEmotionLevel NOTIFY settingsChanged)
    Q_PROPERTY(qreal creativityLevel READ creativityLevel WRITE setCreativityLevel NOTIFY settingsChanged)
    Q_PROPERTY(qreal voiceSpeed READ voiceSpeed WRITE setVoiceSpeed NOTIFY settingsChanged)
    Q_PROPERTY(qreal lipSyncSensitivity READ lipSyncSensitivity WRITE setLipSyncSensitivity NOTIFY settingsChanged)
    Q_PROPERTY(bool voiceLipSyncEnabled READ voiceLipSyncEnabled WRITE setVoiceLipSyncEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool emotionSoundEnabled READ emotionSoundEnabled WRITE setEmotionSoundEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool idleMotionEnabled READ idleMotionEnabled WRITE setIdleMotionEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool gazeFollowEnabled READ gazeFollowEnabled WRITE setGazeFollowEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool memoryEnabled READ memoryEnabled WRITE setMemoryEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool interruptEnabled READ interruptEnabled WRITE setInterruptEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool cameraEnabled READ cameraEnabled WRITE setCameraEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool cloudVisionEnabled READ cloudVisionEnabled WRITE setCloudVisionEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString storagePath READ storagePath NOTIFY settingsChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY settingsChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY settingsChanged)

public:
    explicit PetAssetSettings(QObject *parent = nullptr);

    QString characterName() const { return _character_name; }
    QString roleIdentity() const { return _role_identity; }
    QString modelRoot() const { return _model_root; }
    QString modelJson() const { return _model_json; }
    QString motionDirectory() const { return _motion_directory; }
    QString expressionDirectory() const { return _expression_directory; }
    QString voiceDirectory() const { return _voice_directory; }
    QString defaultVoice() const { return _default_voice; }
    QString idleMotion() const { return _idle_motion; }
    QString speakingMotion() const { return _speaking_motion; }
    QString fallbackExpression() const { return _fallback_expression; }
    QString personalityTags() const { return _personality_tags; }
    QString relationshipStyle() const { return _relationship_style; }
    QString worldSetting() const { return _world_setting; }
    QString speechRules() const { return _speech_rules; }
    QString catchphrases() const { return _catchphrases; }
    QString forbiddenRules() const { return _forbidden_rules; }
    int toneIndex() const { return _tone_index; }
    int responseLengthIndex() const { return _response_length_index; }
    int languageIndex() const { return _language_index; }
    qreal emotionLevel() const { return _emotion_level; }
    qreal creativityLevel() const { return _creativity_level; }
    qreal voiceSpeed() const { return _voice_speed; }
    qreal lipSyncSensitivity() const { return _lip_sync_sensitivity; }
    bool voiceLipSyncEnabled() const { return _voice_lip_sync_enabled; }
    bool emotionSoundEnabled() const { return _emotion_sound_enabled; }
    bool idleMotionEnabled() const { return _idle_motion_enabled; }
    bool gazeFollowEnabled() const { return _gaze_follow_enabled; }
    bool memoryEnabled() const { return _memory_enabled; }
    bool interruptEnabled() const { return _interrupt_enabled; }
    bool cameraEnabled() const { return _camera_enabled; }
    bool cloudVisionEnabled() const { return _cloud_vision_enabled; }
    QString storagePath() const { return _storage_path; }
    bool dirty() const { return _dirty; }
    QString statusText() const { return _status_text; }

    Q_INVOKABLE bool load();
    Q_INVOKABLE bool save();
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE QVariantMap toVariantMap() const;

public slots:
    void setCharacterName(const QString &value);
    void setRoleIdentity(const QString &value);
    void setModelRoot(const QString &value);
    void setModelJson(const QString &value);
    void setMotionDirectory(const QString &value);
    void setExpressionDirectory(const QString &value);
    void setVoiceDirectory(const QString &value);
    void setDefaultVoice(const QString &value);
    void setIdleMotion(const QString &value);
    void setSpeakingMotion(const QString &value);
    void setFallbackExpression(const QString &value);
    void setPersonalityTags(const QString &value);
    void setRelationshipStyle(const QString &value);
    void setWorldSetting(const QString &value);
    void setSpeechRules(const QString &value);
    void setCatchphrases(const QString &value);
    void setForbiddenRules(const QString &value);
    void setToneIndex(int value);
    void setResponseLengthIndex(int value);
    void setLanguageIndex(int value);
    void setEmotionLevel(qreal value);
    void setCreativityLevel(qreal value);
    void setVoiceSpeed(qreal value);
    void setLipSyncSensitivity(qreal value);
    void setVoiceLipSyncEnabled(bool value);
    void setEmotionSoundEnabled(bool value);
    void setIdleMotionEnabled(bool value);
    void setGazeFollowEnabled(bool value);
    void setMemoryEnabled(bool value);
    void setInterruptEnabled(bool value);
    void setCameraEnabled(bool value);
    void setCloudVisionEnabled(bool value);

signals:
    void settingsChanged();

private:
    static bool assignString(QString &target, const QString &value);
    bool assignInt(int &target, int value, int minimum, int maximum);
    bool assignReal(qreal &target, qreal value, qreal minimum, qreal maximum);
    bool assignBool(bool &target, bool value);
    void applyDefaults(bool dirty);
    void applyObject(const QVariantMap &values, bool dirty);
    void markDirty();
    QString defaultStoragePath() const;

    QString _character_name;
    QString _role_identity;
    QString _model_root;
    QString _model_json;
    QString _motion_directory;
    QString _expression_directory;
    QString _voice_directory;
    QString _default_voice;
    QString _idle_motion;
    QString _speaking_motion;
    QString _fallback_expression;
    QString _personality_tags;
    QString _relationship_style;
    QString _world_setting;
    QString _speech_rules;
    QString _catchphrases;
    QString _forbidden_rules;
    int _tone_index = 0;
    int _response_length_index = 1;
    int _language_index = 0;
    qreal _emotion_level = 0.62;
    qreal _creativity_level = 0.48;
    qreal _voice_speed = 1.0;
    qreal _lip_sync_sensitivity = 0.55;
    bool _voice_lip_sync_enabled = true;
    bool _emotion_sound_enabled = true;
    bool _idle_motion_enabled = true;
    bool _gaze_follow_enabled = true;
    bool _memory_enabled = true;
    bool _interrupt_enabled = true;
    bool _camera_enabled = false;
    bool _cloud_vision_enabled = false;
    QString _storage_path;
    bool _dirty = false;
    QString _status_text;
};

#endif // PETASSETSETTINGS_H
