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
