#ifndef LIVE2DRENDERER_H
#define LIVE2DRENDERER_H

#include <QRectF>
#include <QString>
#include <QtGlobal>

class QPainter;

struct Live2DVisualState
{
    QString expression = QStringLiteral("neutral");
    QString motion = QStringLiteral("idle");
    QString emotion = QStringLiteral("neutral");
    qreal intensity = 0.35;
    qreal gazeX = 0.5;
    qreal gazeY = 0.5;
    qreal lipSyncValue = 0.0;
    qreal idlePhase = 0.0;
};

class Live2DRenderer
{
public:
    virtual ~Live2DRenderer() = default;

    virtual QString rendererName() const = 0;
    virtual bool isNative() const = 0;
    virtual void paint(QPainter *painter, const QRectF &bounds, const Live2DVisualState &state) = 0;
};

#endif // LIVE2DRENDERER_H
