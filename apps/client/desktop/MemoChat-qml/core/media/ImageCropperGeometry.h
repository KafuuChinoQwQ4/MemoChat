#ifndef IMAGECROPPERGEOMETRY_H
#define IMAGECROPPERGEOMETRY_H

#include <QBitmap>
#include <QRect>
#include <QSize>

namespace ImageCropperGeometry
{

struct ImageFit
{
    QRect imageRect;
    QSize scaledSize;
    double scaledRate = 1.0;
};

ImageFit fitImageIntoLabel(const QSize& imageSize, const QSize& labelSize);
QRect sourceRectForCropper(const QRect& cropperRect, const QRect& imageRect, double scaledRate);
QRect centeredRect(const QSize& containerSize, const QSize& rectSize);
QRect defaultVariableCropperRect(const QSize& labelSize, const QSize& imageSize);
QRect wheelZoomRect(const QRect& cropperRect,
                    const QRect& imageRect,
                    const QSize& minimumSize,
                    qreal factor,
                    bool squareAspect);
QBitmap ellipseMask(const QSize& size);

} // namespace ImageCropperGeometry

#endif // IMAGECROPPERGEOMETRY_H
