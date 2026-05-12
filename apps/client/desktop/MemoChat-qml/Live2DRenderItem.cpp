#include "Live2DRenderItem.h"

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <QQuickWindow>
#include <QtMath>

Live2DRenderItem::Live2DRenderItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setFillColor(Qt::transparent);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setPerformanceHint(QQuickPaintedItem::FastFBOResizing, true);
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
        if (window) {
            connect(window, &QWindow::visibleChanged, this, [this](bool) {
                updateFrameTimer();
            }, Qt::UniqueConnection);
        }
        updateFrameTimer();
    });
    updateFrameTimer();
}

void Live2DRenderItem::paint(QPainter *painter)
{
    if (!painter) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    const QRectF bounds = boundingRect().adjusted(8, 8, -8, -8);
    if (bounds.width() <= 0 || bounds.height() <= 0) {
        return;
    }

    QLinearGradient bodyGradient(bounds.topLeft(), bounds.bottomRight());
    bodyGradient.setColorAt(0.0, QColor(116, 178, 186, 220));
    bodyGradient.setColorAt(1.0, QColor(244, 165, 132, 220));

    painter->setPen(QPen(QColor(255, 255, 255, 180), 2));
    painter->setBrush(bodyGradient);
    painter->drawRoundedRect(bounds, 22, 22);

    const qreal headSize = qMin(bounds.width() * 0.62, bounds.height() * 0.55);
    const QPointF headCenter(bounds.center().x(), bounds.top() + headSize * 0.58);
    const qreal bob = qSin(_idle_phase * 2.4) * qBound(1.0, _intensity * 5.0, 5.0);
    const qreal breathe = 1.0 + qSin(_idle_phase * 1.8) * 0.018;
    const QRectF head(headCenter.x() - (headSize * breathe) / 2.0,
                      headCenter.y() - (headSize * breathe) / 2.0 + bob,
                      headSize * breathe,
                      headSize * breathe);

    QColor faceColor(255, 244, 230, 235);
    if (_emotion == QStringLiteral("cheerful") || _expression.contains(QStringLiteral("smile"))) {
        faceColor = QColor(255, 238, 214, 240);
    } else if (_expression == QStringLiteral("concerned")) {
        faceColor = QColor(231, 240, 246, 235);
    }

    painter->setPen(QPen(QColor(116, 122, 132, 120), 2));
    painter->setBrush(faceColor);
    painter->drawEllipse(head);

    const qreal eyeOffsetX = head.width() * 0.19;
    const qreal eyeY = head.center().y() - head.height() * 0.08;
    const qreal blink = (qSin(_idle_phase * 3.1) > 0.985) ? 0.22 : 1.0;
    const qreal gazeDx = (_gaze_x - 0.5) * head.width() * 0.07;
    const qreal gazeDy = (_gaze_y - 0.5) * head.height() * 0.07;
    painter->setBrush(QColor(42, 52, 64, 235));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPointF(head.center().x() - eyeOffsetX + gazeDx, eyeY + gazeDy), 5.0, 6.5 * blink);
    painter->drawEllipse(QPointF(head.center().x() + eyeOffsetX + gazeDx, eyeY + gazeDy), 5.0, 6.5 * blink);

    painter->setPen(QPen(QColor(88, 56, 66, 220), 3, Qt::SolidLine, Qt::RoundCap));
    const qreal mouthWidth = head.width() * (0.16 + _intensity * 0.10);
    const qreal speakingPulse = _motion == QStringLiteral("talk") || _motion == QStringLiteral("speak")
                                    ? (0.5 + 0.5 * qSin(_idle_phase * 15.0)) * 0.16
                                    : 0.0;
    const qreal mouthOpen = 3.0 + qBound(0.0, _lip_sync_value + speakingPulse, 1.0) * head.height() * 0.11;
    const QPointF mouthCenter(head.center().x(), head.center().y() + head.height() * 0.18);
    painter->drawArc(QRectF(mouthCenter.x() - mouthWidth / 2.0,
                            mouthCenter.y() - mouthOpen / 2.0,
                            mouthWidth,
                            mouthOpen),
                     0,
                     -180 * 16);

    painter->setPen(QPen(QColor(255, 255, 255, 210), 1));
    painter->setBrush(QColor(255, 255, 255, 38));
    const QRectF badge(bounds.left() + 12, bounds.bottom() - 34, bounds.width() - 24, 22);
    painter->drawRoundedRect(badge, 7, 7);
    painter->setPen(QColor(54, 65, 78, 210));
    painter->drawText(badge, Qt::AlignCenter, QStringLiteral("Live2D adapter: %1 / %2").arg(_expression, _motion));
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

qreal Live2DRenderItem::boundedUnit(qreal value, qreal fallback)
{
    if (qIsNaN(value)) {
        return fallback;
    }
    return qBound(0.0, value, 1.0);
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
