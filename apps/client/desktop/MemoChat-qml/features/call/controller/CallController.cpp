#include "CallController.h"

#include "CallSessionModel.h"
#include "ClientGateway.h"
#include "CallRequestPayloads.h"
#include "LivekitBridge.h"
#include "httpmgr.h"
#include "global.h"
#include <QJsonObject>
#include <utility>

CallController::CallController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
    , _call_session(new CallSessionModel(this))
    , _livekit_bridge(new LivekitBridge(this))
{
    emit callSurfaceChanged();
}

CallSessionModel* CallController::callSession() const
{
    return _call_session;
}

LivekitBridge* CallController::livekitBridge() const
{
    return _livekit_bridge;
}

QString CallController::callId() const
{
    return _call_session->callId();
}

QString CallController::callType() const
{
    return _call_session->callType();
}

QString CallController::peerName() const
{
    return _call_session->peerName();
}

QString CallController::peerIcon() const
{
    return _call_session->peerIcon();
}

QString CallController::stateText() const
{
    return _call_session->stateText();
}

bool CallController::callVisible() const
{
    return _call_session->visible();
}

bool CallController::callIncoming() const
{
    return _call_session->incoming();
}

bool CallController::callActive() const
{
    return _call_session->active();
}

bool CallController::muted() const
{
    return _call_session->muted();
}

bool CallController::cameraEnabled() const
{
    return _call_session->cameraEnabled();
}

void CallController::startVoiceChat()
{
    if (_command_port.startVoiceChat)
    {
        _command_port.startVoiceChat();
    }
}

void CallController::startVideoChat()
{
    if (_command_port.startVideoChat)
    {
        _command_port.startVideoChat();
    }
}

void CallController::acceptIncomingCall()
{
    if (_command_port.acceptIncomingCall)
    {
        _command_port.acceptIncomingCall();
    }
}

void CallController::rejectIncomingCall()
{
    if (_command_port.rejectIncomingCall)
    {
        _command_port.rejectIncomingCall();
    }
}

void CallController::endCurrentCall()
{
    if (_command_port.endCurrentCall)
    {
        _command_port.endCurrentCall();
    }
}

void CallController::toggleCallMuted()
{
    if (_command_port.toggleCallMuted)
    {
        _command_port.toggleCallMuted();
    }
}

void CallController::toggleCallCamera()
{
    if (_command_port.toggleCallCamera)
    {
        _command_port.toggleCallCamera();
    }
}

void CallController::startCall(int uid, const QString& token, int peerUid, const QString& callType) const
{
    Q_UNUSED(uid);
    Q_UNUSED(token);
    post(QStringLiteral("/api/call/start"),
                        memochat::call_request_payload::buildStartCallPayload(peerUid, callType),
                        ReqId::ID_CALL_START);
}

void CallController::acceptCall(int uid, const QString& token, const QString& callId) const
{
    Q_UNUSED(uid);
    Q_UNUSED(token);
    post(QStringLiteral("/api/call/accept"),
                        memochat::call_request_payload::buildCallIdPayload(callId),
                        ReqId::ID_CALL_ACCEPT);
}

void CallController::rejectCall(int uid, const QString& token, const QString& callId) const
{
    Q_UNUSED(uid);
    Q_UNUSED(token);
    post(QStringLiteral("/api/call/reject"),
                        memochat::call_request_payload::buildCallIdPayload(callId),
                        ReqId::ID_CALL_REJECT);
}

void CallController::cancelCall(int uid, const QString& token, const QString& callId) const
{
    Q_UNUSED(uid);
    Q_UNUSED(token);
    post(QStringLiteral("/api/call/cancel"),
                        memochat::call_request_payload::buildCallIdPayload(callId),
                        ReqId::ID_CALL_CANCEL);
}

void CallController::hangupCall(int uid, const QString& token, const QString& callId) const
{
    Q_UNUSED(uid);
    Q_UNUSED(token);
    post(QStringLiteral("/api/call/hangup"),
                        memochat::call_request_payload::buildCallIdPayload(callId),
                        ReqId::ID_CALL_HANGUP);
}

void CallController::fetchToken(int uid, const QString& token, const QString& callId, const QString& role) const
{
    Q_UNUSED(uid);
    Q_UNUSED(token);
    post(QStringLiteral("/api/call/token"),
                        memochat::call_request_payload::buildFetchTokenPayload(callId, role),
                        ReqId::ID_CALL_GET_TOKEN);
}

void CallController::resetCallSurface()
{
    _livekit_bridge->leaveRoom();
    _call_session->clear();
}

void CallController::startOutgoing(const QString& callId,
                                   const QString& callType,
                                   const QString& peerName,
                                   const QString& peerIcon,
                                   const QString& stateText,
                                   qint64 startedAtMs,
                                   qint64 expiresAtMs)
{
    _call_session->startOutgoing(callId, callType, peerName, peerIcon, stateText, startedAtMs, expiresAtMs);
}

void CallController::startIncoming(const QString& callId,
                                   const QString& callType,
                                   const QString& peerName,
                                   const QString& peerIcon,
                                   const QString& stateText,
                                   qint64 startedAtMs,
                                   qint64 expiresAtMs)
{
    _call_session->startIncoming(callId, callType, peerName, peerIcon, stateText, startedAtMs, expiresAtMs);
}

void CallController::markAccepted(const QString& stateText,
                                  const QString& roomName,
                                  const QString& livekitUrl,
                                  qint64 acceptedAtMs)
{
    _call_session->markAccepted(stateText, roomName, livekitUrl, acceptedAtMs);
}

void CallController::markEnded(const QString& stateText)
{
    _call_session->markEnded(stateText);
}

void CallController::setMediaStatusText(const QString& mediaStatusText)
{
    _call_session->setMediaStatusText(mediaStatusText);
}

void CallController::markTokenReady(const QString& mediaStatusText)
{
    _call_session->markTokenReady(mediaStatusText);
}

void CallController::setMediaLaunchJson(const QString& mediaLaunchJson)
{
    _call_session->setMediaLaunchJson(mediaLaunchJson);
}

void CallController::setMuted(bool muted)
{
    _call_session->setMuted(muted);
}

void CallController::setCameraEnabled(bool enabled)
{
    _call_session->setCameraEnabled(enabled);
}

void CallController::leaveRoom()
{
    _livekit_bridge->leaveRoom();
}

void CallController::toggleMic()
{
    _livekit_bridge->toggleMic();
}

void CallController::toggleCamera()
{
    _livekit_bridge->toggleCamera();
}

void CallController::requestJoinRoom(const QString& wsUrl, const QString& token, const QString& metadataJson)
{
    _livekit_bridge->requestJoinRoom(wsUrl, token, metadataJson);
}

void CallController::syncSurface(CallSessionModel* callSession, LivekitBridge* livekitBridge)
{
    Q_UNUSED(callSession);
    Q_UNUSED(livekitBridge);
    emit callSurfaceChanged();
}

void CallController::setCommandPort(CallCommandPort port)
{
    _command_port = std::move(port);
}

void CallController::post(const QString& path, const QJsonObject& payload, ReqId reqId) const
{
    if (!_gateway || !_gateway->httpMgr())
    {
        return;
    }
    _gateway->httpMgr()->PostHttpReq(QUrl(gate_url_prefix + path),
                                     payload,
                                     reqId,
                                     Modules::CALLMOD,
                                     QStringLiteral("call"));
}
