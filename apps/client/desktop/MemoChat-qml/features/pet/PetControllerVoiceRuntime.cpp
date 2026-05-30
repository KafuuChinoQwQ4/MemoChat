#include "PetController.h"
#include "PetControllerPrivate.h"

#include <QFileInfo>
#include <QJsonObject>

using namespace memochat::pet;

void PetController::setModelSelection(const QString& modelType, const QString& modelName)
{
    const QString nextType = modelType.trimmed();
    const QString nextName = modelName.trimmed();
    if (_selected_model_type == nextType && _selected_model_name == nextName)
    {
        return;
    }
    _selected_model_type = nextType;
    _selected_model_name = nextName;
    emit modelSelectionChanged();
}

void PetController::setReplyLanguage(const QString& language)
{
    QString nextLanguage = language.trimmed();
    if (nextLanguage.isEmpty())
    {
        nextLanguage = QStringLiteral("zh-CN");
    }
    if (_reply_language == nextLanguage)
    {
        return;
    }
    _reply_language = nextLanguage;
    emit replyLanguageChanged();
}

void PetController::setSpeechRules(const QString& rules)
{
    const QString nextRules = rules.trimmed();
    if (_speech_rules == nextRules)
    {
        return;
    }
    _speech_rules = nextRules;
    emit speechRulesChanged();
}

void PetController::setVoiceRuntimeSettings(const QVariantMap& settings)
{
    QString referenceAudioPath = localPathFromUrlText(firstStringFromVariant(settings,
                                                                             {"referenceAudioPath",
                                                                              "reference_audio_path",
                                                                              "refAudioPath",
                                                                              "ref_audio_path",
                                                                              "voiceTrainingReferenceAudioPath"}));
    if (referenceAudioPath.isEmpty())
    {
        referenceAudioPath = joinedPath(firstStringFromVariant(settings, {"voiceDirectory", "referenceAudioDirectory"}),
                                        firstStringFromVariant(settings, {"defaultVoice", "referenceAudioFile"}));
    }
    if (referenceAudioPath.isEmpty())
    {
        const QString artifactPath =
            localPathFromUrlText(firstStringFromVariant(settings, {"voiceTrainingArtifactPath", "artifactPath"}));
        if (looksLikeAudioPath(artifactPath))
        {
            referenceAudioPath = artifactPath;
        }
    }
    QString referenceSource = firstStringFromVariant(settings, {"referenceAudioSource", "reference_audio_source"});
    if (_voice_reference_audio_source ==
        QStringLiteral("voice_training") &&
                       referenceSource != QStringLiteral("voice_training") &&
                                                         !_voice_reference_audio_path.trimmed().isEmpty())
    {
        referenceAudioPath = _voice_reference_audio_path.trimmed();
        referenceSource = _voice_reference_audio_source;
    }

    QString provider = firstStringFromVariant(settings, {"voiceProvider", "provider", "voice_provider"});
    if (provider.isEmpty() && !referenceAudioPath.isEmpty())
    {
        provider = QStringLiteral("gpt-sovits");
    }

    QString voiceName = firstStringFromVariant(settings, {"voiceName", "voice_name", "characterName"});
    if (voiceName.isEmpty())
    {
        const QString defaultVoice = firstStringFromVariant(settings, {"defaultVoice", "referenceAudioFile"});
        if (!defaultVoice.isEmpty())
        {
            voiceName = QFileInfo(defaultVoice).completeBaseName();
        }
    }
    if (voiceName.isEmpty() && !provider.isEmpty())
    {
        voiceName = provider;
    }

    QString language = firstStringFromVariant(settings, {"voiceLanguage", "voice_language", "language"});
    if (language.isEmpty())
    {
        language = _reply_language.trimmed().isEmpty() ? QStringLiteral("zh-CN") : _reply_language.trimmed();
    }
    const QString textLanguage = firstStringFromVariant(settings, {"textLanguage", "text_lang"});
    const QString promptLanguage = firstStringFromVariant(settings, {"promptLanguage", "prompt_lang"});
    const QString promptText = firstStringFromVariant(settings, {"promptText", "prompt_text"});

    const bool changed = _voice_provider != provider || _voice_name != voiceName || _voice_language != language ||
                         _voice_reference_audio_path != referenceAudioPath ||
                         _voice_reference_audio_source != referenceSource || _voice_prompt_text != promptText ||
                         _voice_prompt_language != promptLanguage || _voice_text_language != textLanguage;
    if (!changed)
    {
        return;
    }
    _voice_provider = provider;
    _voice_name = voiceName;
    _voice_language = language;
    _voice_reference_audio_path = referenceAudioPath;
    _voice_reference_audio_source = referenceSource;
    _voice_prompt_text = promptText;
    _voice_prompt_language = promptLanguage;
    _voice_text_language = textLanguage;
    emit stateChanged();
}

QJsonObject PetController::defaultObservationPayload() const
{
    QJsonObject audio;
    audio[QStringLiteral("vad")] = QStringLiteral("idle");
    audio[QStringLiteral("rms")] = 0.0;
    audio[QStringLiteral("interrupt")] = false;

    QJsonObject vision;
    vision[QStringLiteral("enabled")] = false;
    vision[QStringLiteral("mode")] = QStringLiteral("landmarks_only");
    vision[QStringLiteral("face_present")] = false;
    appendVoiceRuntimeMetadata(vision);

    QJsonObject privacy;
    privacy[QStringLiteral("raw_frame_sent")] = false;
    privacy[QStringLiteral("raw_audio_recorded")] = false;

    QJsonObject payload;
    payload[QStringLiteral("type")] = QStringLiteral("pet.observation");
    payload[QStringLiteral("audio")] = audio;
    payload[QStringLiteral("vision")] = vision;
    payload[QStringLiteral("privacy")] = privacy;
    return payload;
}

QJsonObject PetController::voiceRuntimeMetadata() const
{
    QJsonObject metadata;
    const QString provider = _voice_provider.trimmed();
    const QString referenceAudioPath = _voice_reference_audio_path.trimmed();
    if (provider.isEmpty() && referenceAudioPath.isEmpty())
    {
        return metadata;
    }
    if (!provider.isEmpty())
    {
        metadata[QStringLiteral("voice_provider")] = provider;
    }
    if (!_voice_name.trimmed().isEmpty())
    {
        metadata[QStringLiteral("voice_name")] = _voice_name.trimmed();
    }
    const QString voiceLanguage = _voice_language.trimmed().isEmpty()
        ? (_reply_language.trimmed().isEmpty() ? QStringLiteral("zh-CN") : _reply_language.trimmed())
           : _voice_language.trimmed();
    if (!voiceLanguage.isEmpty())
    {
        metadata[QStringLiteral("voice_language")] = voiceLanguage;
    }
    if (!referenceAudioPath.isEmpty())
    {
        metadata[QStringLiteral("ref_audio_path")] = referenceAudioPath;
    }
    if (!_voice_reference_audio_source.trimmed().isEmpty())
    {
        metadata[QStringLiteral("reference_audio_source")] = _voice_reference_audio_source.trimmed();
    }
    if (!_voice_prompt_text.trimmed().isEmpty())
    {
        metadata[QStringLiteral("prompt_text")] = _voice_prompt_text.trimmed();
    }
    if (!_voice_prompt_language.trimmed().isEmpty())
    {
        metadata[QStringLiteral("prompt_lang")] = _voice_prompt_language.trimmed();
    }
    metadata[QStringLiteral("text_lang")] = _voice_text_language.trimmed().isEmpty() ? voiceTextLanguage(voiceLanguage)
                                                                                     : _voice_text_language.trimmed();
    return metadata;
}

void PetController::appendVoiceRuntimeMetadata(QJsonObject& metadata) const
{
    const QJsonObject voiceMetadata = voiceRuntimeMetadata();
    for (auto it = voiceMetadata.constBegin(); it != voiceMetadata.constEnd(); ++it)
    {
        metadata.insert(it.key(), it.value());
    }
}
