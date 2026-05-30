#include "PetController.h"

#include <QJsonObject>

void PetController::startSession()
{
    if (_busy)
    {
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

void PetController::sendText(const QString& text)
{
    const QString content = text.trimmed();
    if (content.isEmpty())
    {
        return;
    }
    if (_input_request_in_flight)
    {
        setStatusText(QStringLiteral("上一条消息仍在发送"));
        return;
    }
    if (_session_id.isEmpty())
    {
        setError(QStringLiteral("桌宠会话未连接"));
        return;
    }
    if (!_streaming)
    {
        startStream();
    }

    _model.clearSpeech();
    _input_request_in_flight = true;
    setBusy(true, QStringLiteral("正在发送"));
    QJsonObject payload = authPayload();
    payload[QStringLiteral("content")] = content;
    if (!_selected_model_type.trimmed().isEmpty())
    {
        payload[QStringLiteral("model_type")] = _selected_model_type.trimmed();
    }
    if (!_selected_model_name.trimmed().isEmpty())
    {
        payload[QStringLiteral("model_name")] = _selected_model_name.trimmed();
    }
    QJsonObject metadata;
    metadata[QStringLiteral("reply_language")] = _reply_language.trimmed().isEmpty() ? QStringLiteral("zh-CN")
                                                                                     : _reply_language.trimmed();
    const QString speechRules = _speech_rules.trimmed();
    if (!speechRules.isEmpty())
    {
        metadata[QStringLiteral("speech_rules")] = speechRules;
    }
    appendVoiceRuntimeMetadata(metadata);
    payload[QStringLiteral("metadata")] = metadata;
    postJson(petUrl(QStringLiteral("/sessions/%1/input").arg(encodedSessionId())), payload, QStringLiteral("input"));
}

void PetController::sendObservation(const QVariantMap& observation)
{
    if (_session_id.isEmpty())
    {
        return;
    }
    QJsonObject payload = QJsonObject::fromVariantMap(observation);
    if (payload.isEmpty())
    {
        payload = defaultObservationPayload();
    }
    QJsonObject vision = payload.value(QStringLiteral("vision")).toObject();
    appendVoiceRuntimeMetadata(vision);
    if (!vision.isEmpty())
    {
        payload[QStringLiteral("vision")] = vision;
    }
    postJson(petUrl(QStringLiteral("/sessions/%1/observation").arg(encodedSessionId())),
                    payload,
                    QStringLiteral("observation"));
}

void PetController::interrupt()
{
    if (_session_id.isEmpty())
    {
        return;
    }
    postJson(petUrl(QStringLiteral("/sessions/%1/interrupt").arg(encodedSessionId())),
                    QJsonObject{},
                    QStringLiteral("interrupt"));
}
