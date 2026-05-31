#include "ImageCropperInteractionState.h"

namespace ImageCropperInteractionState
{

QPoint clampedMoveDelta(const QRect& cropperRect, const QRect& imageRect, const QPoint& delta)
{
    int xOffset = delta.x();
    int yOffset = delta.y();
    if (xOffset > 0)
    {
        if (cropperRect.right() + xOffset > imageRect.right())
            xOffset = 0;
    }
    else if (xOffset < 0)
    {
        if (cropperRect.left() + xOffset < imageRect.left())
            xOffset = 0;
    }
    if (yOffset > 0)
    {
        if (cropperRect.bottom() + yOffset > imageRect.bottom())
            yOffset = 0;
    }
    else if (yOffset < 0)
    {
        if (cropperRect.top() + yOffset < imageRect.top())
            yOffset = 0;
    }
    return QPoint(xOffset, yOffset);
}

QRect movedInsideImage(const QRect& cropperRect, const QRect& imageRect, const QPoint& delta)
{
    const QPoint boundedDelta = clampedMoveDelta(cropperRect, imageRect, delta);
    QRect next = cropperRect;
    next.moveTo(next.left() + boundedDelta.x(), next.top() + boundedDelta.y());
    return next;
}

QRect resizedRect(const QRect& cropperRect,
                  const QRect& imageRect,
                  ImageCropperHitTest::Position position,
                  const QPoint& currentPoint,
                  const QPoint& delta,
                  const QSize& minimumSize,
                  CropperShape shape)
{
    QRect next = cropperRect;

    switch (position)
    {
        case ImageCropperHitTest::Position::BottomRight:
        {
            const int disX = currentPoint.x() - cropperRect.left();
            const int disY = currentPoint.y() - cropperRect.top();
            switch (shape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                    break;
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    if (disX >= minimumSize.width() && disY >= minimumSize.height())
                    {
                        if (disX > disY && cropperRect.top() + disX <= imageRect.bottom())
                        {
                            next.setRight(currentPoint.x());
                            next.setBottom(cropperRect.top() + disX);
                        }
                        else if (disX <= disY && cropperRect.left() + disY <= imageRect.right())
                        {
                            next.setBottom(currentPoint.y());
                            next.setRight(cropperRect.left() + disY);
                        }
                    }
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= minimumSize.width())
                        next.setRight(currentPoint.x());
                    if (disY >= minimumSize.height())
                        next.setBottom(currentPoint.y());
                    break;
            }
            break;
        }
        case ImageCropperHitTest::Position::Right:
        {
            const int disX = currentPoint.x() - cropperRect.left();
            switch (shape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= minimumSize.width())
                        next.setRight(currentPoint.x());
                    break;
            }
            break;
        }
        case ImageCropperHitTest::Position::Bottom:
        {
            const int disY = currentPoint.y() - cropperRect.top();
            switch (shape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disY >= minimumSize.height())
                        next.setBottom(cropperRect.bottom() + delta.y());
                    break;
            }
            break;
        }
        case ImageCropperHitTest::Position::BottomLeft:
        {
            const int disX = cropperRect.right() - currentPoint.x();
            const int disY = currentPoint.y() - cropperRect.top();
            switch (shape)
            {
                case CropperShape::UNDEFINED:
                    break;
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= minimumSize.width())
                        next.setLeft(currentPoint.x());
                    if (disY >= minimumSize.height())
                        next.setBottom(currentPoint.y());
                    break;
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    if (disX >= minimumSize.width() && disY >= minimumSize.height())
                    {
                        if (disX > disY && cropperRect.top() + disX <= imageRect.bottom())
                        {
                            next.setLeft(currentPoint.x());
                            next.setBottom(cropperRect.top() + disX);
                        }
                        else if (disX <= disY && cropperRect.right() - disY >= imageRect.left())
                        {
                            next.setBottom(currentPoint.y());
                            next.setLeft(cropperRect.right() - disY);
                        }
                    }
                    break;
            }
            break;
        }
        case ImageCropperHitTest::Position::Left:
        {
            const int disX = cropperRect.right() - currentPoint.x();
            switch (shape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= minimumSize.height())
                        next.setLeft(cropperRect.left() + delta.x());
                    break;
            }
            break;
        }
        case ImageCropperHitTest::Position::TopLeft:
        {
            const int disX = cropperRect.right() - currentPoint.x();
            const int disY = cropperRect.bottom() - currentPoint.y();
            switch (shape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= minimumSize.width())
                        next.setLeft(currentPoint.x());
                    if (disY >= minimumSize.height())
                        next.setTop(currentPoint.y());
                    break;
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    if (disX >= minimumSize.width() && disY >= minimumSize.height())
                    {
                        if (disX > disY && cropperRect.bottom() - disX >= imageRect.top())
                        {
                            next.setLeft(currentPoint.x());
                            next.setTop(cropperRect.bottom() - disX);
                        }
                        else if (disX <= disY && cropperRect.right() - disY >= imageRect.left())
                        {
                            next.setTop(currentPoint.y());
                            next.setLeft(cropperRect.right() - disY);
                        }
                    }
                    break;
            }
            break;
        }
        case ImageCropperHitTest::Position::Top:
        {
            const int disY = cropperRect.bottom() - currentPoint.y();
            switch (shape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disY >= minimumSize.height())
                        next.setTop(cropperRect.top() + delta.y());
                    break;
            }
            break;
        }
        case ImageCropperHitTest::Position::TopRight:
        {
            const int disX = currentPoint.x() - cropperRect.left();
            const int disY = cropperRect.bottom() - currentPoint.y();
            switch (shape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= minimumSize.width())
                        next.setRight(currentPoint.x());
                    if (disY >= minimumSize.height())
                        next.setTop(currentPoint.y());
                    break;
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    if (disX >= minimumSize.width() && disY >= minimumSize.height())
                    {
                        if (disX < disY && cropperRect.left() + disY <= imageRect.right())
                        {
                            next.setTop(currentPoint.y());
                            next.setRight(cropperRect.left() + disY);
                        }
                        else if (disX >= disY && cropperRect.bottom() - disX >= imageRect.top())
                        {
                            next.setRight(currentPoint.x());
                            next.setTop(cropperRect.bottom() - disX);
                        }
                    }
                    break;
            }
            break;
        }
        case ImageCropperHitTest::Position::Inside:
        case ImageCropperHitTest::Position::Outside:
            break;
    }

    return next;
}

} // namespace ImageCropperInteractionState
