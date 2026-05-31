#include "ImageCropperGeometry.h"
#include "ImageCropperCursorPolicy.h"
#include "ImageCropperHitTest.h"
#include "ImageCropperInteractionState.h"

#include <QRect>
#include <QSize>
#include <gtest/gtest.h>

namespace
{

QRect resizedCropper(const QRect& cropperRect,
                     const QRect& imageRect,
                     ImageCropperHitTest::Position position,
                     const QPoint& currentPoint,
                     const QPoint& delta,
                     const QSize& minimumSize,
                     CropperShape shape)
{
    return ImageCropperInteractionState::resizedRect(cropperRect,
                                                     imageRect,
                                                     position,
                                                     currentPoint,
                                                     delta,
                                                     minimumSize,
                                                     shape);
}

} // namespace

TEST(ImageCropperGeometryTest, RectangleHitTestDetectsEachHandle)
{
    const QRect cropper(10, 20, 100, 80);
    constexpr int edge = 8;

    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, cropper.topLeft(), edge),
              ImageCropperHitTest::Position::TopLeft);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, QPoint(cropper.center().x(), cropper.top()), edge),
              ImageCropperHitTest::Position::Top);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, cropper.topRight(), edge),
              ImageCropperHitTest::Position::TopRight);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, QPoint(cropper.right(), cropper.center().y()), edge),
              ImageCropperHitTest::Position::Right);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, cropper.bottomRight(), edge),
              ImageCropperHitTest::Position::BottomRight);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, QPoint(cropper.center().x(), cropper.bottom()), edge),
              ImageCropperHitTest::Position::Bottom);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, cropper.bottomLeft(), edge),
              ImageCropperHitTest::Position::BottomLeft);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, QPoint(cropper.left(), cropper.center().y()), edge),
              ImageCropperHitTest::Position::Left);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, cropper.center(), edge),
              ImageCropperHitTest::Position::Inside);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, QPoint(0, 0), edge), ImageCropperHitTest::Position::Outside);
}

TEST(ImageCropperGeometryTest, CircleHitTestUsesSquareHandlePositions)
{
    const QRect cropper(30, 30, 90, 90);
    constexpr int edge = 10;

    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, cropper.bottomRight(), edge),
              ImageCropperHitTest::Position::BottomRight);
    EXPECT_EQ(ImageCropperHitTest::positionInRect(cropper, cropper.center(), edge),
              ImageCropperHitTest::Position::Inside);
}

TEST(ImageCropperGeometryTest, DragMovementClampsInsideImageBounds)
{
    const QRect imageRect(0, 0, 200, 160);
    const QRect cropper(20, 30, 80, 60);

    EXPECT_EQ(ImageCropperInteractionState::movedInsideImage(cropper, imageRect, QPoint(-50, 0)).left(), 20);
    EXPECT_EQ(ImageCropperInteractionState::movedInsideImage(cropper, imageRect, QPoint(200, 0)).right(), 99);
    EXPECT_EQ(ImageCropperInteractionState::movedInsideImage(cropper, imageRect, QPoint(0, -50)).top(), 30);
    EXPECT_EQ(ImageCropperInteractionState::movedInsideImage(cropper, imageRect, QPoint(0, 200)).bottom(), 89);
    EXPECT_EQ(ImageCropperInteractionState::movedInsideImage(cropper, imageRect, QPoint(10, 12)),
              QRect(30, 42, 80, 60));
}

TEST(ImageCropperGeometryTest, ResizeMovementPreservesMinimumCropSize)
{
    const QRect imageRect(0, 0, 200, 160);
    const QRect cropper(20, 20, 80, 60);
    const QSize minimum(32, 28);

    const QRect tooSmall = resizedCropper(cropper,
                                          imageRect,
                                          ImageCropperHitTest::Position::Right,
                                          QPoint(cropper.left() + 10, cropper.top()),
                                          QPoint(0, 0),
                                          minimum,
                                          CropperShape::RECT);
    EXPECT_EQ(tooSmall, cropper);

    const QRect resized = resizedCropper(cropper,
                                         imageRect,
                                         ImageCropperHitTest::Position::BottomRight,
                                         QPoint(140, 120),
                                         QPoint(0, 0),
                                         minimum,
                                         CropperShape::RECT);
    EXPECT_EQ(resized.right(), 140);
    EXPECT_EQ(resized.bottom(), 120);
    EXPECT_GE(resized.width(), minimum.width());
    EXPECT_GE(resized.height(), minimum.height());
}

TEST(ImageCropperGeometryTest, ResizeMovementKeepsCurrentSideHandleCompatibility)
{
    const QRect imageRect(0, 0, 200, 160);
    const QRect cropper(20, 20, 80, 60);
    const QSize minimum(70, 28);

    const QRect bottom = resizedCropper(cropper,
                                        imageRect,
                                        ImageCropperHitTest::Position::Bottom,
                                        QPoint(60, 120),
                                        QPoint(0, 5),
                                        minimum,
                                        CropperShape::RECT);
    EXPECT_EQ(bottom.bottom(), cropper.bottom() + 5);

    const QRect top = resizedCropper(cropper,
                                     imageRect,
                                     ImageCropperHitTest::Position::Top,
                                     QPoint(60, 10),
                                     QPoint(0, -4),
                                     minimum,
                                     CropperShape::ELLIPSE);
    EXPECT_EQ(top.top(), cropper.top() - 4);

    const QRect left = resizedCropper(cropper,
                                      imageRect,
                                      ImageCropperHitTest::Position::Left,
                                      QPoint(40, 60),
                                      QPoint(-6, 0),
                                      QSize(80, 20),
                                      CropperShape::RECT);
    EXPECT_EQ(left.left(), cropper.left() - 6);
}

TEST(ImageCropperGeometryTest, ResizeMovementIgnoresUnsupportedShapeHandles)
{
    const QRect imageRect(0, 0, 200, 160);
    const QRect cropper(20, 20, 80, 60);
    const QSize minimum(32, 28);

    EXPECT_EQ(resizedCropper(cropper,
                             imageRect,
                             ImageCropperHitTest::Position::BottomRight,
                             QPoint(140, 120),
                             QPoint(0, 0),
                             minimum,
                             CropperShape::FIXED_RECT),
              cropper);
    EXPECT_EQ(resizedCropper(cropper,
                             imageRect,
                             ImageCropperHitTest::Position::Right,
                             QPoint(140, 60),
                             QPoint(0, 0),
                             minimum,
                             CropperShape::SQUARE),
              cropper);
    EXPECT_EQ(resizedCropper(cropper,
                             imageRect,
                             ImageCropperHitTest::Position::Bottom,
                             QPoint(60, 120),
                             QPoint(0, 5),
                             minimum,
                             CropperShape::UNDEFINED),
              cropper);
}

TEST(ImageCropperGeometryTest, ResizeMovementPreservesSquareCornerBehavior)
{
    const QRect imageRect(0, 0, 200, 160);
    const QRect cropper(20, 20, 80, 80);
    const QSize minimum(32, 32);

    const QRect bottomRight = resizedCropper(cropper,
                                             imageRect,
                                             ImageCropperHitTest::Position::BottomRight,
                                             QPoint(120, 90),
                                             QPoint(0, 0),
                                             minimum,
                                             CropperShape::SQUARE);
    EXPECT_EQ(bottomRight.right(), 120);
    EXPECT_EQ(bottomRight.bottom(), 120);
    EXPECT_EQ(bottomRight.width(), bottomRight.height());

    const QRect topLeft = resizedCropper(cropper,
                                         imageRect,
                                         ImageCropperHitTest::Position::TopLeft,
                                         QPoint(0, 10),
                                         QPoint(0, 0),
                                         minimum,
                                         CropperShape::CIRCLE);
    EXPECT_EQ(topLeft.left(), 0);
    EXPECT_EQ(topLeft.top(), 0);
    EXPECT_EQ(topLeft.width(), topLeft.height());
}

TEST(ImageCropperGeometryTest, CursorPolicyMatchesHandleAndShape)
{
    EXPECT_EQ(
        ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::BottomRight, CropperShape::SQUARE),
        Qt::SizeFDiagCursor);
    EXPECT_EQ(
        ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::BottomRight, CropperShape::CIRCLE),
        Qt::SizeFDiagCursor);
    EXPECT_EQ(
        ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::BottomRight, CropperShape::RECT),
        Qt::SizeFDiagCursor);
    EXPECT_EQ(
        ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::BottomRight, CropperShape::ELLIPSE),
        Qt::SizeFDiagCursor);

    EXPECT_EQ(ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::Right, CropperShape::RECT),
              Qt::SizeHorCursor);
    EXPECT_EQ(ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::Bottom, CropperShape::ELLIPSE),
              Qt::SizeVerCursor);
    EXPECT_EQ(ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::Right, CropperShape::SQUARE),
              Qt::ArrowCursor);
    EXPECT_EQ(ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::Bottom, CropperShape::CIRCLE),
              Qt::ArrowCursor);
    EXPECT_EQ(ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::Inside, CropperShape::RECT),
              Qt::SizeAllCursor);
    EXPECT_EQ(ImageCropperCursorPolicy::cursorForPosition(ImageCropperHitTest::Position::Outside, CropperShape::RECT),
              Qt::ArrowCursor);
}
