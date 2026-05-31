#include "imagecropperlabel.h"

#include "ImageCropperCursorPolicy.h"
#include "ImageCropperGeometry.h"
#include "ImageCropperHitTest.h"
#include "ImageCropperInteractionState.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

bool ImageCropperLabel::isPosNearDragSquare(const QPoint& pt1, const QPoint& pt2)
{
    return ImageCropperHitTest::isNearDragSquare(pt1, pt2, dragSquareEdge);
}

int ImageCropperLabel::getPosInCropperRect(const QPoint& pt)
{
    return static_cast<int>(ImageCropperHitTest::positionInRect(cropperRect, pt, dragSquareEdge));
}

/*************************************************
 *
 *  Change mouse cursor type
 *      Arrow, SizeHor, SizeVer, etc...
 *
 *************************************************/

void ImageCropperLabel::changeCursor()
{
    setCursor(
        ImageCropperCursorPolicy::cursorForPosition(static_cast<ImageCropperHitTest::Position>(cursorPosInCropperRect),
                                                    cropperShape));
}

/*****************************************************
 *
 *  Mouse Events
 *
 *****************************************************/

void ImageCropperLabel::mousePressEvent(QMouseEvent* e)
{
    currPos = lastPos = e->pos();
    isLButtonPressed = true;
}

void ImageCropperLabel::mouseMoveEvent(QMouseEvent* e)
{
    currPos = e->pos();
    if (!isCursorPosCalculated)
    {
        cursorPosInCropperRect = getPosInCropperRect(currPos);
        changeCursor();
    }

    if (!isLButtonPressed)
        return;
    if (!imageRect.contains(currPos))
        return;

    isCursorPosCalculated = true;

    const int xOffset = currPos.x() - lastPos.x();
    const int yOffset = currPos.y() - lastPos.y();
    lastPos = currPos;

    const auto position = static_cast<ImageCropperHitTest::Position>(cursorPosInCropperRect);
    if (position == ImageCropperHitTest::Position::Inside)
    {
        const QRect nextRect =
            ImageCropperInteractionState::movedInsideImage(cropperRect, imageRect, QPoint(xOffset, yOffset));
        if (nextRect != cropperRect)
        {
            cropperRect = nextRect;
            emit croppedImageChanged();
        }
    }
    else
    {
        const QRect nextRect =
            ImageCropperInteractionState::resizedRect(cropperRect,
                                                      imageRect,
                                                      position,
                                                      currPos,
                                                      QPoint(xOffset, yOffset),
                                                      QSize(cropperMinimumWidth, cropperMinimumHeight),
                                                      cropperShape);
        if (nextRect != cropperRect)
        {
            cropperRect = nextRect;
            emit croppedImageChanged();
        }
    }

    repaint();
}

void ImageCropperLabel::mouseReleaseEvent(QMouseEvent*)
{
    isLButtonPressed = false;
    isCursorPosCalculated = false;
    setCursor(Qt::ArrowCursor);
}

void ImageCropperLabel::wheelEvent(QWheelEvent* event)
{
    if (cropperShape == CropperShape::UNDEFINED)
        return;

    const int delta = event->angleDelta().y();
    if (delta == 0)
        return;

    const QRect nextRect = ImageCropperGeometry::wheelZoomRect(cropperRect,
                                                               imageRect,
                                                               QSize(cropperMinimumWidth, cropperMinimumHeight),
                                                               delta > 0 ? 1.10 : 0.90,
                                                               cropperShape == CropperShape::SQUARE ||
                                                                   cropperShape == CropperShape::CIRCLE);
    if (nextRect.isNull())
        return;

    cropperRect = nextRect;
    emit croppedImageChanged();
    repaint();
}

void ImageCropperLabel::keyPressEvent(QKeyEvent* event)
{
    const int step = 1;
    switch (event->key())
    {
        case Qt::Key_Left:
            cropperRect.moveLeft(cropperRect.left() - step);
            emit croppedImageChanged();
            break;
        case Qt::Key_Right:
            cropperRect.moveLeft(cropperRect.left() + step);
            emit croppedImageChanged();
            break;
        case Qt::Key_Up:
            cropperRect.moveTop(cropperRect.top() - step);
            emit croppedImageChanged();
            break;
        case Qt::Key_Down:
            cropperRect.moveTop(cropperRect.top() + step);
            emit croppedImageChanged();
            break;
        default:
            QLabel::keyPressEvent(event);
            return;
    }
    repaint();
    event->accept();
}
