#include "imagecropperlabel.h"

#include <QPainter>
#include <QPainterPath>

void ImageCropperLabel::paintEvent(QPaintEvent* event)
{
    // Draw original image
    QLabel::paintEvent(event);

    // Draw cropper and set some effects
    switch (cropperShape)
    {
        case CropperShape::UNDEFINED:
            break;
        case CropperShape::FIXED_RECT:
            drawRectOpacity();
            break;
        case CropperShape::FIXED_ELLIPSE:
            drawEllipseOpacity();
            break;
        case CropperShape::RECT:
            drawRectOpacity();
            drawSquareEdge(!ONLY_FOUR_CORNERS);
            break;
        case CropperShape::SQUARE:
            drawRectOpacity();
            drawSquareEdge(ONLY_FOUR_CORNERS);
            break;
        case CropperShape::ELLIPSE:
            drawEllipseOpacity();
            drawSquareEdge(!ONLY_FOUR_CORNERS);
            break;
        case CropperShape::CIRCLE:
            drawEllipseOpacity();
            drawSquareEdge(ONLY_FOUR_CORNERS);
            break;
    }

    // Draw cropper rect
    if (isShowRectBorder)
    {
        QPainter painter(this);
        painter.setPen(borderPen);
        painter.drawRect(cropperRect);
    }
}

void ImageCropperLabel::drawSquareEdge(bool onlyFourCorners)
{
    if (!isShowDragSquare)
        return;

    // Four corners
    drawFillRect(cropperRect.topLeft(), dragSquareEdge, dragSquareColor);
    drawFillRect(cropperRect.topRight(), dragSquareEdge, dragSquareColor);
    drawFillRect(cropperRect.bottomLeft(), dragSquareEdge, dragSquareColor);
    drawFillRect(cropperRect.bottomRight(), dragSquareEdge, dragSquareColor);

    // Four edges
    if (!onlyFourCorners)
    {
        int centralX = cropperRect.left() + cropperRect.width() / 2;
        int centralY = cropperRect.top() + cropperRect.height() / 2;
        drawFillRect(QPoint(cropperRect.left(), centralY), dragSquareEdge, dragSquareColor);
        drawFillRect(QPoint(centralX, cropperRect.top()), dragSquareEdge, dragSquareColor);
        drawFillRect(QPoint(cropperRect.right(), centralY), dragSquareEdge, dragSquareColor);
        drawFillRect(QPoint(centralX, cropperRect.bottom()), dragSquareEdge, dragSquareColor);
    }
}

void ImageCropperLabel::drawFillRect(QPoint centralPoint, int edge, QColor color)
{
    QRect rect(centralPoint.x() - edge / 2, centralPoint.y() - edge / 2, edge, edge);
    QPainter painter(this);
    painter.fillRect(rect, color);
}

// Opacity effect
void ImageCropperLabel::drawOpacity(const QPainterPath& path)
{
    QPainter painterOpac(this);
    painterOpac.setOpacity(opacity);
    painterOpac.fillPath(path, QBrush(Qt::black));
}

void ImageCropperLabel::drawRectOpacity()
{
    if (isShowOpacityEffect)
    {
        QPainterPath p1, p2, p;
        p1.addRect(imageRect);
        p2.addRect(cropperRect);
        p = p1.subtracted(p2);
        drawOpacity(p);
    }
}

void ImageCropperLabel::drawEllipseOpacity()
{
    if (isShowOpacityEffect)
    {
        QPainterPath p1, p2, p;
        p1.addRect(imageRect);
        p2.addEllipse(cropperRect);
        p = p1.subtracted(p2);
        drawOpacity(p);
    }
}
