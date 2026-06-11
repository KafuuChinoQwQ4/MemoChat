#include "AppController.h"

#include "AppCoordinators.h"

#include <QDebug>

void AppController::bindAppCallSignals()
{
    connect(_features.callController.livekitBridge(),
            &LivekitBridge::roomJoined,
            this,
            [this]()
            {
                _call_coordinator->onLivekitRoomJoined();
            });
    connect(_features.callController.livekitBridge(),
            &LivekitBridge::remoteTrackReady,
            this,
            [this]()
            {
                _call_coordinator->onLivekitRemoteTrackReady();
            });
    connect(_features.callController.livekitBridge(),
            &LivekitBridge::roomDisconnected,
            this,
            [this](const QString& reason, bool recoverable)
            {
                _call_coordinator->onLivekitRoomDisconnected(reason, recoverable);
            });
    connect(_features.callController.livekitBridge(),
            &LivekitBridge::permissionError,
            this,
            [this](const QString& deviceType, const QString& message)
            {
                _call_coordinator->onLivekitPermissionError(deviceType, message);
            });
    connect(_features.callController.livekitBridge(),
            &LivekitBridge::mediaError,
            this,
            [this](const QString& message)
            {
                _call_coordinator->onLivekitMediaError(message);
            });
    connect(_features.callController.livekitBridge(),
            &LivekitBridge::reconnecting,
            this,
            [this](const QString& message)
            {
                _call_coordinator->onLivekitReconnecting(message);
            });
    connect(_features.callController.livekitBridge(),
            &LivekitBridge::bridgeLog,
            this,
            [](const QString& message)
            {
                qInfo().noquote() << "[livekit]" << message;
            });
}
