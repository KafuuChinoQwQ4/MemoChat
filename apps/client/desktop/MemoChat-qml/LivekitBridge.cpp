#include "LivekitBridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace {
QString defaultRoomPageUrl()
{
    return QStringLiteral("qrc:/web/livekit/index.html");
}
}

LivekitBridge::LivekitBridge(QObject *parent)
    : QObject(parent)
{
    setObjectName(QStringLiteral("livekitBridge"));
    loadConfig();
}

QString LivekitBridge::roomPageUrl() const
{
    return _room_page_url;
}

bool LivekitBridge::embeddedEnabled() const
{
    return _embedded_enabled;
}

void LivekitBridge::requestJoinRoom(const QString &wsUrl, const QString &token, const QString &metadataJson)
{
    _pending_ws_url = wsUrl;
    _pending_token = token;
    _pending_metadata_json = metadataJson;
    flushPendingJoin();
}

void LivekitBridge::setPageReady(bool ready)
{
    if (_page_ready == ready) {
        return;
    }
    _page_ready = ready;
    emit stateChanged();
    if (_page_ready) {
        flushPendingJoin();
    }
}

void LivekitBridge::toggleMic()
{
    emit micToggleRequested();
}

void LivekitBridge::toggleCamera()
{
    emit cameraToggleRequested();
}

void LivekitBridge::leaveRoom()
{
    emit leaveRequested();
}

void LivekitBridge::reportRoomJoined()
{
    emit roomJoined();
}

void LivekitBridge::reportRemoteTrackReady()
{
    emit remoteTrackReady();
}

void LivekitBridge::reportRoomDisconnected(const QString &reason, bool recoverable)
{
    emit roomDisconnected(reason, recoverable);
}

void LivekitBridge::reportPermissionError(const QString &deviceType, const QString &message)
{
    emit permissionError(deviceType, message);
}

void LivekitBridge::reportMediaError(const QString &message)
{
    emit mediaError(message);
}

void LivekitBridge::reportReconnecting(const QString &message)
{
    emit reconnecting(message);
}

void LivekitBridge::reportLog(const QString &message)
{
    emit bridgeLog(message);
}

void LivekitBridge::loadConfig()
{
    const QString configPath = QDir::toNativeSeparators(
        QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("config.ini"));
    QSettings settings(configPath, QSettings::IniFormat);
    _embedded_enabled = settings.value(QStringLiteral("Call/UseEmbeddedWebView"), true).toBool();
    _room_page_url = settings.value(QStringLiteral("Call/LiveKitUiUrl"), defaultRoomPageUrl()).toString().trimmed();
    if (_room_page_url.isEmpty()) {
        _room_page_url = defaultRoomPageUrl();
    }
    emit stateChanged();
}

void LivekitBridge::flushPendingJoin()
{
    if (!_page_ready || !_embedded_enabled || _pending_ws_url.isEmpty() || _pending_token.isEmpty()) {
        return;
    }
    emit joinRequested(_pending_ws_url, _pending_token, _pending_metadata_json);
    _pending_ws_url.clear();
    _pending_token.clear();
    _pending_metadata_json.clear();
}
