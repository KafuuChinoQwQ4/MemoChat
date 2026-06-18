#include "PetController.h"
#include "PetControllerPrivate.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

using namespace memochat::pet;

void PetController::postJson(const QUrl& url, const QJsonObject& payload, const QString& op)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    configurePetRequest(request);
    auto* reply = _network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("pet_op", op);
    reply->setProperty("pet_generation", _request_generation);
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply]()
            {
                handleJsonReply(reply);
            });
}

void PetController::getJson(const QUrl& url, const QString& op)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    configurePetRequest(request);
    auto* reply = _network.get(request);
    reply->setProperty("pet_op", op);
    reply->setProperty("pet_generation", _request_generation);
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply]()
            {
                handleJsonReply(reply);
            });
}

void PetController::startStream()
{
    if (_session_id.isEmpty() || _streaming)
    {
        return;
    }

    QNetworkRequest request(petUrl(QStringLiteral("/sessions/%1/stream").arg(encodedSessionId())));
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Cache-Control", "no-cache");
    configurePetRequest(request);
    _stream_reply = _stream_network.get(request);
    _streaming = true;
    _stream_buffer.clear();
    setStatusText(QStringLiteral("桌宠事件流已连接"));
    emit stateChanged();

    connect(_stream_reply.data(),
            &QNetworkReply::readyRead,
            this,
            [this]()
            {
                if (_stream_reply)
                {
                    consumeStreamBytes(_stream_reply->readAll());
                }
            });
    connect(_stream_reply.data(),
            &QNetworkReply::finished,
            this,
            [this]()
            {
                if (!_stream_reply)
                {
                    return;
                }
                const auto err = _stream_reply->error();
                const QString errText = _stream_reply->errorString();
                _stream_reply->deleteLater();
                _stream_reply.clear();
                _streaming = false;
                if (err != QNetworkReply::NoError && err != QNetworkReply::OperationCanceledError)
                {
                    setError(QStringLiteral("桌宠事件流断开: %1").arg(errText));
                }
                else
                {
                    setStatusText(QStringLiteral("桌宠事件流已关闭"));
                }
                emit stateChanged();
            });
}

void PetController::handleJsonReply(QNetworkReply* reply)
{
    const QString op = reply->property("pet_op").toString();
    const int replyGeneration = reply->property("pet_generation").toInt();
    if (replyGeneration != _request_generation)
    {
        reply->deleteLater();
        return;
    }
    const bool visionOp = op == QStringLiteral("vision_capture") || op == QStringLiteral("vision_segment_capture");
    const auto networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray bytes = reply->readAll();
    reply->deleteLater();

    if (op == QStringLiteral("create_session"))
    {
        setBusy(false);
    }
    else if (op == QStringLiteral("input"))
    {
        _input_request_in_flight = false;
        setBusy(false);
    }
    else if (op == QStringLiteral("voice_training_create") || op == QStringLiteral("voice_training_get"))
    {
        setVoiceTrainingBusy(false);
    }

    if (networkError != QNetworkReply::NoError)
    {
        if (visionOp)
        {
            setVisionRequestInFlight(false);
        }
        setError(QStringLiteral("桌宠请求失败: %1").arg(networkErrorText));
        return;
    }
    if (httpStatus >= 400)
    {
        if (visionOp)
        {
            setVisionRequestInFlight(false);
        }
        setError(QStringLiteral("桌宠 HTTP %1").arg(httpStatus));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject())
    {
        if (visionOp)
        {
            setVisionRequestInFlight(false);
        }
        setError(QStringLiteral("桌宠响应格式错误"));
        return;
    }
    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("code")).toInt(0) != 0)
    {
        if (visionOp)
        {
            setVisionRequestInFlight(false);
        }
        setError(root.value(QStringLiteral("message")).toString(QStringLiteral("桌宠请求失败")));
        return;
    }

    if (op == QStringLiteral("create_session"))
    {
        const QJsonObject session = root.value(QStringLiteral("session")).toObject();
        const QString nextSessionId = session.value(QStringLiteral("session_id")).toString();
        if (nextSessionId.isEmpty())
        {
            setError(QStringLiteral("桌宠会话缺少 session_id"));
            return;
        }
        if (_session_id != nextSessionId)
        {
            resetControlEventDedupe();
        }
        _session_id = nextSessionId;
        setStatusText(QStringLiteral("桌宠会话已创建"));
        emit stateChanged();
        startStream();
        return;
    }

    if (op == QStringLiteral("voice_training_create") || op == QStringLiteral("voice_training_get"))
    {
        applyVoiceTrainingJob(root.value(QStringLiteral("job")).toObject());
        return;
    }

    if (visionOp)
    {
        setVisionRequestInFlight(false);
    }

    const QJsonArray events = root.value(QStringLiteral("events")).toArray();
    for (const QJsonValue& value : events)
    {
        applyControlEvent(value.toObject());
    }
    const QJsonObject event = root.value(QStringLiteral("event")).toObject();
    if (!event.isEmpty())
    {
        applyControlEvent(event);
    }

    if (op == QStringLiteral("vision_segment_capture"))
    {
        setStatusText(QStringLiteral("视觉片段已分析"));
    }
    else if (op == QStringLiteral("vision_capture"))
    {
        setStatusText(QStringLiteral("摄像头帧已分析"));
    }
}

void PetController::applyControlEvent(const QJsonObject& event)
{
    if (event.value(QStringLiteral("type")).toString() != QStringLiteral("pet.control"))
    {
        return;
    }
    if (!rememberControlEvent(event))
    {
        return;
    }
    _model.applyControlEvent(event);
    emit controlEventReceived(event.toVariantMap());
}

bool PetController::rememberControlEvent(const QJsonObject& event)
{
    const QString sessionId = event.value(QStringLiteral("session_id")).toString(_session_id).trimmed();
    const QString eventId = event.value(QStringLiteral("event_id")).toString().trimmed();
    const QString turnId = event.value(QStringLiteral("turn_id")).toString().trimmed();
    const QString phase = event.value(QStringLiteral("phase")).toString().trimmed();
    const QString actionName =
        event.value(QStringLiteral("action")).toObject().value(QStringLiteral("name")).toString().trimmed();
    const QString textDelta = event.value(QStringLiteral("text")).toObject().value(QStringLiteral("delta")).toString();
    const QString audioUrl = event.value(QStringLiteral("audio")).toObject().value(QStringLiteral("url")).toString();
    QString key;
    if (!eventId.isEmpty())
    {
        key = sessionId + QStringLiteral(":event:") + eventId;
    }
    else if (!turnId.isEmpty())
    {
        key = sessionId + QStringLiteral(":turn:") + turnId +
                                         QStringLiteral(":") + phase +
                                                        QStringLiteral(":") + actionName +
                                                                       QStringLiteral(":") + textDelta +
                                                                                      QStringLiteral(":") + audioUrl;
    }
    else
    {
        const int sequence = event.value(QStringLiteral("seq")).toInt(-1);
        if (sequence < 0)
        {
            return true;
        }
        key = sessionId + QStringLiteral(":seq:") + QString::number(sequence);
    }

    if (_applied_control_event_keys.contains(key))
    {
        return false;
    }
    _applied_control_event_keys.insert(key);
    _applied_control_event_order.append(key);
    while (_applied_control_event_order.size() > kMaxRememberedPetControlEvents)
    {
        _applied_control_event_keys.remove(_applied_control_event_order.takeFirst());
    }
    return true;
}

void PetController::resetControlEventDedupe()
{
    _applied_control_event_keys.clear();
    _applied_control_event_order.clear();
}

void PetController::consumeStreamBytes(const QByteArray& bytes)
{
    _stream_buffer += bytes;
    for (;;)
    {
        const int newline = _stream_buffer.indexOf('\n');
        if (newline < 0)
        {
            return;
        }
        QByteArray line = _stream_buffer.left(newline);
        _stream_buffer.remove(0, newline + 1);
        if (line.endsWith('\r'))
        {
            line.chop(1);
        }
        consumeSseLine(line);
    }
}

void PetController::consumeSseLine(const QByteArray& line)
{
    if (!line.startsWith("data: "))
    {
        return;
    }
    const QByteArray payload = line.mid(6);
    if (payload == "[DONE]")
    {
        stopStream();
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
    {
        return;
    }
    applyControlEvent(doc.object());
}
