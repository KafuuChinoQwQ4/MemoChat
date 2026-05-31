#include "PetAssetSettings.h"

#include <QString>
#include <QtGlobal>

void PetAssetSettings::setVoiceTrainingConsent(bool value)
{
    if (assignBool(_voice_training_consent, value))
        markDirty();
}

void PetAssetSettings::setVoiceTrainingConsentScope(const QString& value)
{
    if (assignString(_voice_training_consent_scope, value))
        markDirty();
}

void PetAssetSettings::setVoiceTrainingJobId(const QString& value)
{
    if (assignString(_voice_training_job_id, value))
        markDirty();
}

void PetAssetSettings::setVoiceTrainingStatus(const QString& value)
{
    if (assignString(_voice_training_status, value))
        markDirty();
}

void PetAssetSettings::setVoiceTrainingStage(const QString& value)
{
    if (assignString(_voice_training_stage, value))
        markDirty();
}

void PetAssetSettings::setVoiceTrainingProgress(int value)
{
    if (assignInt(_voice_training_progress, value, 0, 100))
        markDirty();
}

void PetAssetSettings::setVoiceTrainingArtifactPath(const QString& value)
{
    if (assignString(_voice_training_artifact_path, value))
        markDirty();
}

void PetAssetSettings::setVoiceTrainingMessage(const QString& value)
{
    if (assignString(_voice_training_message, value))
        markDirty();
}

void PetAssetSettings::normalizeVoiceTrainingState()
{
    const QString normalizedStage = _voice_training_stage.trimmed().toLower();
    const QString normalizedStatus = _voice_training_status.trimmed().toLower();
    if (normalizedStage != QStringLiteral("ready_for_worker"))
    {
        if (normalizedStatus ==
            QStringLiteral("blocked") &&
                           (normalizedStage ==
                            QStringLiteral("reference_not_visible") ||
                                           normalizedStage == QStringLiteral("reference_unreadable") ||
                                                                             normalizedStage ==
                                                                                 QStringLiteral("reference_too_short")))
        {
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
    if (_voice_training_status ==
        QStringLiteral("prepared") || _voice_training_status == QStringLiteral("idle") || _voice_training_status ==
                                                                                              QStringLiteral("blocked"))
    {
        _voice_training_status = QStringLiteral("ready");
    }
    if (_voice_training_progress < 70)
    {
        _voice_training_progress = 70;
    }
    if (_voice_training_message.isEmpty() ||
        _voice_training_message.contains(QStringLiteral("worker"), Qt::CaseInsensitive))
    {
        _voice_training_message = QStringLiteral("声音参考已就绪，可直接用于 GPT-SoVITS 零样本合成。");
    }
}
