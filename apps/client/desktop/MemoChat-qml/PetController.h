#ifndef PETCONTROLLER_H
#define PETCONTROLLER_H

#include "PetModel.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QVariantMap>

class ClientGateway;

class PetController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PetModel* model READ model CONSTANT)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY stateChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString speechText READ speechText NOTIFY petStateChanged)
    Q_PROPERTY(QString emotion READ emotion NOTIFY petStateChanged)
    Q_PROPERTY(QString expression READ expression NOTIFY petStateChanged)
    Q_PROPERTY(QString motion READ motion NOTIFY petStateChanged)
    Q_PROPERTY(qreal intensity READ intensity NOTIFY petStateChanged)
    Q_PROPERTY(qreal gazeX READ gazeX NOTIFY petStateChanged)
    Q_PROPERTY(qreal gazeY READ gazeY NOTIFY petStateChanged)
    Q_PROPERTY(qreal lipSyncValue READ lipSyncValue NOTIFY petStateChanged)

public:
    explicit PetController(ClientGateway *gateway, QObject *parent = nullptr);
    ~PetController() override;

    PetModel *model() { return &_model; }
    QString sessionId() const { return _session_id; }
    bool streaming() const { return _streaming; }
    bool busy() const { return _busy; }
    QString error() const { return _error; }
    QString statusText() const { return _status_text; }
    QString speechText() const { return _model.speechText(); }
    QString emotion() const { return _model.emotion(); }
    QString expression() const { return _model.expression(); }
    QString motion() const { return _model.motion(); }
    qreal intensity() const { return _model.intensity(); }
    qreal gazeX() const { return _model.gazeX(); }
    qreal gazeY() const { return _model.gazeY(); }
    qreal lipSyncValue() const { return _model.lipSyncValue(); }

    Q_INVOKABLE void startSession();
    Q_INVOKABLE void sendText(const QString &text);
    Q_INVOKABLE void sendObservation(const QVariantMap &observation);
    Q_INVOKABLE void interrupt();
    Q_INVOKABLE void stopStream();
    Q_INVOKABLE void clearSpeech();

signals:
    void stateChanged();
    void petStateChanged();
    void controlEventReceived(const QVariantMap &event);

private:
    void postJson(const QUrl &url, const QJsonObject &payload, const QString &op);
    void startStream();
    void handleJsonReply(QNetworkReply *reply);
    void applyControlEvent(const QJsonObject &event);
    void consumeStreamBytes(const QByteArray &bytes);
    void consumeSseLine(const QByteArray &line);
    void setBusy(bool busy, const QString &statusText = QString());
    void setStatusText(const QString &statusText);
    void setError(const QString &error);
    QJsonObject authPayload() const;
    QJsonObject defaultObservationPayload() const;
    QUrl petUrl(const QString &path) const;
    QString encodedSessionId() const;

    ClientGateway *_gateway = nullptr;
    PetModel _model;
    QNetworkAccessManager _network;
    QNetworkAccessManager _stream_network;
    QPointer<QNetworkReply> _stream_reply;
    QByteArray _stream_buffer;
    QString _session_id;
    bool _streaming = false;
    bool _busy = false;
    QString _error;
    QString _status_text;
};

#endif // PETCONTROLLER_H
