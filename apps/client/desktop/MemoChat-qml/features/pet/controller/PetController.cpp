#include "PetController.h"

#include "global.h"

#include <QProcess>
#include <QUrl>
#include <QUrlQuery>

PetController::PetController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
{
    connect(&_model, &PetModel::changed, this, &PetController::petStateChanged);
}

PetController::~PetController()
{
    stopStream();
    if (_windows_camera_bridge_process)
    {
        auto* process = _windows_camera_bridge_process.data();
        _windows_camera_bridge_process.clear();
        process->disconnect(this);
        if (process->state() != QProcess::NotRunning)
        {
            process->kill();
            process->waitForFinished(250);
        }
        process->deleteLater();
    }
}

QString PetController::audioUrl() const
{
    const QString raw = _model.audioUrl().trimmed();
    if (raw.isEmpty())
    {
        return {};
    }
    const QUrl parsed(raw);
    if (parsed.isValid() && !parsed.isRelative())
    {
        return raw;
    }
    const QString path = raw.startsWith(QLatin1Char('/')) ? raw : QStringLiteral("/%1").arg(raw);
    if (path.startsWith(QStringLiteral("/audio/")))
    {
        const QString mediaBase =
            gate_media_url_prefix.trimmed().isEmpty() ? gate_url_prefix.trimmed() : gate_media_url_prefix.trimmed();
        QUrl base(mediaBase);
        if (base.isValid() && !base.host().isEmpty())
        {
            const int port = base.port(8096);
            if (port == 8096)
            {
                base.setPath(QStringLiteral("/pet") + path);
            }
            else if (port == 80 || port == 8080 || port == 8443)
            {
                base.setScheme(QStringLiteral("http"));
                base.setPort(8096);
                base.setPath(QStringLiteral("/pet") + path);
            }
            else
            {
                base.setPath(QStringLiteral("/ai/pet") + path);
            }
            QUrlQuery query(base);
            if (!_model.turnId().isEmpty())
            {
                query.addQueryItem(QStringLiteral("turn"), _model.turnId());
            }
            if (!_model.eventId().isEmpty())
            {
                query.addQueryItem(QStringLiteral("event"), _model.eventId());
            }
            base.setQuery(query);
            return base.toString();
        }
    }
    QUrl url = petUrl(path);
    QUrlQuery query(url);
    if (!_model.turnId().isEmpty())
    {
        query.addQueryItem(QStringLiteral("turn"), _model.turnId());
    }
    if (!_model.eventId().isEmpty())
    {
        query.addQueryItem(QStringLiteral("event"), _model.eventId());
    }
    url.setQuery(query);
    return url.toString();
}

void PetController::stopStream()
{
    if (_stream_reply)
    {
        auto* reply = _stream_reply.data();
        _stream_reply.clear();
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    _stream_buffer.clear();
    if (_streaming)
    {
        _streaming = false;
        emit stateChanged();
    }
}

void PetController::clearSpeech()
{
    _model.clearSpeech();
}

void PetController::resetForLogout()
{
    ++_request_generation;
    stopStream();
    resetControlEventDedupe();

    const bool selectionChanged = !_selected_model_type.isEmpty() || !_selected_model_name.isEmpty();
    const bool languageChanged = _reply_language != QStringLiteral("zh-CN");
    const bool rulesChanged = !_speech_rules.isEmpty();

    _session_id.clear();
    _busy = false;
    _input_request_in_flight = false;
    _voice_training_busy = false;
    _voice_training_job_id.clear();
    _voice_training_status = QStringLiteral("idle");
    _voice_training_stage.clear();
    _voice_training_progress = 0;
    _voice_training_artifact_path.clear();
    _voice_training_message.clear();
    _error.clear();
    _status_text.clear();
    _selected_model_type.clear();
    _selected_model_name.clear();
    _reply_language = QStringLiteral("zh-CN");
    _speech_rules.clear();
    _voice_provider.clear();
    _voice_name.clear();
    _voice_language.clear();
    _voice_reference_audio_path.clear();
    _voice_reference_audio_source.clear();
    _voice_prompt_text.clear();
    _voice_prompt_language.clear();
    _voice_text_language.clear();
    _vision_segment_frames = QJsonArray();
    _vision_segment_started_at_ms = 0;
    _vision_segment_last_posted_at_ms = 0;
    _last_vision_frame_signature.clear();
    _last_vision_frame_accepted_at_ms = 0;
    _vision_request_in_flight = false;
    _model.reset();

    emit stateChanged();
    emit voiceTrainingChanged();
    if (selectionChanged)
    {
        emit modelSelectionChanged();
    }
    if (languageChanged)
    {
        emit replyLanguageChanged();
    }
    if (rulesChanged)
    {
        emit speechRulesChanged();
    }
}
