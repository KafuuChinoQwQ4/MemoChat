#ifndef CALLCONTROLLER_H
#define CALLCONTROLLER_H

#include <QObject>
#include <QString>
#include <functional>
#include "global.h"

class CallSessionModel;
class ClientGateway;
class LivekitBridge;
class QJsonObject;

struct CallCommandPort
{
    std::function<void()> startVoiceChat;
    std::function<void()> startVideoChat;
    std::function<void()> acceptIncomingCall;
    std::function<void()> rejectIncomingCall;
    std::function<void()> endCurrentCall;
    std::function<void()> toggleCallMuted;
    std::function<void()> toggleCallCamera;
};

class CallController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CallSessionModel* callSession READ callSession NOTIFY callSurfaceChanged)
    Q_PROPERTY(LivekitBridge* livekitBridge READ livekitBridge NOTIFY callSurfaceChanged)

public:
    explicit CallController(ClientGateway* gateway, QObject* parent = nullptr);

    CallSessionModel* callSession() const;
    LivekitBridge* livekitBridge() const;
    QString callId() const;
    QString callType() const;
    QString peerName() const;
    QString peerIcon() const;
    QString stateText() const;
    bool callVisible() const;
    bool callIncoming() const;
    bool callActive() const;
    bool muted() const;
    bool cameraEnabled() const;

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

    void resetCallSurface();
    void startOutgoing(const QString& callId,
                       const QString& callType,
                       const QString& peerName,
                       const QString& peerIcon,
                       const QString& stateText,
                       qint64 startedAtMs,
                       qint64 expiresAtMs);
    void startIncoming(const QString& callId,
                       const QString& callType,
                       const QString& peerName,
                       const QString& peerIcon,
                       const QString& stateText,
                       qint64 startedAtMs,
                       qint64 expiresAtMs);
    void
    markAccepted(const QString& stateText, const QString& roomName, const QString& livekitUrl, qint64 acceptedAtMs);
    void markEnded(const QString& stateText);
    void setMediaStatusText(const QString& mediaStatusText);
    void markTokenReady(const QString& mediaStatusText);
    void setMediaLaunchJson(const QString& mediaLaunchJson);
    void setMuted(bool muted);
    void setCameraEnabled(bool enabled);
    void leaveRoom();
    void toggleMic();
    void toggleCamera();
    void requestJoinRoom(const QString& wsUrl, const QString& token, const QString& metadataJson);

    void syncSurface(CallSessionModel* callSession, LivekitBridge* livekitBridge);
    void setCommandPort(CallCommandPort port);

signals:
    void callSurfaceChanged();

private:
    void post(const QString& path, const QJsonObject& payload, ReqId reqId) const;

    ClientGateway* _gateway = nullptr;
    CallSessionModel* _call_session = nullptr;
    LivekitBridge* _livekit_bridge = nullptr;
    CallCommandPort _command_port;
};

#endif // CALLCONTROLLER_H
