#include "CallSessionModel.h"

#include <QDateTime>

CallSessionModel::CallSessionModel(QObject *parent)
    : QObject(parent)
{
    _tick_timer.setInterval(1000);
    connect(&_tick_timer, &QTimer::timeout, this, &CallSessionModel::onTick);
}

bool CallSessionModel::visible() const { return _visible; }
bool CallSessionModel::incoming() const { return _incoming; }
bool CallSessionModel::active() const { return _active; }
bool CallSessionModel::tokenReady() const { return _token_ready; }
QString CallSessionModel::callId() const { return _call_id; }
QString CallSessionModel::callType() const { return _call_type; }
QString CallSessionModel::peerName() const { return _peer_name; }
QString CallSessionModel::peerIcon() const { return _peer_icon; }
QString CallSessionModel::stateText() const { return _state_text; }
QString CallSessionModel::roomName() const { return _room_name; }
QString CallSessionModel::livekitUrl() const { return _livekit_url; }
QString CallSessionModel::mediaLaunchJson() const { return _media_launch_json; }
QString CallSessionModel::mediaStatusText() const { return _media_status_text; }
int CallSessionModel::elapsedSeconds() const { return _elapsed_seconds; }
bool CallSessionModel::muted() const { return _muted; }
bool CallSessionModel::cameraEnabled() const { return _camera_enabled; }

void CallSessionModel::startOutgoing(const QString &callId, const QString &callType, const QString &peerName,
                                     const QString &peerIcon, const QString &stateText,
                                     qint64 startedAtMs, qint64 expiresAtMs)
{
    _visible = true;
    _incoming = false;
    _active = false;
    _token_ready = false;
    _call_id = callId;
    _call_type = callType;
    _peer_name = peerName;
    _peer_icon = peerIcon;
    _state_text = stateText;
    _room_name.clear();
    _livekit_url.clear();
    _media_launch_json.clear();
    _media_status_text = QStringLiteral("等待对方接听");
    _started_at_ms = startedAtMs;
    _accepted_at_ms = 0;
    _expires_at_ms = expiresAtMs;
    _elapsed_seconds = 0;
    _muted = false;
    _camera_enabled = callType == QStringLiteral("video");
    _tick_timer.start();
    emit changed();
}

void CallSessionModel::startIncoming(const QString &callId, const QString &callType, const QString &peerName,
                                     const QString &peerIcon, const QString &stateText,
                                     qint64 startedAtMs, qint64 expiresAtMs)
{
    _visible = true;
    _incoming = true;
    _active = false;
    _token_ready = false;
    _call_id = callId;
    _call_type = callType;
    _peer_name = peerName;
    _peer_icon = peerIcon;
    _state_text = stateText;
    _room_name.clear();
    _livekit_url.clear();
    _media_launch_json.clear();
    _media_status_text = QStringLiteral("等待接听");
    _started_at_ms = startedAtMs;
    _accepted_at_ms = 0;
    _expires_at_ms = expiresAtMs;
    _elapsed_seconds = 0;
    _muted = false;
    _camera_enabled = callType == QStringLiteral("video");
    _tick_timer.start();
    emit changed();
}

void CallSessionModel::markAccepted(const QString &stateText, const QString &roomName,
                                    const QString &livekitUrl, qint64 acceptedAtMs)
{
    _incoming = false;
    _active = true;
    _state_text = stateText;
    _room_name = roomName;
    _livekit_url = livekitUrl;
    _accepted_at_ms = acceptedAtMs > 0 ? acceptedAtMs : QDateTime::currentMSecsSinceEpoch();
    _media_status_text = QStringLiteral("正在准备媒体通道");
    emit changed();
}

void CallSessionModel::markTokenReady(const QString &mediaStatusText)
{
    _token_ready = true;
    _media_status_text = mediaStatusText;
    emit changed();
}

void CallSessionModel::setMediaLaunchJson(const QString &mediaLaunchJson)
{
    if (_media_launch_json == mediaLaunchJson) {
        return;
    }
    _media_launch_json = mediaLaunchJson;
    emit changed();
}

void CallSessionModel::setMediaStatusText(const QString &mediaStatusText)
{
    if (_media_status_text == mediaStatusText) {
        return;
    }
    _media_status_text = mediaStatusText;
    emit changed();
}

void CallSessionModel::markEnded(const QString &stateText)
{
    _state_text = stateText;
    _active = false;
    _token_ready = false;
    _tick_timer.stop();
    emit changed();
}

void CallSessionModel::clear()
{
    _tick_timer.stop();
    _visible = false;
    _incoming = false;
    _active = false;
    _token_ready = false;
    _call_id.clear();
    _call_type.clear();
    _peer_name.clear();
    _peer_icon.clear();
    _state_text.clear();
    _room_name.clear();
    _livekit_url.clear();
    _media_launch_json.clear();
    _media_status_text.clear();
    _started_at_ms = 0;
    _accepted_at_ms = 0;
    _expires_at_ms = 0;
    _elapsed_seconds = 0;
    _muted = false;
    _camera_enabled = true;
    emit changed();
}

void CallSessionModel::setMuted(bool muted)
{
    if (_muted == muted) {
        return;
    }
    _muted = muted;
    emit changed();
}

void CallSessionModel::setCameraEnabled(bool enabled)
{
    if (_camera_enabled == enabled) {
        return;
    }
    _camera_enabled = enabled;
    emit changed();
}

void CallSessionModel::onTick()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 baseMs = _accepted_at_ms > 0 ? _accepted_at_ms : _started_at_ms;
    if (baseMs > 0) {
        const int seconds = static_cast<int>(qMax<qint64>(0, nowMs - baseMs) / 1000);
        if (seconds != _elapsed_seconds) {
            _elapsed_seconds = seconds;
            emit changed();
        }
    }
    if (_expires_at_ms > 0 && nowMs >= _expires_at_ms && !_active) {
        _state_text = _incoming ? QStringLiteral("来电已超时") : QStringLiteral("呼叫超时");
        _media_status_text = QStringLiteral("等待清理会话");
        emit changed();
    }
}
