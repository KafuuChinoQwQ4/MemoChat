#include "PetController.h"

#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QJsonObject>
#include <QUrl>

void PetController::setBusy(bool busy, const QString& statusText)
{
    const bool changed = _busy != busy || (!statusText.isNull() && _status_text != statusText);
    _busy = busy;
    if (!statusText.isNull())
    {
        _status_text = statusText;
    }
    if (changed)
    {
        emit stateChanged();
    }
}

void PetController::setVoiceTrainingBusy(bool busy, const QString& message)
{
    const bool changed = _voice_training_busy != busy || (!message.isNull() && _voice_training_message != message);
    _voice_training_busy = busy;
    if (!message.isNull())
    {
        _voice_training_message = message;
    }
    if (changed)
    {
        emit voiceTrainingChanged();
    }
}

void PetController::setVisionRequestInFlight(bool inFlight)
{
    if (_vision_request_in_flight == inFlight)
    {
        return;
    }
    _vision_request_in_flight = inFlight;
    emit stateChanged();
}

void PetController::setWindowsImeBridgeBusy(bool busy)
{
    if (_windows_ime_bridge_busy == busy)
    {
        return;
    }
    _windows_ime_bridge_busy = busy;
    emit windowsImeBridgeChanged();
}

void PetController::setWindowsCameraBridgeBusy(bool busy)
{
    if (_windows_camera_bridge_busy == busy)
    {
        return;
    }
    _windows_camera_bridge_busy = busy;
    emit windowsCameraBridgeChanged();
}

void PetController::setStatusText(const QString& statusText)
{
    if (_status_text == statusText)
    {
        return;
    }
    _status_text = statusText;
    emit stateChanged();
}

void PetController::setError(const QString& error)
{
    if (_error == error)
    {
        return;
    }
    _error = error;
    emit stateChanged();
}

QJsonObject PetController::authPayload() const
{
    QJsonObject payload;
    if (_gateway && _gateway->userMgr())
    {
        payload[QStringLiteral("uid")] = _gateway->userMgr()->GetUid();
    }
    return payload;
}

QUrl PetController::petUrl(const QString& path) const
{
    return QUrl(gate_url_prefix.trimmed() + QStringLiteral("/ai/pet") + path);
}

QString PetController::encodedSessionId() const
{
    return QString::fromLatin1(QUrl::toPercentEncoding(_session_id));
}
