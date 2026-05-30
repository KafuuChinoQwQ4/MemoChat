#ifndef LIVE2DPLACEHOLDERRENDERER_H
#define LIVE2DPLACEHOLDERRENDERER_H

#include "Live2DRenderer.h"

class Live2DPlaceholderRenderer final : public Live2DRenderer
{
public:
    QString rendererName() const override;
    bool isNative() const override;
    void paint(QPainter* painter, const QRectF& bounds, const Live2DVisualState& state) override;
};

#endif // LIVE2DPLACEHOLDERRENDERER_H
