#include "PetController.h"
#include "PetControllerPrivate.h"

#include <QtCore/QIODevice>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QUrl>
#include <QtGlobal>

using namespace memochat::pet;

void PetController::startVoiceTraining(const QVariantMap& request)
{
    if (_voice_training_busy)
    {
        return;
    }
    QJsonObject payload = authPayload();
    const QJsonObject requestObject = QJsonObject::fromVariantMap(request);
    for (auto it = requestObject.constBegin(); it != requestObject.constEnd(); ++it)
    {
        payload.insert(it.key(), it.value());
    }
    if (payload.value(QStringLiteral("profile_id")).toString().trimmed().isEmpty())
    {
        payload[QStringLiteral("profile_id")] = accountProfileId();
    }
    if (!payload.value(QStringLiteral("consent_confirmed")).toBool(false))
    {
        setError(QStringLiteral("声音训练需要先确认参考音频授权"));
        _voice_training_status = QStringLiteral("blocked");
        _voice_training_message = QStringLiteral("请先勾选参考音频授权确认");
        emit voiceTrainingChanged();
        return;
    }
    QString referenceAudioPath = payload.value(QStringLiteral("reference_audio_path")).toString().trimmed();
    if (referenceAudioPath.startsWith(QStringLiteral("file://")))
    {
        referenceAudioPath = QUrl(referenceAudioPath).toLocalFile();
        payload[QStringLiteral("reference_audio_path")] = referenceAudioPath;
    }
    if (referenceAudioPath.isEmpty())
    {
        setError(QStringLiteral("声音训练缺少参考音频路径"));
        _voice_training_status = QStringLiteral("blocked");
        _voice_training_message = QStringLiteral("请先设置默认音频文件");
        emit voiceTrainingChanged();
        return;
    }
    QFileInfo referenceAudio(referenceAudioPath);
    if (referenceAudio.exists() && referenceAudio.isFile() && referenceAudio.size() > 0 &&
        referenceAudio.size() <= 32 * 1024 * 1024)
    {
        QFile file(referenceAudio.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly))
        {
            QJsonObject metadata = payload.value(QStringLiteral("metadata")).toObject();
            metadata[QStringLiteral("reference_audio_base64")] = QString::fromLatin1(file.readAll().toBase64());
            metadata[QStringLiteral("reference_audio_file_name")] = referenceAudio.fileName();
            metadata[QStringLiteral("reference_audio_size_bytes")] = qint64(referenceAudio.size());
            metadata[QStringLiteral("reference_audio_transfer")] = QStringLiteral("client_base64");
            payload[QStringLiteral("metadata")] = metadata;
        }
    }
    setError({});
    setVoiceTrainingBusy(true, QStringLiteral("正在提交声音训练准备任务"));
    postJson(petUrl(QStringLiteral("/voice-training/jobs")), payload, QStringLiteral("voice_training_create"));
}

void PetController::refreshVoiceTrainingJob(const QString& jobId)
{
    const QString selectedJobId = jobId.trimmed().isEmpty() ? _voice_training_job_id : jobId.trimmed();
    if (selectedJobId.isEmpty() || _voice_training_busy)
    {
        return;
    }
    setError({});
    setVoiceTrainingBusy(true, QStringLiteral("正在刷新声音训练任务"));
    getJson(petUrl(
        QStringLiteral("/voice-training/jobs/%1").arg(QString::fromLatin1(QUrl::toPercentEncoding(selectedJobId)))),
        QStringLiteral("voice_training_get"));
}

void PetController::applyVoiceTrainingJob(const QJsonObject& job)
{
    if (job.isEmpty())
    {
        setError(QStringLiteral("声音训练响应缺少任务信息"));
        return;
    }
    _voice_training_job_id = job.value(QStringLiteral("job_id")).toString();
    _voice_training_status = job.value(QStringLiteral("status")).toString(QStringLiteral("prepared"));
    _voice_training_stage = job.value(QStringLiteral("stage")).toString();
    _voice_training_progress = qBound(0, job.value(QStringLiteral("progress")).toInt(0), 100);
    _voice_training_artifact_path = job.value(QStringLiteral("artifact_path")).toString();
    _voice_training_message = job.value(QStringLiteral("message")).toString(QStringLiteral("声音训练任务已准备"));
    const QJsonObject diagnostics = job.value(QStringLiteral("diagnostics")).toObject();
    const QString trainedReferenceAudio =
        diagnostics.value(QStringLiteral("gpt_sovits_reference_audio")).toString().trimmed().isEmpty()
        ? diagnostics.value(QStringLiteral("reference_audio_runtime_path")).toString().trimmed()
        : diagnostics.value(QStringLiteral("gpt_sovits_reference_audio")).toString().trimmed();
    if (!trainedReferenceAudio.isEmpty())
    {
        _voice_reference_audio_path = trainedReferenceAudio;
        _voice_reference_audio_source = QStringLiteral("voice_training");
        _voice_provider = QStringLiteral("gpt-sovits");
        const QString trainedVoiceName = job.value(QStringLiteral("voice_name")).toString().trimmed();
        if (!trainedVoiceName.isEmpty())
        {
            _voice_name = trainedVoiceName;
        }
        const QString trainedLanguage = job.value(QStringLiteral("language")).toString().trimmed();
        if (!trainedLanguage.isEmpty())
        {
            _voice_language = trainedLanguage;
            if (_voice_text_language.isEmpty())
            {
                _voice_text_language = voiceTextLanguage(trainedLanguage);
            }
        }
    }
    if (_voice_training_stage == QStringLiteral("ready_for_worker"))
    {
        _voice_training_stage = QStringLiteral("ready_for_gpt_sovits");
        if (_voice_training_status ==
            QStringLiteral("prepared") ||
                           _voice_training_status == QStringLiteral("idle") ||
                                                                    _voice_training_status == QStringLiteral("blocked"))
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
    setStatusText(_voice_training_message);
    emit voiceTrainingChanged();
}
