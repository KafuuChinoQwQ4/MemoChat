#ifndef LIVEKITBRIDGE_H
#define LIVEKITBRIDGE_H

#include <QObject>
#include <QString>

class LivekitBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString roomPageUrl READ roomPageUrl NOTIFY stateChanged)
    Q_PROPERTY(bool embeddedEnabled READ embeddedEnabled NOTIFY stateChanged)

public:
    explicit LivekitBridge(QObject *parent = nullptr);

    QString roomPageUrl() const;
    bool embeddedEnabled() const;

    void requestJoinRoom(const QString &wsUrl, const QString &token, const QString &metadataJson);

    Q_INVOKABLE void setPageReady(bool ready);
    Q_INVOKABLE void toggleMic();
    Q_INVOKABLE void toggleCamera();
    Q_INVOKABLE void leaveRoom();

    Q_INVOKABLE void reportRoomJoined();
    Q_INVOKABLE void reportRemoteTrackReady();
    Q_INVOKABLE void reportRoomDisconnected(const QString &reason, bool recoverable);
    Q_INVOKABLE void reportPermissionError(const QString &deviceType, const QString &message);
    Q_INVOKABLE void reportMediaError(const QString &message);
    Q_INVOKABLE void reportReconnecting(const QString &message);
    Q_INVOKABLE void reportLog(const QString &message);

signals:
    void stateChanged();
    void joinRequested(const QString &wsUrl, const QString &token, const QString &metadataJson);
    void micToggleRequested();
    void cameraToggleRequested();
    void leaveRequested();
    void roomJoined();
    void remoteTrackReady();
    void roomDisconnected(const QString &reason, bool recoverable);
    void permissionError(const QString &deviceType, const QString &message);
    void mediaError(const QString &message);
    void reconnecting(const QString &message);
    void bridgeLog(const QString &message);

private:
    void loadConfig();
    void flushPendingJoin();

    QString _room_page_url;
    bool _embedded_enabled = true;
    bool _page_ready = false;
    QString _pending_ws_url;
    QString _pending_token;
    QString _pending_metadata_json;
};

#endif // LIVEKITBRIDGE_H
