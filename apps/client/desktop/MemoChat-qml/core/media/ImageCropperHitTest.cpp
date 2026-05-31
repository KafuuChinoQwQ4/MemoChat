#include "ImageCropperHitTest.h"

#include <cstdlib>

namespace ImageCropperHitTest
{

bool isNearDragSquare(const QPoint& point, const QPoint& handlePoint, int dragSquareEdge)
{
    return std::abs(point.x() - handlePoint.x()) * 2 <= dragSquareEdge &&
           std::abs(point.y() - handlePoint.y()) * 2 <= dragSquareEdge;
}

Position positionInRect(const QRect& cropperRect, const QPoint& point, int dragSquareEdge)
{
    if (isNearDragSquare(point, QPoint(cropperRect.right(), cropperRect.center().y()), dragSquareEdge))
        return Position::Right;
    if (isNearDragSquare(point, cropperRect.bottomRight(), dragSquareEdge))
        return Position::BottomRight;
    if (isNearDragSquare(point, QPoint(cropperRect.center().x(), cropperRect.bottom()), dragSquareEdge))
        return Position::Bottom;
    if (isNearDragSquare(point, cropperRect.bottomLeft(), dragSquareEdge))
        return Position::BottomLeft;
    if (isNearDragSquare(point, QPoint(cropperRect.left(), cropperRect.center().y()), dragSquareEdge))
        return Position::Left;
    if (isNearDragSquare(point, cropperRect.topLeft(), dragSquareEdge))
        return Position::TopLeft;
    if (isNearDragSquare(point, QPoint(cropperRect.center().x(), cropperRect.top()), dragSquareEdge))
        return Position::Top;
    if (isNearDragSquare(point, cropperRect.topRight(), dragSquareEdge))
        return Position::TopRight;
    if (cropperRect.contains(point, true))
        return Position::Inside;
    return Position::Outside;
}

} // namespace ImageCropperHitTest
