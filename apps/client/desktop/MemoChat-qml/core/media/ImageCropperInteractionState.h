#ifndef IMAGECROPPERINTERACTIONSTATE_H
#define IMAGECROPPERINTERACTIONSTATE_H

#include "ImageCropperHitTest.h"
#include "ImageCropperTypes.h"

#include <QPoint>
#include <QRect>
#include <QSize>

namespace ImageCropperInteractionState
{

QPoint clampedMoveDelta(const QRect& cropperRect, const QRect& imageRect, const QPoint& delta);
QRect movedInsideImage(const QRect& cropperRect, const QRect& imageRect, const QPoint& delta);
QRect resizedRect(const QRect& cropperRect,
                  const QRect& imageRect,
                  ImageCropperHitTest::Position position,
                  const QPoint& currentPoint,
                  const QPoint& delta,
                  const QSize& minimumSize,
                  CropperShape shape);

} // namespace ImageCropperInteractionState

#endif // IMAGECROPPERINTERACTIONSTATE_H
