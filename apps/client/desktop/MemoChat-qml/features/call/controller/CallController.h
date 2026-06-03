#ifndef CALLCONTROLLER_H
#define CALLCONTROLLER_H

#include <QObject>
#include <QString>
#include "global.h"

class CallSessionModel;
class ClientGateway;
class LivekitBridge;
class QJsonObject;

class CallController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CallSessionModel* callSession READ callSession NOTIFY callSurfaceChanged)
    Q_PROPERTY(LivekitBridge* livekitBridge READ livekitBridge NOTIFY callSurfaceChanged)

public:
    explicit CallController(ClientGateway* gateway, QObject* parent = nullptr);

    CallSessionModel* callSession() const;
    LivekitBridge* livekitBridge() const;

    Q_INVOKABLE void startVoiceChat();
    Q_INVOKABLE void startVideoChat();
    Q_INVOKABLE void acceptIncomingCall();
    Q_INVOKABLE void rejectIncomingCall();
    Q_INVOKABLE void endCurrentCall();
    Q_INVOKABLE void toggleCallMuted();
    Q_INVOKABLE void toggleCallCamera();

    void startCall(int uid, const QString& token, int peerUid, const QString& callType) const;
    void acceptCall(int uid, const QString& token, const QString& callId) const;
    void rejectCall(int uid, const QString& token, const QString& callId) const;
    void cancelCall(int uid, const QString& token, const QString& callId) const;
    void hangupCall(int uid, const QString& token, const QString& callId) const;
    void fetchToken(int uid, const QString& token, const QString& callId, const QString& role) const;

    void syncSurface(CallSessionModel* callSession, LivekitBridge* livekitBridge);

signals:
    void callSurfaceChanged();
    void startVoiceChatRequested();
    void startVideoChatRequested();
    void acceptIncomingCallRequested();
    void rejectIncomingCallRequested();
    void endCurrentCallRequested();
    void toggleCallMutedRequested();
    void toggleCallCameraRequested();

private:
    void post(const QString& path, const QJsonObject& payload, ReqId reqId) const;

    ClientGateway* _gateway = nullptr;
    CallSessionModel* _call_session = nullptr;
    LivekitBridge* _livekit_bridge = nullptr;
};

#endif // CALLCONTROLLER_H
