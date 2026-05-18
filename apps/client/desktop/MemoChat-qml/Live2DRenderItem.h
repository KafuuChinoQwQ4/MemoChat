#ifndef LIVE2DRENDERITEM_H
#define LIVE2DRENDERITEM_H

#include "Live2DRenderer.h"

#include <QElapsedTimer>
#include <QMetaObject>
#include <QQuickFramebufferObject>
#include <QTimer>
#include <QVariantMap>

class Live2DRenderItem : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(QString modelRoot READ modelRoot WRITE setModelRoot NOTIFY modelSourceChanged)
    Q_PROPERTY(QString modelJson READ modelJson WRITE setModelJson NOTIFY modelSourceChanged)
    Q_PROPERTY(QString motionDirectory READ motionDirectory WRITE setMotionDirectory NOTIFY modelSourceChanged)
    Q_PROPERTY(QString expressionDirectory READ expressionDirectory WRITE setExpressionDirectory NOTIFY modelSourceChanged)
    Q_PROPERTY(QString expression READ expression WRITE setExpression NOTIFY visualStateChanged)
    Q_PROPERTY(QString motion READ motion WRITE setMotion NOTIFY visualStateChanged)
    Q_PROPERTY(QString emotion READ emotion WRITE setEmotion NOTIFY visualStateChanged)
    Q_PROPERTY(qreal intensity READ intensity WRITE setIntensity NOTIFY visualStateChanged)
    Q_PROPERTY(qreal gazeX READ gazeX WRITE setGazeX NOTIFY visualStateChanged)
    Q_PROPERTY(qreal gazeY READ gazeY WRITE setGazeY NOTIFY visualStateChanged)
    Q_PROPERTY(qreal lipSyncValue READ lipSyncValue WRITE setLipSyncValue NOTIFY visualStateChanged)
    Q_PROPERTY(int actionSerial READ actionSerial WRITE setActionSerial NOTIFY visualStateChanged)
    Q_PROPERTY(bool persistentMotion READ persistentMotion WRITE setPersistentMotion NOTIFY visualStateChanged)
    Q_PROPERTY(QString renderStatus READ renderStatus NOTIFY renderStatusChanged)
    Q_PROPERTY(QString renderError READ renderError NOTIFY renderStatusChanged)

public:
    explicit Live2DRenderItem(QQuickItem *parent = nullptr);

    QString modelRoot() const { return _model_root; }
    QString modelJson() const { return _model_json; }
    QString motionDirectory() const { return _motion_directory; }
    QString expressionDirectory() const { return _expression_directory; }
    QString expression() const { return _expression; }
    QString motion() const { return _motion; }
    QString emotion() const { return _emotion; }
    qreal intensity() const { return _intensity; }
    qreal gazeX() const { return _gaze_x; }
    qreal gazeY() const { return _gaze_y; }
    qreal lipSyncValue() const { return _lip_sync_value; }
    int actionSerial() const { return _action_serial; }
    bool persistentMotion() const { return _persistent_motion; }
    QString renderStatus() const { return _render_status; }
    QString renderError() const { return _render_error; }

    Renderer *createRenderer() const override;
    QString resolvedModelPath() const;
    Live2DVisualState visualState() const;
    void setRenderStatusFromRenderer(const QString &status,
                                     const QString &error,
                                     const QString &modelPath);

    Q_INVOKABLE void applyControlEvent(const QVariantMap &event);

public slots:
    void setModelRoot(const QString &value);
    void setModelJson(const QString &value);
    void setMotionDirectory(const QString &value);
    void setExpressionDirectory(const QString &value);
    void setExpression(const QString &value);
    void setMotion(const QString &value);
    void setEmotion(const QString &value);
    void setIntensity(qreal value);
    void setGazeX(qreal value);
    void setGazeY(qreal value);
    void setLipSyncValue(qreal value);
    void setActionSerial(int value);
    void setPersistentMotion(bool value);

signals:
    void modelSourceChanged();
    void visualStateChanged();
    void renderStatusChanged();

private:
    static qreal boundedUnit(qreal value, qreal fallback = 0.0);
    static QString resolveModelPath(const QString &modelRoot, const QString &modelJson);
    void updateVisual();
    void updateFrameTimer();

    QString _model_root;
    QString _model_json;
    QString _motion_directory;
    QString _expression_directory;
    QString _expression = QStringLiteral("neutral");
    QString _motion = QStringLiteral("idle");
    QString _emotion = QStringLiteral("neutral");
    qreal _intensity = 0.35;
    qreal _gaze_x = 0.5;
    qreal _gaze_y = 0.5;
    qreal _lip_sync_value = 0.0;
    int _action_serial = 0;
    bool _persistent_motion = false;
    QString _render_status = QStringLiteral("loading");
    QString _render_error;
    QString _render_model_path;
    QMetaObject::Connection _window_visible_connection;
    QTimer _frame_timer;
    QElapsedTimer _animation_clock;
    qreal _idle_phase = 0.0;
};

#endif // LIVE2DRENDERITEM_H
