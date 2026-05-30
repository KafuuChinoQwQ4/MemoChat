#include "imagecropperlabel.h"

#include "ImageCropperGeometry.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

bool ImageCropperLabel::isPosNearDragSquare(const QPoint& pt1, const QPoint& pt2)
{
    return abs(pt1.x() - pt2.x()) * 2 <= dragSquareEdge && abs(pt1.y() - pt2.y()) * 2 <= dragSquareEdge;
}

int ImageCropperLabel::getPosInCropperRect(const QPoint& pt)
{
    if (isPosNearDragSquare(pt, QPoint(cropperRect.right(), cropperRect.center().y())))
        return RECT_RIGHT;
    if (isPosNearDragSquare(pt, cropperRect.bottomRight()))
        return RECT_BOTTOM_RIGHT;
    if (isPosNearDragSquare(pt, QPoint(cropperRect.center().x(), cropperRect.bottom())))
        return RECT_BOTTOM;
    if (isPosNearDragSquare(pt, cropperRect.bottomLeft()))
        return RECT_BOTTOM_LEFT;
    if (isPosNearDragSquare(pt, QPoint(cropperRect.left(), cropperRect.center().y())))
        return RECT_LEFT;
    if (isPosNearDragSquare(pt, cropperRect.topLeft()))
        return RECT_TOP_LEFT;
    if (isPosNearDragSquare(pt, QPoint(cropperRect.center().x(), cropperRect.top())))
        return RECT_TOP;
    if (isPosNearDragSquare(pt, cropperRect.topRight()))
        return RECT_TOP_RIGHT;
    if (cropperRect.contains(pt, true))
        return RECT_INSIDE;
    return RECT_OUTSIZD;
}

/*************************************************
 *
 *  Change mouse cursor type
 *      Arrow, SizeHor, SizeVer, etc...
 *
 *************************************************/

void ImageCropperLabel::changeCursor()
{
    switch (cursorPosInCropperRect)
    {
        case RECT_OUTSIZD:
            setCursor(Qt::ArrowCursor);
            break;
        case RECT_BOTTOM_RIGHT:
        {
            switch (cropperShape)
            {
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    setCursor(Qt::SizeFDiagCursor);
                    break;
                default:
                    break;
            }
            break;
        }
        case RECT_RIGHT:
        {
            switch (cropperShape)
            {
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    setCursor(Qt::SizeHorCursor);
                    break;
                default:
                    break;
            }
            break;
        }
        case RECT_BOTTOM:
        {
            switch (cropperShape)
            {
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    setCursor(Qt::SizeVerCursor);
                    break;
                default:
                    break;
            }
            break;
        }
        case RECT_BOTTOM_LEFT:
        {
            switch (cropperShape)
            {
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    setCursor(Qt::SizeBDiagCursor);
                    break;
                default:
                    break;
            }
            break;
        }
        case RECT_LEFT:
        {
            switch (cropperShape)
            {
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    setCursor(Qt::SizeHorCursor);
                    break;
                default:
                    break;
            }
            break;
        }
        case RECT_TOP_LEFT:
        {
            switch (cropperShape)
            {
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    setCursor(Qt::SizeFDiagCursor);
                    break;
                default:
                    break;
            }
            break;
        }
        case RECT_TOP:
        {
            switch (cropperShape)
            {
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    setCursor(Qt::SizeVerCursor);
                    break;
                default:
                    break;
            }
            break;
        }
        case RECT_TOP_RIGHT:
        {
            switch (cropperShape)
            {
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    setCursor(Qt::SizeBDiagCursor);
                    break;
                default:
                    break;
            }
            break;
        }
        case RECT_INSIDE:
        {
            setCursor(Qt::SizeAllCursor);
            break;
        }
    }
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

    int xOffset = currPos.x() - lastPos.x();
    int yOffset = currPos.y() - lastPos.y();
    lastPos = currPos;

    int disX = 0;
    int disY = 0;

    // Move cropper
    switch (cursorPosInCropperRect)
    {
        case RECT_OUTSIZD:
            break;
        case RECT_BOTTOM_RIGHT:
        {
            disX = currPos.x() - cropperRect.left();
            disY = currPos.y() - cropperRect.top();
            switch (cropperShape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                    break;
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    setCursor(Qt::SizeFDiagCursor);
                    if (disX >= cropperMinimumWidth && disY >= cropperMinimumHeight)
                    {
                        if (disX > disY && cropperRect.top() + disX <= imageRect.bottom())
                        {
                            cropperRect.setRight(currPos.x());
                            cropperRect.setBottom(cropperRect.top() + disX);
                            emit croppedImageChanged();
                        }
                        else if (disX <= disY && cropperRect.left() + disY <= imageRect.right())
                        {
                            cropperRect.setBottom(currPos.y());
                            cropperRect.setRight(cropperRect.left() + disY);
                            emit croppedImageChanged();
                        }
                    }
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    setCursor(Qt::SizeFDiagCursor);
                    if (disX >= cropperMinimumWidth)
                    {
                        cropperRect.setRight(currPos.x());
                        emit croppedImageChanged();
                    }
                    if (disY >= cropperMinimumHeight)
                    {
                        cropperRect.setBottom(currPos.y());
                        emit croppedImageChanged();
                    }
                    break;
            }
            break;
        }
        case RECT_RIGHT:
        {
            disX = currPos.x() - cropperRect.left();
            switch (cropperShape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= cropperMinimumWidth)
                    {
                        cropperRect.setRight(currPos.x());
                        emit croppedImageChanged();
                    }
                    break;
            }
            break;
        }
        case RECT_BOTTOM:
        {
            disY = currPos.y() - cropperRect.top();
            switch (cropperShape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disY >= cropperMinimumHeight)
                    {
                        cropperRect.setBottom(cropperRect.bottom() + yOffset);
                        emit croppedImageChanged();
                    }
                    break;
            }
            break;
        }
        case RECT_BOTTOM_LEFT:
        {
            disX = cropperRect.right() - currPos.x();
            disY = currPos.y() - cropperRect.top();
            switch (cropperShape)
            {
                case CropperShape::UNDEFINED:
                    break;
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= cropperMinimumWidth)
                    {
                        cropperRect.setLeft(currPos.x());
                        emit croppedImageChanged();
                    }
                    if (disY >= cropperMinimumHeight)
                    {
                        cropperRect.setBottom(currPos.y());
                        emit croppedImageChanged();
                    }
                    break;
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    if (disX >= cropperMinimumWidth && disY >= cropperMinimumHeight)
                    {
                        if (disX > disY && cropperRect.top() + disX <= imageRect.bottom())
                        {
                            cropperRect.setLeft(currPos.x());
                            cropperRect.setBottom(cropperRect.top() + disX);
                            emit croppedImageChanged();
                        }
                        else if (disX <= disY && cropperRect.right() - disY >= imageRect.left())
                        {
                            cropperRect.setBottom(currPos.y());
                            cropperRect.setLeft(cropperRect.right() - disY);
                            emit croppedImageChanged();
                        }
                    }
                    break;
            }
            break;
        }
        case RECT_LEFT:
        {
            disX = cropperRect.right() - currPos.x();
            switch (cropperShape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= cropperMinimumHeight)
                    {
                        cropperRect.setLeft(cropperRect.left() + xOffset);
                        emit croppedImageChanged();
                    }
                    break;
            }
            break;
        }
        case RECT_TOP_LEFT:
        {
            disX = cropperRect.right() - currPos.x();
            disY = cropperRect.bottom() - currPos.y();
            switch (cropperShape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= cropperMinimumWidth)
                    {
                        cropperRect.setLeft(currPos.x());
                        emit croppedImageChanged();
                    }
                    if (disY >= cropperMinimumHeight)
                    {
                        cropperRect.setTop(currPos.y());
                        emit croppedImageChanged();
                    }
                    break;
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    if (disX >= cropperMinimumWidth && disY >= cropperMinimumHeight)
                    {
                        if (disX > disY && cropperRect.bottom() - disX >= imageRect.top())
                        {
                            cropperRect.setLeft(currPos.x());
                            cropperRect.setTop(cropperRect.bottom() - disX);
                            emit croppedImageChanged();
                        }
                        else if (disX <= disY && cropperRect.right() - disY >= imageRect.left())
                        {
                            cropperRect.setTop(currPos.y());
                            cropperRect.setLeft(cropperRect.right() - disY);
                            emit croppedImageChanged();
                        }
                    }
                    break;
            }
            break;
        }
        case RECT_TOP:
        {
            disY = cropperRect.bottom() - currPos.y();
            switch (cropperShape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disY >= cropperMinimumHeight)
                    {
                        cropperRect.setTop(cropperRect.top() + yOffset);
                        emit croppedImageChanged();
                    }
                    break;
            }
            break;
        }
        case RECT_TOP_RIGHT:
        {
            disX = currPos.x() - cropperRect.left();
            disY = cropperRect.bottom() - currPos.y();
            switch (cropperShape)
            {
                case CropperShape::UNDEFINED:
                case CropperShape::FIXED_RECT:
                case CropperShape::FIXED_ELLIPSE:
                    break;
                case CropperShape::RECT:
                case CropperShape::ELLIPSE:
                    if (disX >= cropperMinimumWidth)
                    {
                        cropperRect.setRight(currPos.x());
                        emit croppedImageChanged();
                    }
                    if (disY >= cropperMinimumHeight)
                    {
                        cropperRect.setTop(currPos.y());
                        emit croppedImageChanged();
                    }
                    break;
                case CropperShape::SQUARE:
                case CropperShape::CIRCLE:
                    if (disX >= cropperMinimumWidth && disY >= cropperMinimumHeight)
                    {
                        if (disX < disY && cropperRect.left() + disY <= imageRect.right())
                        {
                            cropperRect.setTop(currPos.y());
                            cropperRect.setRight(cropperRect.left() + disY);
                            emit croppedImageChanged();
                        }
                        else if (disX >= disY && cropperRect.bottom() - disX >= imageRect.top())
                        {
                            cropperRect.setRight(currPos.x());
                            cropperRect.setTop(cropperRect.bottom() - disX);
                            emit croppedImageChanged();
                        }
                    }
                    break;
            }
            break;
        }
        case RECT_INSIDE:
        {
            // Make sure the cropperRect is entirely inside the imageRecct
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
            cropperRect.moveTo(cropperRect.left() + xOffset, cropperRect.top() + yOffset);
            emit croppedImageChanged();
        }
        break;
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
