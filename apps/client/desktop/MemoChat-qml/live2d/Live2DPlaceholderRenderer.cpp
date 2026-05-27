#include "Live2DPlaceholderRenderer.h"

#include <QColor>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRadialGradient>
#include <QtMath>

namespace {
QRectF centeredRect(const QPointF &center, qreal width, qreal height)
{
    return QRectF(center.x() - width / 2.0,
                  center.y() - height / 2.0,
                  width,
                  height);
}

void drawSoftShadow(QPainter *painter, const QPointF &center, qreal width, qreal height)
{
    QRadialGradient shadow(center, width * 0.50);
    shadow.setColorAt(0.0, QColor(38, 45, 62, 82));
    shadow.setColorAt(0.72, QColor(38, 45, 62, 28));
    shadow.setColorAt(1.0, QColor(38, 45, 62, 0));

    painter->setPen(Qt::NoPen);
    painter->setBrush(shadow);
    painter->drawEllipse(center, width * 0.50, height * 0.50);
}

void drawRibbon(QPainter *painter, const QPointF &base, qreal unit, bool mirrored)
{
    painter->save();
    painter->translate(base);
    painter->scale(mirrored ? -1.0 : 1.0, 1.0);

    QPainterPath ribbon;
    ribbon.moveTo(-unit * 0.03, -unit * 0.03);
    ribbon.cubicTo(unit * 0.18, -unit * 0.18,
                   unit * 0.40, -unit * 0.12,
                   unit * 0.45, unit * 0.06);
    ribbon.cubicTo(unit * 0.26, unit * 0.16,
                   unit * 0.10, unit * 0.12,
                   -unit * 0.02, unit * 0.03);
    ribbon.closeSubpath();

    QLinearGradient ribbonFill(QPointF(0, -unit * 0.18), QPointF(unit * 0.44, unit * 0.14));
    ribbonFill.setColorAt(0.0, QColor(128, 149, 214, 235));
    ribbonFill.setColorAt(1.0, QColor(88, 104, 169, 230));
    painter->setPen(QPen(QColor(68, 78, 132, 180), unit * 0.010));
    painter->setBrush(ribbonFill);
    painter->drawPath(ribbon);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 58));
    painter->drawEllipse(QPointF(unit * 0.20, -unit * 0.03), unit * 0.10, unit * 0.028);
    painter->restore();
}

void drawHairStrand(QPainter *painter, const QPointF &top, const QPointF &bottom, qreal width, const QColor &color)
{
    QPainterPath strand;
    strand.moveTo(top);
    strand.cubicTo(QPointF(top.x() - width * 0.22, top.y() + width * 0.55),
                   QPointF(bottom.x() - width * 0.20, bottom.y() - width * 0.40),
                   bottom);
    strand.cubicTo(QPointF(bottom.x() + width * 0.18, bottom.y() - width * 0.35),
                   QPointF(top.x() + width * 0.20, top.y() + width * 0.50),
                   top);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPath(strand);
}

void drawPlaceholderPet(QPainter *painter, const QRectF &bounds, const Live2DVisualState &state)
{
    const QRectF stage = bounds.adjusted(8, 8, -8, -8);
    const qreal unit = qMin(stage.width(), stage.height());
    if (unit <= 0) {
        return;
    }

    const qreal bob = qSin(state.idlePhase * 2.3) * qBound<qreal>(1.0, state.intensity * 5.0, 5.0);
    const qreal breathe = 1.0 + qSin(state.idlePhase * 1.6) * 0.018;
    const qreal gazeX = (state.gazeX - 0.5) * unit * 0.035;
    const qreal gazeY = (state.gazeY - 0.5) * unit * 0.024;
    const qreal tilt = (state.gazeX - 0.5) * 2.8 + qSin(state.idlePhase * 0.85) * 0.8;
    const QPointF base(stage.center().x(), stage.top() + unit * 0.52 + bob);

    drawSoftShadow(painter,
                   QPointF(stage.center().x(), stage.bottom() - unit * 0.07),
                   unit * 0.58,
                   unit * 0.14);

    painter->save();
    painter->translate(base);
    painter->rotate(tilt);
    painter->scale(breathe, breathe);

    const QPointF localOrigin(0, 0);
    Q_UNUSED(localOrigin);

    drawRibbon(painter, QPointF(-unit * 0.23, -unit * 0.05), unit, false);
    drawRibbon(painter, QPointF(unit * 0.23, -unit * 0.05), unit, true);

    const QRectF body = centeredRect(QPointF(0, unit * 0.18), unit * 0.43, unit * 0.62);
    QLinearGradient bodyFill(body.topLeft(), body.bottomRight());
    bodyFill.setColorAt(0.0, QColor(136, 154, 218, 244));
    bodyFill.setColorAt(0.58, QColor(105, 124, 194, 242));
    bodyFill.setColorAt(1.0, QColor(72, 84, 132, 240));
    painter->setPen(QPen(QColor(255, 255, 255, 54), unit * 0.012));
    painter->setBrush(bodyFill);
    painter->drawRoundedRect(body, unit * 0.08, unit * 0.08);

    const QRectF scarf = centeredRect(QPointF(0, -unit * 0.02), unit * 0.38, unit * 0.10);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(34, 52, 136, 245));
    painter->drawRoundedRect(scarf, unit * 0.035, unit * 0.035);

    const QRectF leftArm = centeredRect(QPointF(-unit * 0.27, unit * 0.17), unit * 0.10, unit * 0.40);
    const QRectF rightArm = centeredRect(QPointF(unit * 0.27, unit * 0.17), unit * 0.10, unit * 0.40);
    painter->setBrush(QColor(240, 244, 255, 236));
    painter->setPen(QPen(QColor(255, 255, 255, 46), unit * 0.008));
    painter->drawRoundedRect(leftArm, unit * 0.045, unit * 0.045);
    painter->drawRoundedRect(rightArm, unit * 0.045, unit * 0.045);

    painter->setBrush(QColor(244, 194, 186, 242));
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(centeredRect(QPointF(-unit * 0.27, unit * 0.39), unit * 0.095, unit * 0.060),
                             unit * 0.018,
                             unit * 0.018);
    painter->drawRoundedRect(centeredRect(QPointF(unit * 0.27, unit * 0.39), unit * 0.095, unit * 0.060),
                             unit * 0.018,
                             unit * 0.018);

    const QColor hairBack(226, 232, 250, 238);
    drawHairStrand(painter, QPointF(-unit * 0.20, -unit * 0.34), QPointF(-unit * 0.34, unit * 0.30), unit * 0.30, hairBack);
    drawHairStrand(painter, QPointF(unit * 0.20, -unit * 0.34), QPointF(unit * 0.34, unit * 0.30), unit * 0.30, hairBack);

    const QRectF head = centeredRect(QPointF(gazeX * 0.35, -unit * 0.30 + gazeY * 0.20),
                                     unit * 0.34,
                                     unit * 0.38);
    QLinearGradient faceFill(head.topLeft(), head.bottomRight());
    faceFill.setColorAt(0.0, QColor(255, 238, 225, 252));
    faceFill.setColorAt(1.0, QColor(244, 203, 194, 250));
    painter->setPen(QPen(QColor(74, 70, 82, 120), unit * 0.010));
    painter->setBrush(faceFill);
    painter->drawEllipse(head);

    QPainterPath bangs;
    bangs.moveTo(head.left() + head.width() * 0.08, head.top() + head.height() * 0.30);
    bangs.cubicTo(head.left() + head.width() * 0.24, head.top() - head.height() * 0.06,
                  head.left() + head.width() * 0.58, head.top() - head.height() * 0.10,
                  head.right() - head.width() * 0.05, head.top() + head.height() * 0.28);
    bangs.cubicTo(head.right() - head.width() * 0.18, head.top() + head.height() * 0.44,
                  head.left() + head.width() * 0.28, head.top() + head.height() * 0.40,
                  head.left() + head.width() * 0.08, head.top() + head.height() * 0.30);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(218, 225, 246, 242));
    painter->drawPath(bangs);

    const QRectF leftTail = centeredRect(QPointF(head.left() - unit * 0.07, head.center().y() + unit * 0.05),
                                         unit * 0.11,
                                         unit * 0.46);
    const QRectF rightTail = centeredRect(QPointF(head.right() + unit * 0.07, head.center().y() + unit * 0.05),
                                          unit * 0.11,
                                          unit * 0.46);
    painter->setBrush(QColor(210, 218, 242, 232));
    painter->drawRoundedRect(leftTail, unit * 0.05, unit * 0.05);
    painter->drawRoundedRect(rightTail, unit * 0.05, unit * 0.05);

    const QString expression = state.expression.toLower();
    const QString emotion = state.emotion.toLower();
    const bool cheerful = emotion == QStringLiteral("cheerful")
                          || expression.contains(QStringLiteral("smile"))
                          || expression.contains(QStringLiteral("脸红"));
    const bool concerned = expression.contains(QStringLiteral("concerned"))
                           || emotion == QStringLiteral("concerned");

    const qreal eyeY = head.center().y() - head.height() * 0.055;
    const qreal eyeOffsetX = head.width() * 0.18;
    const qreal blink = (qSin(state.idlePhase * 3.2) > 0.986) ? 0.20 : 1.0;
    const qreal eyeGazeX = (state.gazeX - 0.5) * head.width() * 0.050;
    const qreal eyeGazeY = (state.gazeY - 0.5) * head.height() * 0.040;

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(248, 145, 160, cheerful ? 92 : 58));
    painter->drawEllipse(QPointF(head.center().x() - head.width() * 0.23, head.center().y() + head.height() * 0.12),
                         head.width() * 0.055,
                         head.height() * 0.035);
    painter->drawEllipse(QPointF(head.center().x() + head.width() * 0.23, head.center().y() + head.height() * 0.12),
                         head.width() * 0.055,
                         head.height() * 0.035);

    painter->setBrush(QColor(44, 74, 106, 240));
    if (blink < 0.25) {
        painter->setPen(QPen(QColor(44, 74, 106, 230), unit * 0.010, Qt::SolidLine, Qt::RoundCap));
        painter->drawLine(QPointF(head.center().x() - eyeOffsetX - unit * 0.020, eyeY),
                          QPointF(head.center().x() - eyeOffsetX + unit * 0.020, eyeY));
        painter->drawLine(QPointF(head.center().x() + eyeOffsetX - unit * 0.020, eyeY),
                          QPointF(head.center().x() + eyeOffsetX + unit * 0.020, eyeY));
    } else {
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(head.center().x() - eyeOffsetX + eyeGazeX, eyeY + eyeGazeY),
                             unit * 0.018,
                             unit * 0.027 * blink);
        painter->drawEllipse(QPointF(head.center().x() + eyeOffsetX + eyeGazeX, eyeY + eyeGazeY),
                             unit * 0.018,
                             unit * 0.027 * blink);
        painter->setBrush(QColor(255, 255, 255, 218));
        painter->drawEllipse(QPointF(head.center().x() - eyeOffsetX + eyeGazeX - unit * 0.006,
                                     eyeY + eyeGazeY - unit * 0.010),
                             unit * 0.0045,
                             unit * 0.006);
        painter->drawEllipse(QPointF(head.center().x() + eyeOffsetX + eyeGazeX - unit * 0.006,
                                     eyeY + eyeGazeY - unit * 0.010),
                             unit * 0.0045,
                             unit * 0.006);
    }

    const QString motion = state.motion.toLower();
    const qreal speakingPulse = (motion == QStringLiteral("talk") || motion == QStringLiteral("speak"))
                                    ? (0.5 + 0.5 * qSin(state.idlePhase * 15.0)) * 0.18
                                    : 0.0;
    const qreal mouthSignal = qBound<qreal>(0.0, state.lipSyncValue + speakingPulse, 1.0);
    const QPointF mouthCenter(head.center().x(), head.center().y() + head.height() * 0.20);
    const qreal mouthWidth = head.width() * (cheerful ? 0.20 : 0.16);
    const qreal mouthHeight = head.height() * (0.030 + mouthSignal * 0.12);

    if (mouthSignal > 0.18) {
        painter->setPen(QPen(QColor(110, 62, 76, 220), unit * 0.006));
        painter->setBrush(QColor(126, 64, 84, 210));
        painter->drawEllipse(centeredRect(mouthCenter, mouthWidth * 0.62, mouthHeight));
    } else {
        painter->setPen(QPen(QColor(104, 63, 74, 230), unit * 0.010, Qt::SolidLine, Qt::RoundCap));
        const int startAngle = concerned ? 18 * 16 : 0;
        const int spanAngle = concerned ? 144 * 16 : -180 * 16;
        painter->drawArc(centeredRect(mouthCenter, mouthWidth, mouthHeight),
                         startAngle,
                         spanAngle);
    }

    const QRectF leftShoe = centeredRect(QPointF(-unit * 0.11, unit * 0.53), unit * 0.18, unit * 0.08);
    const QRectF rightShoe = centeredRect(QPointF(unit * 0.11, unit * 0.53), unit * 0.18, unit * 0.08);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(40, 42, 54, 238));
    painter->drawRoundedRect(leftShoe, unit * 0.035, unit * 0.035);
    painter->drawRoundedRect(rightShoe, unit * 0.035, unit * 0.035);

    painter->restore();
}
} // namespace

QString Live2DPlaceholderRenderer::rendererName() const
{
    return QStringLiteral("placeholder");
}

bool Live2DPlaceholderRenderer::isNative() const
{
    return false;
}

void Live2DPlaceholderRenderer::paint(QPainter *painter, const QRectF &itemBounds, const Live2DVisualState &state)
{
    if (!painter) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRectF bounds = itemBounds.adjusted(2, 2, -2, -2);
    if (bounds.width() <= 0 || bounds.height() <= 0) {
        return;
    }

    drawPlaceholderPet(painter, bounds, state);
}
