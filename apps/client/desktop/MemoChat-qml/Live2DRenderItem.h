#ifndef LIVE2DRENDERITEM_H
#define LIVE2DRENDERITEM_H

#include <QQuickPaintedItem>
#include <QVariantMap>

class Live2DRenderItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString expression READ expression WRITE setExpression NOTIFY visualStateChanged)
    Q_PROPERTY(QString motion READ motion WRITE setMotion NOTIFY visualStateChanged)
    Q_PROPERTY(QString emotion READ emotion WRITE setEmotion NOTIFY visualStateChanged)
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY visualStateChanged)
    Q_PROPERTY(qreal gazeX READ gazeX WRITE setGazeX NOTIFY visualStateChanged)
    Q_PROPERTY(qreal gazeY READ gazeY WRITE setGazeY NOTIFY visualStateChanged)
    Q_PROPERTY(qreal lipSyncValue READ lipSyncValue WRITE setLipSyncValue NOTIFY visualStateChanged)

public:
    explicit Live2DRenderItem(QQuickItem *parent = nullptr);

    QString expression() const { return _expression; }
    QString motion() const { return _motion; }
    QString emotion() const { return _emotion; }
    qreal intensity() const { return _intensity; }
    qreal gazeX() const { return _gaze_x; }
    qreal gazeY() const { return _gaze_y; }
    qreal lipSyncValue() const { return _lip_sync_value; }

    void paint(QPainter *painter) override;

    Q_INVOKABLE void applyControlEvent(const QVariantMap &event);

public slots:
    void setExpression(const QString &value);
    void setMotion(const QString &value);
    void setEmotion(const QString &value);
    void setIntensity(qreal value);
    void setGazeX(qreal value);
    void setGazeY(qreal value);
    void setLipSyncValue(qreal value);

signals:
    void visualStateChanged();

private:
    static qreal boundedUnit(qreal value, qreal fallback = 0.0);
    void updateVisual();

    QString _expression = QStringLiteral("neutral");
    QString _motion = QStringLiteral("idle");
    QString _emotion = QStringLiteral("neutral");
    qreal _intensity = 0.35;
    qreal _gaze_x = 0.5;
    qreal _gaze_y = 0.5;
    qreal _lip_sync_value = 0.0;
};

#endif // LIVE2DRENDERITEM_H
