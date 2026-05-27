#include "Live2DRenderItem.h"

#include "Live2DOfficialOpenGLRenderer.h"

#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QPointer>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QtMath>
#include <memory>

namespace {
QString loadingStatus()
{
    return QStringLiteral("loading");
}

QString readyStatus()
{
    return QStringLiteral("ready");
}

QString errorStatus()
{
    return QStringLiteral("error");
}

class Live2DFboRenderer final : public QQuickFramebufferObject::Renderer
{
public:
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(0);
        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject *item) override
    {
        auto *live2dItem = qobject_cast<Live2DRenderItem *>(item);
        if (!live2dItem) {
            return;
        }

        const QString nextModelPath = live2dItem->resolvedModelPath();
        const QString nextMotionDirectory = live2dItem->motionDirectory();
        const QString nextExpressionDirectory = live2dItem->expressionDirectory();
        if (_model_path != nextModelPath
            || _motion_directory != nextMotionDirectory
            || _expression_directory != nextExpressionDirectory) {
            _model_path = nextModelPath;
            _motion_directory = nextMotionDirectory;
            _expression_directory = nextExpressionDirectory;
            _render_error.clear();
            _official_renderer.reset();
            _native_attempted = false;
            _last_render_status.clear();
            _last_render_error.clear();
            _last_reported_model_path.clear();
            qInfo().noquote() << "Live2D model source changed:" << _model_path;
            reportStatus(live2dItem,
                         loadingStatus(),
                         QString(),
                         _model_path);
        }

        _item = live2dItem;
        _visual_state = live2dItem->visualState();
        _item_size = QSize(qMax(1, qCeil(live2dItem->width())),
                           qMax(1, qCeil(live2dItem->height())));
    }

    void render() override
    {
        if (_item_size.width() <= 0 || _item_size.height() <= 0) {
            return;
        }

        QOpenGLFunctions *functions = QOpenGLContext::currentContext()
                                          ? QOpenGLContext::currentContext()->functions()
                                          : nullptr;
        if (!functions) {
            return;
        }

        if (!_native_attempted) {
            _native_attempted = true;
            _official_renderer = std::make_unique<Live2DOfficialOpenGLRenderer>(_model_path,
                                                                                _motion_directory,
                                                                                _expression_directory);
            if (!_official_renderer->isReady()) {
                _render_error = _official_renderer->errorString();
                qWarning().noquote() << "Live2D official OpenGL renderer unavailable for"
                                     << _model_path << ":" << _render_error;
                _official_renderer.reset();
                reportStatus(errorStatus(), _render_error);
            } else {
                _render_error.clear();
                qInfo().noquote() << "Live2D official OpenGL renderer ready for" << _model_path;
                reportStatus(readyStatus(), QString());
            }
        }

        if (_official_renderer && _official_renderer->render(_item_size, _visual_state)) {
            QQuickOpenGLUtils::resetOpenGLState();
            reportStatus(readyStatus(), QString());
            return;
        }

        if (_official_renderer) {
            _render_error = QStringLiteral("official renderer returned false while drawing");
            qWarning().noquote() << "Live2D official OpenGL renderer draw failed for"
                                 << _model_path << ":" << _render_error;
            _official_renderer.reset();
            reportStatus(errorStatus(), _render_error);
        } else if (_render_error.isEmpty()) {
            _render_error = QStringLiteral("official renderer is unavailable");
            reportStatus(errorStatus(), _render_error);
        }

        functions->glViewport(0, 0, _item_size.width(), _item_size.height());
        functions->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        functions->glClear(GL_COLOR_BUFFER_BIT);
        QQuickOpenGLUtils::resetOpenGLState();
    }

private:
    void reportStatus(const QString &status, const QString &error)
    {
        reportStatus(_item, status, error, _model_path);
    }

    void reportStatus(Live2DRenderItem *item,
                      const QString &status,
                      const QString &error,
                      const QString &modelPath)
    {
        if (!item) {
            return;
        }
        if (_last_render_status == status
            && _last_render_error == error
            && _last_reported_model_path == modelPath) {
            return;
        }
        _last_render_status = status;
        _last_render_error = error;
        _last_reported_model_path = modelPath;
        QPointer<Live2DRenderItem> itemPointer(item);
        QMetaObject::invokeMethod(item,
                                  [itemPointer, status, error, modelPath]() {
                                      if (itemPointer) {
                                          itemPointer->setRenderStatusFromRenderer(status,
                                                                                   error,
                                                                                   modelPath);
                                      }
                                  },
                                  Qt::QueuedConnection);
    }

    QPointer<Live2DRenderItem> _item;
    QString _model_path;
    QString _motion_directory;
    QString _expression_directory;
    QString _render_error;
    QString _last_render_status;
    QString _last_render_error;
    QString _last_reported_model_path;
    QSize _item_size;
    Live2DVisualState _visual_state;
    bool _native_attempted = false;
    std::unique_ptr<Live2DOfficialOpenGLRenderer> _official_renderer;
};

QString resolveModelPath(const QString &modelRoot, const QString &modelJson)
{
    const QString cleaned = modelJson.trimmed();
    if (cleaned.isEmpty()) {
        return Live2DOfficialOpenGLRenderer::defaultModelPath();
    }
    if (cleaned.startsWith(QStringLiteral("qrc:/"))) {
        return QStringLiteral(":") + QUrl(cleaned).path();
    }
    if (cleaned.startsWith(QStringLiteral(":/"))) {
        return QDir::cleanPath(cleaned);
    }

    const QUrl url(cleaned);
    if (url.isLocalFile()) {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (QDir::isAbsolutePath(cleaned)) {
        return QDir::cleanPath(cleaned);
    }

    QStringList candidates;
    const QString root = modelRoot.trimmed();
    if (!root.isEmpty()) {
        candidates << QDir(root).absoluteFilePath(cleaned);
    }
    candidates << QDir::current().absoluteFilePath(cleaned);
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }
    return QDir::cleanPath(candidates.value(0, QDir::current().absoluteFilePath(cleaned)));
}
} // namespace

Live2DRenderItem::Live2DRenderItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    setAntialiasing(true);
    setMirrorVertically(true);
    _animation_clock.start();
    _frame_timer.setTimerType(Qt::PreciseTimer);
    _frame_timer.setInterval(1000 / _target_fps);
    connect(&_frame_timer, &QTimer::timeout, this, [this]() {
        const qreal elapsed = _animation_clock.elapsed() / 1000.0;
        _idle_phase = elapsed;
        update();
    });
    connect(this, &QQuickItem::visibleChanged, this, &Live2DRenderItem::updateFrameTimer);
    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *window) {
        if (_window_visible_connection) {
            disconnect(_window_visible_connection);
            _window_visible_connection = {};
        }
        if (window) {
            _window_visible_connection = connect(window, &QWindow::visibleChanged, this, [this](bool) {
                updateFrameTimer();
            });
        }
        updateFrameTimer();
    });
    updateFrameTimer();
}

QQuickFramebufferObject::Renderer *Live2DRenderItem::createRenderer() const
{
    return new Live2DFboRenderer();
}

void Live2DRenderItem::applyControlEvent(const QVariantMap &event)
{
    if (event.value(QStringLiteral("type")).toString() != QStringLiteral("pet.control")) {
        return;
    }
    setExpression(event.value(QStringLiteral("expression"), _expression).toString());
    setMotion(event.value(QStringLiteral("motion"), _motion).toString());
    setEmotion(event.value(QStringLiteral("emotion"), _emotion).toString());
    setIntensity(event.value(QStringLiteral("intensity"), _intensity).toDouble());
    const QVariantMap gaze = event.value(QStringLiteral("gaze")).toMap();
    setGazeX(gaze.value(QStringLiteral("x"), _gaze_x).toDouble());
    setGazeY(gaze.value(QStringLiteral("y"), _gaze_y).toDouble());
    const QVariantMap lipSync = event.value(QStringLiteral("lip_sync")).toMap();
    setLipSyncValue(lipSync.value(QStringLiteral("value"), _lip_sync_value).toDouble());
    if (event.contains(QStringLiteral("expression")) || event.contains(QStringLiteral("motion"))) {
        setActionSerial(_action_serial + 1);
    }
}

void Live2DRenderItem::setModelRoot(const QString &value)
{
    if (_model_root == value) {
        return;
    }
    _model_root = value;
    update();
    emit modelSourceChanged();
}

void Live2DRenderItem::setModelJson(const QString &value)
{
    if (_model_json == value) {
        return;
    }
    _model_json = value;
    update();
    emit modelSourceChanged();
}

void Live2DRenderItem::setMotionDirectory(const QString &value)
{
    if (_motion_directory == value) {
        return;
    }
    _motion_directory = value;
    update();
    emit modelSourceChanged();
}

void Live2DRenderItem::setExpressionDirectory(const QString &value)
{
    if (_expression_directory == value) {
        return;
    }
    _expression_directory = value;
    update();
    emit modelSourceChanged();
}

void Live2DRenderItem::setExpression(const QString &value)
{
    if (_expression == value) {
        return;
    }
    _expression = value;
    updateVisual();
}

void Live2DRenderItem::setMotion(const QString &value)
{
    if (_motion == value) {
        return;
    }
    _motion = value;
    updateVisual();
}

void Live2DRenderItem::setEmotion(const QString &value)
{
    if (_emotion == value) {
        return;
    }
    _emotion = value;
    updateVisual();
}

void Live2DRenderItem::setIntensity(qreal value)
{
    const qreal next = boundedUnit(value, _intensity);
    if (qFuzzyCompare(_intensity + 1.0, next + 1.0)) {
        return;
    }
    _intensity = next;
    updateVisual();
}

void Live2DRenderItem::setGazeX(qreal value)
{
    const qreal next = boundedUnit(value, _gaze_x);
    if (qFuzzyCompare(_gaze_x + 1.0, next + 1.0)) {
        return;
    }
    _gaze_x = next;
    updateVisual();
}

void Live2DRenderItem::setGazeY(qreal value)
{
    const qreal next = boundedUnit(value, _gaze_y);
    if (qFuzzyCompare(_gaze_y + 1.0, next + 1.0)) {
        return;
    }
    _gaze_y = next;
    updateVisual();
}

void Live2DRenderItem::setLipSyncValue(qreal value)
{
    const qreal next = boundedUnit(value, _lip_sync_value);
    if (qFuzzyCompare(_lip_sync_value + 1.0, next + 1.0)) {
        return;
    }
    _lip_sync_value = next;
    updateVisual();
}

void Live2DRenderItem::setActionSerial(int value)
{
    if (_action_serial == value) {
        return;
    }
    _action_serial = value;
    updateVisual();
}

void Live2DRenderItem::setPersistentMotion(bool value)
{
    if (_persistent_motion == value) {
        return;
    }
    _persistent_motion = value;
    updateVisual();
}

void Live2DRenderItem::setTargetFps(int value)
{
    const int next = qBound(15, value, 60);
    if (_target_fps == next) {
        return;
    }
    _target_fps = next;
    _frame_timer.setInterval(qMax(1, 1000 / _target_fps));
    emit targetFpsChanged();
}

qreal Live2DRenderItem::boundedUnit(qreal value, qreal fallback)
{
    if (qIsNaN(value)) {
        return fallback;
    }
    return qBound(0.0, value, 1.0);
}

QString Live2DRenderItem::resolveModelPath(const QString &modelRoot, const QString &modelJson)
{
    return ::resolveModelPath(modelRoot, modelJson);
}

QString Live2DRenderItem::resolvedModelPath() const
{
    return resolveModelPath(_model_root, _model_json);
}

Live2DVisualState Live2DRenderItem::visualState() const
{
    Live2DVisualState state;
    state.expression = _expression;
    state.motion = _motion;
    state.emotion = _emotion;
    state.intensity = _intensity;
    state.gazeX = _gaze_x;
    state.gazeY = _gaze_y;
    state.lipSyncValue = _lip_sync_value;
    state.idlePhase = _idle_phase;
    state.actionSerial = _action_serial;
    state.persistentMotion = _persistent_motion;
    return state;
}

void Live2DRenderItem::setRenderStatusFromRenderer(const QString &status,
                                                   const QString &error,
                                                   const QString &modelPath)
{
    if (_render_status == status
        && _render_error == error
        && _render_model_path == modelPath) {
        return;
    }
    _render_status = status;
    _render_error = error;
    _render_model_path = modelPath;
    emit renderStatusChanged();
}

void Live2DRenderItem::updateVisual()
{
    update();
    emit visualStateChanged();
}

void Live2DRenderItem::updateFrameTimer()
{
    const bool should_run = isVisible() && window() && window()->isVisible();
    if (should_run && !_frame_timer.isActive()) {
        _animation_clock.restart();
        _frame_timer.start();
    } else if (!should_run && _frame_timer.isActive()) {
        _frame_timer.stop();
    }
}
