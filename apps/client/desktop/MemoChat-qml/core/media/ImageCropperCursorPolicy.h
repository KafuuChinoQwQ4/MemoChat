#ifndef IMAGECROPPERCURSORPOLICY_H
#define IMAGECROPPERCURSORPOLICY_H

#include "ImageCropperHitTest.h"
#include "ImageCropperTypes.h"

#include <Qt>

namespace ImageCropperCursorPolicy
{

Qt::CursorShape cursorForPosition(ImageCropperHitTest::Position position, CropperShape shape);

} // namespace ImageCropperCursorPolicy

#endif // IMAGECROPPERCURSORPOLICY_H
