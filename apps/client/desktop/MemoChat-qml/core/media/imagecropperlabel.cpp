#include "imagecropperlabel.h"

#include "ImageCropperGeometry.h"

ImageCropperLabel::ImageCropperLabel(int width, int height, QWidget* parent)
    : QLabel(parent)
{
    this->setFixedSize(width, height);
    this->setAlignment(Qt::AlignCenter);
    this->setMouseTracking(true);
    this->setFocusPolicy(Qt::StrongFocus);

    borderPen.setWidth(1);
    borderPen.setColor(Qt::white);
    borderPen.setDashPattern(QVector<qreal>() << 3 << 3 << 3 << 3);
}

void ImageCropperLabel::setOriginalImage(const QPixmap& pixmap)
{
    originalImage = pixmap;

    const auto fit = ImageCropperGeometry::fitImageIntoLabel(pixmap.size(), size());
    scaledRate = fit.scaledRate;
    imageRect = fit.imageRect;

    tempImage = originalImage.scaled(fit.scaledSize.width(),
                                     fit.scaledSize.height(),
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
    this->setPixmap(tempImage);

    if (cropperShape >= CropperShape::FIXED_RECT)
    {
        cropperRect.setWidth(int(cropperRect_.width() * scaledRate));
        cropperRect.setHeight(int(cropperRect_.height() * scaledRate));
    }
    resetCropperPos();
}

/*****************************************
 * set cropper's shape (and size)
 *****************************************/
void ImageCropperLabel::setRectCropper()
{
    cropperShape = CropperShape::RECT;
    resetCropperPos();
}

void ImageCropperLabel::setSquareCropper()
{
    cropperShape = CropperShape::SQUARE;
    resetCropperPos();
}

void ImageCropperLabel::setEllipseCropper()
{
    cropperShape = CropperShape::ELLIPSE;
    resetCropperPos();
}

void ImageCropperLabel::setCircleCropper()
{
    cropperShape = CropperShape::CIRCLE;
    resetCropperPos();
}

void ImageCropperLabel::setFixedRectCropper(QSize size)
{
    cropperShape = CropperShape::FIXED_RECT;
    cropperRect_.setSize(size);
    resetCropperPos();
}

void ImageCropperLabel::setFixedEllipseCropper(QSize size)
{
    cropperShape = CropperShape::FIXED_ELLIPSE;
    cropperRect_.setSize(size);
    resetCropperPos();
}

// not recommended
void ImageCropperLabel::setCropper(CropperShape shape, QSize size)
{
    cropperShape = shape;
    cropperRect_.setSize(size);
    resetCropperPos();
}

/*****************************************************************************
 * Set cropper's fixed size
 *****************************************************************************/
void ImageCropperLabel::setCropperFixedSize(int fixedWidth, int fixedHeight)
{
    cropperRect_.setSize(QSize(fixedWidth, fixedHeight));
    resetCropperPos();
}

void ImageCropperLabel::setCropperFixedWidth(int fixedWidth)
{
    cropperRect_.setWidth(fixedWidth);
    resetCropperPos();
}

void ImageCropperLabel::setCropperFixedHeight(int fixedHeight)
{
    cropperRect_.setHeight(fixedHeight);
    resetCropperPos();
}

/**********************************************
 * Move cropper to the center of the image
 * And resize to default
 **********************************************/
void ImageCropperLabel::resetCropperPos()
{
    if (cropperShape == CropperShape::FIXED_RECT || cropperShape == CropperShape::FIXED_ELLIPSE)
    {
        cropperRect.setWidth(int(cropperRect_.width() * scaledRate));
        cropperRect.setHeight(int(cropperRect_.height() * scaledRate));
    }

    switch (cropperShape)
    {
        case CropperShape::UNDEFINED:
            break;
        case CropperShape::FIXED_RECT:
        case CropperShape::FIXED_ELLIPSE:
        {
            cropperRect = ImageCropperGeometry::centeredRect(size(), cropperRect.size());
            break;
        }
        case CropperShape::RECT:
        case CropperShape::SQUARE:
        case CropperShape::ELLIPSE:
        case CropperShape::CIRCLE:
        {
            cropperRect = ImageCropperGeometry::defaultVariableCropperRect(size(), tempImage.size());
            break;
        }
    }
}

QPixmap ImageCropperLabel::getCroppedImage()
{
    return getCroppedImage(this->outputShape);
}

QPixmap ImageCropperLabel::getCroppedImage(OutputShape shape)
{
    const QRect sourceRect = ImageCropperGeometry::sourceRectForCropper(cropperRect, imageRect, scaledRate);

    QPixmap resultImage(sourceRect.width(), sourceRect.height());
    resultImage = originalImage.copy(sourceRect);

    // Set ellipse mask (cut to ellipse shape)
    if (shape == OutputShape::ELLIPSE)
    {
        resultImage.setMask(ImageCropperGeometry::ellipseMask(sourceRect.size()));
    }

    return resultImage;
}
