#ifndef IMAGECROPPERHITTEST_H
#define IMAGECROPPERHITTEST_H

#include <QPoint>
#include <QRect>

namespace ImageCropperHitTest
{

enum class Position
{
    Outside = 0,
    Inside = 1,
    TopLeft = 2,
    Top = 3,
    TopRight = 4,
    Right = 5,
    BottomRight = 6,
    Bottom = 7,
    BottomLeft = 8,
    Left = 9
};

bool isNearDragSquare(const QPoint& point, const QPoint& handlePoint, int dragSquareEdge);
Position positionInRect(const QRect& cropperRect, const QPoint& point, int dragSquareEdge);

} // namespace ImageCropperHitTest

#endif // IMAGECROPPERHITTEST_H
