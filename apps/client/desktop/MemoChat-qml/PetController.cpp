#include "PetController.h"

#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

PetController::PetController(ClientGateway *gateway, QObject *parent)
    : QObject(parent)
    , _gateway(gateway)
{
    connect(&_model, &PetModel::changed, this, &PetController::petStateChanged);
}

PetController::~PetController()
{
    stopStream();
}

void PetController::startSession()
{
    if (_busy) {
        return;
    }
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
    if (_session_id.isEmpty()) {
        setError(QStringLiteral("桌宠会话未连接"));
        return;
    }

    QJsonObject payload = authPayload();
    payload[QStringLiteral("content")] = content;
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

void PetController::postJson(const QUrl &url, const QJsonObject &payload, const QString &op)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto *reply = _network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("pet_op", op);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleJsonReply(reply);
    });
}

void PetController::startStream()
{
    if (_session_id.isEmpty() || _streaming) {
        return;
    }

    QNetworkRequest request(petUrl(QStringLiteral("/sessions/%1/stream").arg(encodedSessionId())));
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Cache-Control", "no-cache");
    _stream_reply = _stream_network.get(request);
    _streaming = true;
    _stream_buffer.clear();
    setStatusText(QStringLiteral("桌宠事件流已连接"));
    emit stateChanged();

    connect(_stream_reply.data(), &QNetworkReply::readyRead, this, [this]() {
        if (_stream_reply) {
            consumeStreamBytes(_stream_reply->readAll());
        }
    });
    connect(_stream_reply.data(), &QNetworkReply::finished, this, [this]() {
        if (!_stream_reply) {
            return;
        }
        const auto err = _stream_reply->error();
        const QString errText = _stream_reply->errorString();
        _stream_reply->deleteLater();
        _stream_reply.clear();
        _streaming = false;
        if (err != QNetworkReply::NoError && err != QNetworkReply::OperationCanceledError) {
            setError(QStringLiteral("桌宠事件流断开: %1").arg(errText));
        } else {
            setStatusText(QStringLiteral("桌宠事件流已关闭"));
        }
        emit stateChanged();
    });
}

void PetController::handleJsonReply(QNetworkReply *reply)
{
    const QString op = reply->property("pet_op").toString();
    const auto networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray bytes = reply->readAll();
    reply->deleteLater();

    if (op == QStringLiteral("create_session")) {
        setBusy(false);
    }

    if (networkError != QNetworkReply::NoError) {
        setError(QStringLiteral("桌宠请求失败: %1").arg(networkErrorText));
        return;
    }
    if (httpStatus >= 400) {
        setError(QStringLiteral("桌宠 HTTP %1").arg(httpStatus));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) {
        setError(QStringLiteral("桌宠响应格式错误"));
        return;
    }
    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("code")).toInt(0) != 0) {
        setError(root.value(QStringLiteral("message")).toString(QStringLiteral("桌宠请求失败")));
        return;
    }

    if (op == QStringLiteral("create_session")) {
        const QJsonObject session = root.value(QStringLiteral("session")).toObject();
        const QString nextSessionId = session.value(QStringLiteral("session_id")).toString();
        if (nextSessionId.isEmpty()) {
            setError(QStringLiteral("桌宠会话缺少 session_id"));
            return;
        }
        _session_id = nextSessionId;
        setStatusText(QStringLiteral("桌宠会话已创建"));
        emit stateChanged();
        startStream();
        return;
    }

    if (!_streaming) {
        const QJsonArray events = root.value(QStringLiteral("events")).toArray();
        for (const QJsonValue &value : events) {
            applyControlEvent(value.toObject());
        }
        const QJsonObject event = root.value(QStringLiteral("event")).toObject();
        if (!event.isEmpty()) {
            applyControlEvent(event);
        }
    }
}

void PetController::applyControlEvent(const QJsonObject &event)
{
    if (event.value(QStringLiteral("type")).toString() != QStringLiteral("pet.control")) {
        return;
    }
    _model.applyControlEvent(event);
    emit controlEventReceived(event.toVariantMap());
}

void PetController::consumeStreamBytes(const QByteArray &bytes)
{
    _stream_buffer += bytes;
    for (;;) {
        const int newline = _stream_buffer.indexOf('\n');
        if (newline < 0) {
            return;
        }
        QByteArray line = _stream_buffer.left(newline);
        _stream_buffer.remove(0, newline + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        consumeSseLine(line);
    }
}

void PetController::consumeSseLine(const QByteArray &line)
{
    if (!line.startsWith("data: ")) {
        return;
    }
    const QByteArray payload = line.mid(6);
    if (payload == "[DONE]") {
        stopStream();
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        return;
    }
    applyControlEvent(doc.object());
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

QUrl PetController::petUrl(const QString &path) const
{
    return QUrl(gate_url_prefix + QStringLiteral("/ai/pet") + path);
}

QString PetController::encodedSessionId() const
{
    return QString::fromLatin1(QUrl::toPercentEncoding(_session_id));
}
