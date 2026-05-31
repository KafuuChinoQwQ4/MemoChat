#ifndef IMAGECROPPERTYPES_H
#define IMAGECROPPERTYPES_H

enum class CropperShape
{
    UNDEFINED = 0,
    RECT = 1,
    SQUARE = 2,
    FIXED_RECT = 3,
    ELLIPSE = 4,
    CIRCLE = 5,
    FIXED_ELLIPSE = 6
};

enum class OutputShape
{
    RECT = 0,
    ELLIPSE = 1
};

enum class SizeType
{
    fixedSize = 0,
    fitToMaxWidth = 1,
    fitToMaxHeight = 2,
    fitToMaxWidthHeight = 3,
};

#endif // IMAGECROPPERTYPES_H
