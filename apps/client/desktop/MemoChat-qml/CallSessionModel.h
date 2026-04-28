#ifndef CALLSESSIONMODEL_H
#define CALLSESSIONMODEL_H

#include <QObject>
#include <QTimer>

class CallSessionModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY changed)
    Q_PROPERTY(bool incoming READ incoming NOTIFY changed)
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(bool tokenReady READ tokenReady NOTIFY changed)
    Q_PROPERTY(QString callId READ callId NOTIFY changed)
    Q_PROPERTY(QString callType READ callType NOTIFY changed)
    Q_PROPERTY(QString peerName READ peerName NOTIFY changed)
    Q_PROPERTY(QString peerIcon READ peerIcon NOTIFY changed)
    Q_PROPERTY(QString stateText READ stateText NOTIFY changed)
    Q_PROPERTY(QString roomName READ roomName NOTIFY changed)
    Q_PROPERTY(QString livekitUrl READ livekitUrl NOTIFY changed)
    Q_PROPERTY(QString mediaLaunchJson READ mediaLaunchJson NOTIFY changed)
    Q_PROPERTY(QString mediaStatusText READ mediaStatusText NOTIFY changed)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)
    Q_PROPERTY(bool cameraEnabled READ cameraEnabled NOTIFY changed)

public:
    explicit CallSessionModel(QObject *parent = nullptr);

    bool visible() const;
    bool incoming() const;
    bool active() const;
    bool tokenReady() const;
    QString callId() const;
    QString callType() const;
    QString peerName() const;
    QString peerIcon() const;
    QString stateText() const;
    QString roomName() const;
    QString livekitUrl() const;
    QString mediaLaunchJson() const;
    QString mediaStatusText() const;
    int elapsedSeconds() const;
    bool muted() const;
    bool cameraEnabled() const;

    void startOutgoing(const QString &callId, const QString &callType, const QString &peerName,
                       const QString &peerIcon, const QString &stateText, qint64 startedAtMs, qint64 expiresAtMs);
    void startIncoming(const QString &callId, const QString &callType, const QString &peerName,
                       const QString &peerIcon, const QString &stateText, qint64 startedAtMs, qint64 expiresAtMs);
    void markAccepted(const QString &stateText, const QString &roomName, const QString &livekitUrl, qint64 acceptedAtMs);
    void markTokenReady(const QString &mediaStatusText);
    void setMediaLaunchJson(const QString &mediaLaunchJson);
    void setMediaStatusText(const QString &mediaStatusText);
    void markEnded(const QString &stateText);
    void clear();
    void setMuted(bool muted);
    void setCameraEnabled(bool enabled);

signals:
    void changed();

private:
    void onTick();

    bool _visible = false;
    bool _incoming = false;
    bool _active = false;
    bool _token_ready = false;
    QString _call_id;
    QString _call_type;
    QString _peer_name;
    QString _peer_icon;
    QString _state_text;
    QString _room_name;
    QString _livekit_url;
    QString _media_launch_json;
    QString _media_status_text;
    qint64 _started_at_ms = 0;
    qint64 _accepted_at_ms = 0;
    qint64 _expires_at_ms = 0;
    int _elapsed_seconds = 0;
    bool _muted = false;
    bool _camera_enabled = true;
    QTimer _tick_timer;
};

#endif // CALLSESSIONMODEL_H
