#include "ImageCropperCursorPolicy.h"

namespace
{

bool isFreeAspectShape(CropperShape shape)
{
    return shape == CropperShape::RECT || shape == CropperShape::ELLIPSE;
}

bool isCornerResizableShape(CropperShape shape)
{
    return shape == CropperShape::SQUARE || shape == CropperShape::CIRCLE || isFreeAspectShape(shape);
}

} // namespace

namespace ImageCropperCursorPolicy
{

Qt::CursorShape cursorForPosition(ImageCropperHitTest::Position position, CropperShape shape)
{
    switch (position)
    {
        case ImageCropperHitTest::Position::BottomRight:
        case ImageCropperHitTest::Position::TopLeft:
            return isCornerResizableShape(shape) ? Qt::SizeFDiagCursor : Qt::ArrowCursor;
        case ImageCropperHitTest::Position::BottomLeft:
        case ImageCropperHitTest::Position::TopRight:
            return isCornerResizableShape(shape) ? Qt::SizeBDiagCursor : Qt::ArrowCursor;
        case ImageCropperHitTest::Position::Right:
        case ImageCropperHitTest::Position::Left:
            return isFreeAspectShape(shape) ? Qt::SizeHorCursor : Qt::ArrowCursor;
        case ImageCropperHitTest::Position::Bottom:
        case ImageCropperHitTest::Position::Top:
            return isFreeAspectShape(shape) ? Qt::SizeVerCursor : Qt::ArrowCursor;
        case ImageCropperHitTest::Position::Inside:
            return Qt::SizeAllCursor;
        case ImageCropperHitTest::Position::Outside:
            return Qt::ArrowCursor;
    }
    return Qt::ArrowCursor;
}

} // namespace ImageCropperCursorPolicy
