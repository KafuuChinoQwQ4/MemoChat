#include "Live2DRenderItem.h"

#include "Live2DOfficialOpenGLRenderer.h"

#include <QColor>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFont>
#include <QMetaObject>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QPointer>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <QRadialGradient>
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

QString compactErrorText(QString error)
{
    error = error.simplified();
    constexpr qsizetype kMaximumLength = 220;
    if (error.size() > kMaximumLength) {
        error = error.left(kMaximumLength - 3) + QStringLiteral("...");
    }
    return error;
}

QRectF centeredRect(const QPointF &center, qreal width, qreal height)
{
    return QRectF(center.x() - width / 2.0,
                  center.y() - height / 2.0,
                  width,
                  height);
}

void drawModelErrorMarker(QPainter *painter,
                          const QRectF &itemBounds,
                          const QString &modelPath,
                          const QString &error)
{
    if (!painter) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF bounds = itemBounds.adjusted(10, 10, -10, -10);
    if (bounds.width() <= 0 || bounds.height() <= 0) {
        return;
    }

    const qreal unit = qMin(bounds.width(), bounds.height());
    const QPointF center = bounds.center();

    QRadialGradient glow(center, unit * 0.50);
    glow.setColorAt(0.0, QColor(255, 76, 96, 70));
    glow.setColorAt(0.62, QColor(255, 76, 96, 26));
    glow.setColorAt(1.0, QColor(255, 76, 96, 0));
    painter->setPen(Qt::NoPen);
    painter->setBrush(glow);
    painter->drawEllipse(center, unit * 0.50, unit * 0.50);

    const QRectF badge = centeredRect(center, unit * 0.62, unit * 0.62);
    painter->setPen(QPen(QColor(255, 110, 126, 230), qMax<qreal>(2.0, unit * 0.018)));
    painter->setBrush(QColor(24, 28, 36, 210));
    painter->drawRoundedRect(badge, unit * 0.08, unit * 0.08);

    QPainterPath warning;
    warning.moveTo(center.x(), badge.top() + badge.height() * 0.20);
    warning.lineTo(badge.right() - badge.width() * 0.18, badge.bottom() - badge.height() * 0.18);
    warning.lineTo(badge.left() + badge.width() * 0.18, badge.bottom() - badge.height() * 0.18);
    warning.closeSubpath();
    painter->setPen(QPen(QColor(255, 214, 116, 240), qMax<qreal>(2.0, unit * 0.012)));
    painter->setBrush(QColor(255, 89, 112, 224));
    painter->drawPath(warning);

    QFont iconFont = painter->font();
    iconFont.setBold(true);
    iconFont.setPixelSize(qMax(22, qRound(unit * 0.17)));
    painter->setFont(iconFont);
    painter->setPen(QColor(255, 255, 255, 245));
    painter->drawText(warning.boundingRect(), Qt::AlignCenter, QStringLiteral("!"));

    const QString pathName = QFileInfo(modelPath).fileName();
    const QString title = QStringLiteral("模型显示错误");
    const QString details = pathName.isEmpty()
                                ? QStringLiteral("未收到 model3.json")
                                : QStringLiteral("加载失败：%1").arg(pathName);
    const QString reason = compactErrorText(error);
    const QString message = reason.isEmpty() ? details : details + QStringLiteral("\n") + reason;

    QRectF textRect(bounds.left(),
                    qMin(bounds.bottom() - unit * 0.22, badge.bottom() + unit * 0.06),
                    bounds.width(),
                    qMax<qreal>(unit * 0.20, bounds.bottom() - badge.bottom() - unit * 0.04));
    painter->setPen(QColor(255, 246, 249, 245));
    QFont titleFont = painter->font();
    titleFont.setBold(true);
    titleFont.setPixelSize(qMax(13, qRound(unit * 0.052)));
    painter->setFont(titleFont);
    painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, title);

    textRect.adjust(6, qMax<qreal>(18, unit * 0.075), -6, 0);
    QFont detailFont = painter->font();
    detailFont.setBold(false);
    detailFont.setPixelSize(qMax(10, qRound(unit * 0.032)));
    painter->setFont(detailFont);
    painter->setPen(QColor(255, 226, 232, 220));
    painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, message);
}

class Live2DFboRenderer final : public QQuickFramebufferObject::Renderer
{
public:
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
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
            update();
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

        QImage fallback(_item_size, QImage::Format_ARGB32_Premultiplied);
        fallback.fill(Qt::transparent);
        QPainter painter(&fallback);
        drawModelErrorMarker(&painter,
                             QRectF(QPointF(0, 0), QSizeF(_item_size)),
                             _model_path,
                             _render_error);
        painter.end();

        functions->glViewport(0, 0, _item_size.width(), _item_size.height());
        functions->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        functions->glClear(GL_COLOR_BUFFER_BIT);
        QOpenGLPaintDevice paintDevice(_item_size);
        QPainter fboPainter(&paintDevice);
        fboPainter.drawImage(QPoint(0, 0), fallback);
        fboPainter.end();
        QQuickOpenGLUtils::resetOpenGLState();
        update();
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
    _frame_timer.setInterval(16);
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
