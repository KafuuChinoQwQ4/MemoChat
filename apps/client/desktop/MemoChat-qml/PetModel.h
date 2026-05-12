#ifndef PETMODEL_H
#define PETMODEL_H

#include <QJsonObject>
#include <QObject>
#include <QString>

class PetModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int sequence READ sequence NOTIFY changed)
    Q_PROPERTY(int schemaVersion READ schemaVersion NOTIFY changed)
    Q_PROPERTY(QString eventId READ eventId NOTIFY changed)
    Q_PROPERTY(QString turnId READ turnId NOTIFY changed)
    Q_PROPERTY(QString phase READ phase NOTIFY changed)
    Q_PROPERTY(QString speechText READ speechText NOTIFY changed)
    Q_PROPERTY(QString emotion READ emotion NOTIFY changed)
    Q_PROPERTY(QString expression READ expression NOTIFY changed)
    Q_PROPERTY(QString motion READ motion NOTIFY changed)
    Q_PROPERTY(qreal intensity READ intensity NOTIFY changed)
    Q_PROPERTY(qreal gazeX READ gazeX NOTIFY changed)
    Q_PROPERTY(qreal gazeY READ gazeY NOTIFY changed)
    Q_PROPERTY(qreal lipSyncValue READ lipSyncValue NOTIFY changed)

public:
    explicit PetModel(QObject *parent = nullptr);

    int sequence() const { return _sequence; }
    int schemaVersion() const { return _schema_version; }
    QString eventId() const { return _event_id; }
    QString turnId() const { return _turn_id; }
    QString phase() const { return _phase; }
    QString speechText() const { return _speech_text; }
    QString emotion() const { return _emotion; }
    QString expression() const { return _expression; }
    QString motion() const { return _motion; }
    qreal intensity() const { return _intensity; }
    qreal gazeX() const { return _gaze_x; }
    qreal gazeY() const { return _gaze_y; }
    qreal lipSyncValue() const { return _lip_sync_value; }

    Q_INVOKABLE void applyControlEvent(const QJsonObject &event);
    Q_INVOKABLE void clearSpeech();

signals:
    void changed();

private:
    static qreal boundedUnit(qreal value, qreal fallback = 0.0);
    void emitChangedIf(bool changed);

    int _sequence = 0;
    int _schema_version = 0;
    QString _event_id;
    QString _turn_id;
    QString _phase = QStringLiteral("idle");
    QString _speech_text;
    QString _speech_turn_id;
    QString _emotion = QStringLiteral("neutral");
    QString _expression = QStringLiteral("neutral");
    QString _motion = QStringLiteral("idle");
    qreal _intensity = 0.35;
    qreal _gaze_x = 0.5;
    qreal _gaze_y = 0.5;
    qreal _lip_sync_value = 0.0;
};

#endif // PETMODEL_H
