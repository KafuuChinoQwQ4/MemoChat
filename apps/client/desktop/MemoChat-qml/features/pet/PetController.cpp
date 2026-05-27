#include "PetController.h"
#include "PetControllerPrivate.h"

#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QtCore/QIODevice>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>
#include <QUrlQuery>
#include <QtGlobal>

using namespace memochat::pet;

PetController::PetController(ClientGateway *gateway, QObject *parent)
    : QObject(parent)
    , _gateway(gateway)
{
    connect(&_model, &PetModel::changed, this, &PetController::petStateChanged);
}

PetController::~PetController()
{
    stopStream();
    if (_windows_camera_bridge_process) {
        auto *process = _windows_camera_bridge_process.data();
        _windows_camera_bridge_process.clear();
        process->disconnect(this);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(250);
        }
        process->deleteLater();
    }
}

QString PetController::audioUrl() const
{
    const QString raw = _model.audioUrl().trimmed();
    if (raw.isEmpty()) {
        return {};
    }
    const QUrl parsed(raw);
    if (parsed.isValid() && !parsed.isRelative()) {
        return raw;
    }
    const QString path = raw.startsWith(QLatin1Char('/')) ? raw : QStringLiteral("/%1").arg(raw);
    if (path.startsWith(QStringLiteral("/audio/"))) {
        const QString mediaBase = gate_media_url_prefix.trimmed().isEmpty()
                                      ? gate_url_prefix.trimmed()
                                      : gate_media_url_prefix.trimmed();
        QUrl base(mediaBase);
        if (base.isValid() && !base.host().isEmpty()) {
            const int port = base.port(8096);
            if (port == 8096) {
                base.setPath(QStringLiteral("/pet") + path);
            } else if (port == 80 || port == 8080 || port == 8443) {
                base.setScheme(QStringLiteral("http"));
                base.setPort(8096);
                base.setPath(QStringLiteral("/pet") + path);
            } else {
                base.setPath(QStringLiteral("/ai/pet") + path);
            }
            QUrlQuery query(base);
            if (!_model.turnId().isEmpty()) {
                query.addQueryItem(QStringLiteral("turn"), _model.turnId());
            }
            if (!_model.eventId().isEmpty()) {
                query.addQueryItem(QStringLiteral("event"), _model.eventId());
            }
            base.setQuery(query);
            return base.toString();
        }
    }
    QUrl url = petUrl(path);
    QUrlQuery query(url);
    if (!_model.turnId().isEmpty()) {
        query.addQueryItem(QStringLiteral("turn"), _model.turnId());
    }
    if (!_model.eventId().isEmpty()) {
        query.addQueryItem(QStringLiteral("event"), _model.eventId());
    }
    url.setQuery(query);
    return url.toString();
}

void PetController::startSession()
{
    if (_busy) {
        return;
    }
    resetControlEventDedupe();
    setError({});
    setBusy(true, QStringLiteral("正在连接桌宠"));
    QJsonObject payload = authPayload();
    payload[QStringLiteral("profile_id")] = QStringLiteral("default");
    payload[QStringLiteral("persona")] = QStringLiteral("memo-pet");
    payload[QStringLiteral("provider")] = QStringLiteral("scripted");
    postJson(petUrl(QStringLiteral("/sessions")), payload, QStringLiteral("create_session"));
}

void PetController::sendText(const QString &text)
{
    const QString content = text.trimmed();
    if (content.isEmpty()) {
        return;
    }
    if (_input_request_in_flight) {
        setStatusText(QStringLiteral("上一条消息仍在发送"));
        return;
    }
    if (_session_id.isEmpty()) {
        setError(QStringLiteral("桌宠会话未连接"));
        return;
    }
    if (!_streaming) {
        startStream();
    }

    _model.clearSpeech();
    _input_request_in_flight = true;
    setBusy(true, QStringLiteral("正在发送"));
    QJsonObject payload = authPayload();
    payload[QStringLiteral("content")] = content;
    if (!_selected_model_type.trimmed().isEmpty()) {
        payload[QStringLiteral("model_type")] = _selected_model_type.trimmed();
    }
    if (!_selected_model_name.trimmed().isEmpty()) {
        payload[QStringLiteral("model_name")] = _selected_model_name.trimmed();
    }
    QJsonObject metadata;
    metadata[QStringLiteral("reply_language")] = _reply_language.trimmed().isEmpty()
                                                    ? QStringLiteral("zh-CN")
                                                    : _reply_language.trimmed();
    const QString speechRules = _speech_rules.trimmed();
    if (!speechRules.isEmpty()) {
        metadata[QStringLiteral("speech_rules")] = speechRules;
    }
    appendVoiceRuntimeMetadata(metadata);
    payload[QStringLiteral("metadata")] = metadata;
    postJson(petUrl(QStringLiteral("/sessions/%1/input").arg(encodedSessionId())),
             payload,
             QStringLiteral("input"));
}

void PetController::sendObservation(const QVariantMap &observation)
{
    if (_session_id.isEmpty()) {
        return;
    }
    QJsonObject payload = QJsonObject::fromVariantMap(observation);
    if (payload.isEmpty()) {
        payload = defaultObservationPayload();
    }
    QJsonObject vision = payload.value(QStringLiteral("vision")).toObject();
    appendVoiceRuntimeMetadata(vision);
    if (!vision.isEmpty()) {
        payload[QStringLiteral("vision")] = vision;
    }
    postJson(petUrl(QStringLiteral("/sessions/%1/observation").arg(encodedSessionId())),
             payload,
             QStringLiteral("observation"));
}

void PetController::interrupt()
{
    if (_session_id.isEmpty()) {
        return;
    }
    postJson(petUrl(QStringLiteral("/sessions/%1/interrupt").arg(encodedSessionId())),
             QJsonObject{},
             QStringLiteral("interrupt"));
}

void PetController::stopStream()
{
    if (_stream_reply) {
        auto *reply = _stream_reply.data();
        _stream_reply.clear();
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    _stream_buffer.clear();
    if (_streaming) {
        _streaming = false;
        emit stateChanged();
    }
}

void PetController::clearSpeech()
{
    _model.clearSpeech();
}

void PetController::startVoiceTraining(const QVariantMap &request)
{
    if (_voice_training_busy) {
        return;
    }
    QJsonObject payload = authPayload();
    const QJsonObject requestObject = QJsonObject::fromVariantMap(request);
    for (auto it = requestObject.constBegin(); it != requestObject.constEnd(); ++it) {
        payload.insert(it.key(), it.value());
    }
    if (!payload.value(QStringLiteral("consent_confirmed")).toBool(false)) {
        setError(QStringLiteral("声音训练需要先确认参考音频授权"));
        _voice_training_status = QStringLiteral("blocked");
        _voice_training_message = QStringLiteral("请先勾选参考音频授权确认");
        emit voiceTrainingChanged();
        return;
    }
    QString referenceAudioPath = payload.value(QStringLiteral("reference_audio_path")).toString().trimmed();
    if (referenceAudioPath.startsWith(QStringLiteral("file://"))) {
        referenceAudioPath = QUrl(referenceAudioPath).toLocalFile();
        payload[QStringLiteral("reference_audio_path")] = referenceAudioPath;
    }
    if (referenceAudioPath.isEmpty()) {
        setError(QStringLiteral("声音训练缺少参考音频路径"));
        _voice_training_status = QStringLiteral("blocked");
        _voice_training_message = QStringLiteral("请先设置默认音频文件");
        emit voiceTrainingChanged();
        return;
    }
    QFileInfo referenceAudio(referenceAudioPath);
    if (referenceAudio.exists() && referenceAudio.isFile() && referenceAudio.size() > 0
        && referenceAudio.size() <= 32 * 1024 * 1024) {
        QFile file(referenceAudio.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonObject metadata = payload.value(QStringLiteral("metadata")).toObject();
            metadata[QStringLiteral("reference_audio_base64")] =
                QString::fromLatin1(file.readAll().toBase64());
            metadata[QStringLiteral("reference_audio_file_name")] = referenceAudio.fileName();
            metadata[QStringLiteral("reference_audio_size_bytes")] = qint64(referenceAudio.size());
            metadata[QStringLiteral("reference_audio_transfer")] = QStringLiteral("client_base64");
            payload[QStringLiteral("metadata")] = metadata;
        }
    }
    setError({});
    setVoiceTrainingBusy(true, QStringLiteral("正在提交声音训练准备任务"));
    postJson(petUrl(QStringLiteral("/voice-training/jobs")),
             payload,
             QStringLiteral("voice_training_create"));
}

void PetController::setModelSelection(const QString &modelType, const QString &modelName)
{
    const QString nextType = modelType.trimmed();
    const QString nextName = modelName.trimmed();
    if (_selected_model_type == nextType && _selected_model_name == nextName) {
        return;
    }
    _selected_model_type = nextType;
    _selected_model_name = nextName;
    emit modelSelectionChanged();
}

void PetController::setReplyLanguage(const QString &language)
{
    QString nextLanguage = language.trimmed();
    if (nextLanguage.isEmpty()) {
        nextLanguage = QStringLiteral("zh-CN");
    }
    if (_reply_language == nextLanguage) {
        return;
    }
    _reply_language = nextLanguage;
    emit replyLanguageChanged();
}

void PetController::setSpeechRules(const QString &rules)
{
    const QString nextRules = rules.trimmed();
    if (_speech_rules == nextRules) {
        return;
    }
    _speech_rules = nextRules;
    emit speechRulesChanged();
}

void PetController::setVoiceRuntimeSettings(const QVariantMap &settings)
{
    QString referenceAudioPath = localPathFromUrlText(firstStringFromVariant(settings,
                                                                            {"referenceAudioPath",
                                                                             "reference_audio_path",
                                                                             "refAudioPath",
                                                                             "ref_audio_path",
                                                                             "voiceTrainingReferenceAudioPath"}));
    if (referenceAudioPath.isEmpty()) {
        referenceAudioPath = joinedPath(firstStringFromVariant(settings, {"voiceDirectory", "referenceAudioDirectory"}),
                                        firstStringFromVariant(settings, {"defaultVoice", "referenceAudioFile"}));
    }
    if (referenceAudioPath.isEmpty()) {
        const QString artifactPath = localPathFromUrlText(firstStringFromVariant(settings,
                                                                                 {"voiceTrainingArtifactPath",
                                                                                  "artifactPath"}));
        if (looksLikeAudioPath(artifactPath)) {
            referenceAudioPath = artifactPath;
        }
    }
    QString referenceSource = firstStringFromVariant(settings, {"referenceAudioSource", "reference_audio_source"});
    if (_voice_reference_audio_source == QStringLiteral("voice_training")
        && referenceSource != QStringLiteral("voice_training")
        && !_voice_reference_audio_path.trimmed().isEmpty()) {
        referenceAudioPath = _voice_reference_audio_path.trimmed();
        referenceSource = _voice_reference_audio_source;
    }

    QString provider = firstStringFromVariant(settings, {"voiceProvider", "provider", "voice_provider"});
    if (provider.isEmpty() && !referenceAudioPath.isEmpty()) {
        provider = QStringLiteral("gpt-sovits");
    }

    QString voiceName = firstStringFromVariant(settings, {"voiceName", "voice_name", "characterName"});
    if (voiceName.isEmpty()) {
        const QString defaultVoice = firstStringFromVariant(settings, {"defaultVoice", "referenceAudioFile"});
        if (!defaultVoice.isEmpty()) {
            voiceName = QFileInfo(defaultVoice).completeBaseName();
        }
    }
    if (voiceName.isEmpty() && !provider.isEmpty()) {
        voiceName = provider;
    }

    QString language = firstStringFromVariant(settings, {"voiceLanguage", "voice_language", "language"});
    if (language.isEmpty()) {
        language = _reply_language.trimmed().isEmpty() ? QStringLiteral("zh-CN") : _reply_language.trimmed();
    }
    const QString textLanguage = firstStringFromVariant(settings, {"textLanguage", "text_lang"});
    const QString promptLanguage = firstStringFromVariant(settings, {"promptLanguage", "prompt_lang"});
    const QString promptText = firstStringFromVariant(settings, {"promptText", "prompt_text"});

    const bool changed = _voice_provider != provider
                         || _voice_name != voiceName
                         || _voice_language != language
                         || _voice_reference_audio_path != referenceAudioPath
                         || _voice_reference_audio_source != referenceSource
                         || _voice_prompt_text != promptText
                         || _voice_prompt_language != promptLanguage
                         || _voice_text_language != textLanguage;
    if (!changed) {
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

void PetController::refreshVoiceTrainingJob(const QString &jobId)
{
    const QString selectedJobId = jobId.trimmed().isEmpty() ? _voice_training_job_id : jobId.trimmed();
    if (selectedJobId.isEmpty() || _voice_training_busy) {
        return;
    }
    setError({});
    setVoiceTrainingBusy(true, QStringLiteral("正在刷新声音训练任务"));
    getJson(petUrl(QStringLiteral("/voice-training/jobs/%1").arg(QString::fromLatin1(QUrl::toPercentEncoding(selectedJobId)))),
            QStringLiteral("voice_training_get"));
}

void PetController::setBusy(bool busy, const QString &statusText)
{
    const bool changed = _busy != busy || (!statusText.isNull() && _status_text != statusText);
    _busy = busy;
    if (!statusText.isNull()) {
        _status_text = statusText;
    }
    if (changed) {
        emit stateChanged();
    }
}

void PetController::setVoiceTrainingBusy(bool busy, const QString &message)
{
    const bool changed = _voice_training_busy != busy || (!message.isNull() && _voice_training_message != message);
    _voice_training_busy = busy;
    if (!message.isNull()) {
        _voice_training_message = message;
    }
    if (changed) {
        emit voiceTrainingChanged();
    }
}

void PetController::setVisionRequestInFlight(bool inFlight)
{
    if (_vision_request_in_flight == inFlight) {
        return;
    }
    _vision_request_in_flight = inFlight;
    emit stateChanged();
}

void PetController::applyVoiceTrainingJob(const QJsonObject &job)
{
    if (job.isEmpty()) {
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
    const QString trainedReferenceAudio = diagnostics.value(QStringLiteral("gpt_sovits_reference_audio")).toString().trimmed().isEmpty()
                                              ? diagnostics.value(QStringLiteral("reference_audio_runtime_path")).toString().trimmed()
                                              : diagnostics.value(QStringLiteral("gpt_sovits_reference_audio")).toString().trimmed();
    if (!trainedReferenceAudio.isEmpty()) {
        _voice_reference_audio_path = trainedReferenceAudio;
        _voice_reference_audio_source = QStringLiteral("voice_training");
        _voice_provider = QStringLiteral("gpt-sovits");
        const QString trainedVoiceName = job.value(QStringLiteral("voice_name")).toString().trimmed();
        if (!trainedVoiceName.isEmpty()) {
            _voice_name = trainedVoiceName;
        }
        const QString trainedLanguage = job.value(QStringLiteral("language")).toString().trimmed();
        if (!trainedLanguage.isEmpty()) {
            _voice_language = trainedLanguage;
            if (_voice_text_language.isEmpty()) {
                _voice_text_language = voiceTextLanguage(trainedLanguage);
            }
        }
    }
    if (_voice_training_stage == QStringLiteral("ready_for_worker")) {
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
    setStatusText(_voice_training_message);
    emit voiceTrainingChanged();
}

void PetController::setWindowsImeBridgeBusy(bool busy)
{
    if (_windows_ime_bridge_busy == busy) {
        return;
    }
    _windows_ime_bridge_busy = busy;
    emit windowsImeBridgeChanged();
}

void PetController::setWindowsCameraBridgeBusy(bool busy)
{
    if (_windows_camera_bridge_busy == busy) {
        return;
    }
    _windows_camera_bridge_busy = busy;
    emit windowsCameraBridgeChanged();
}

void PetController::setStatusText(const QString &statusText)
{
    if (_status_text == statusText) {
        return;
    }
    _status_text = statusText;
    emit stateChanged();
}

void PetController::setError(const QString &error)
{
    if (_error == error) {
        return;
    }
    _error = error;
    emit stateChanged();
}

QJsonObject PetController::authPayload() const
{
    QJsonObject payload;
    if (_gateway && _gateway->userMgr()) {
        payload[QStringLiteral("uid")] = _gateway->userMgr()->GetUid();
    }
    return payload;
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
    if (provider.isEmpty() && referenceAudioPath.isEmpty()) {
        return metadata;
    }
    if (!provider.isEmpty()) {
        metadata[QStringLiteral("voice_provider")] = provider;
    }
    if (!_voice_name.trimmed().isEmpty()) {
        metadata[QStringLiteral("voice_name")] = _voice_name.trimmed();
    }
    const QString voiceLanguage = _voice_language.trimmed().isEmpty()
                                      ? (_reply_language.trimmed().isEmpty() ? QStringLiteral("zh-CN") : _reply_language.trimmed())
                                      : _voice_language.trimmed();
    if (!voiceLanguage.isEmpty()) {
        metadata[QStringLiteral("voice_language")] = voiceLanguage;
    }
    if (!referenceAudioPath.isEmpty()) {
        metadata[QStringLiteral("ref_audio_path")] = referenceAudioPath;
    }
    if (!_voice_reference_audio_source.trimmed().isEmpty()) {
        metadata[QStringLiteral("reference_audio_source")] = _voice_reference_audio_source.trimmed();
    }
    if (!_voice_prompt_text.trimmed().isEmpty()) {
        metadata[QStringLiteral("prompt_text")] = _voice_prompt_text.trimmed();
    }
    if (!_voice_prompt_language.trimmed().isEmpty()) {
        metadata[QStringLiteral("prompt_lang")] = _voice_prompt_language.trimmed();
    }
    metadata[QStringLiteral("text_lang")] = _voice_text_language.trimmed().isEmpty()
                                                ? voiceTextLanguage(voiceLanguage)
                                                : _voice_text_language.trimmed();
    return metadata;
}

void PetController::appendVoiceRuntimeMetadata(QJsonObject &metadata) const
{
    const QJsonObject voiceMetadata = voiceRuntimeMetadata();
    for (auto it = voiceMetadata.constBegin(); it != voiceMetadata.constEnd(); ++it) {
        metadata.insert(it.key(), it.value());
    }
}

QUrl PetController::petUrl(const QString &path) const
{
    return QUrl(gate_url_prefix.trimmed() + QStringLiteral("/ai/pet") + path);
}

QString PetController::encodedSessionId() const
{
    return QString::fromLatin1(QUrl::toPercentEncoding(_session_id));
}
