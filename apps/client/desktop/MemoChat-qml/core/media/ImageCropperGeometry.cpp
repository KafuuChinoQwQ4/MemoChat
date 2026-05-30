#include "ImageCropperGeometry.h"

#include <QPainter>

namespace ImageCropperGeometry
{

ImageFit fitImageIntoLabel(const QSize& imageSize, const QSize& labelSize)
{
    const int imgWidth = imageSize.width();
    const int imgHeight = imageSize.height();
    const int labelWidth = labelSize.width();
    const int labelHeight = labelSize.height();

    ImageFit fit;
    if (imgWidth * labelHeight < imgHeight * labelWidth)
    {
        fit.scaledRate = labelHeight / double(imgHeight);
        const int imgHeightInLabel = labelHeight;
        const int imgWidthInLabel = int(fit.scaledRate * imgWidth);
        fit.scaledSize = QSize(imgWidthInLabel, imgHeightInLabel);
        fit.imageRect.setRect((labelWidth - imgWidthInLabel) / 2, 0, imgWidthInLabel, imgHeightInLabel);
    }
    else
    {
        fit.scaledRate = labelWidth / double(imgWidth);
        const int imgWidthInLabel = labelWidth;
        const int imgHeightInLabel = int(fit.scaledRate * imgHeight);
        fit.scaledSize = QSize(imgWidthInLabel, imgHeightInLabel);
        fit.imageRect.setRect(0, (labelHeight - imgHeightInLabel) / 2, imgWidthInLabel, imgHeightInLabel);
    }
    return fit;
}

QRect sourceRectForCropper(const QRect& cropperRect, const QRect& imageRect, double scaledRate)
{
    return QRect(int((cropperRect.left() - imageRect.left()) / scaledRate),
                 int((cropperRect.top() - imageRect.top()) / scaledRate),
                 int(cropperRect.width() / scaledRate),
                 int(cropperRect.height() / scaledRate));
}

QRect centeredRect(const QSize& containerSize, const QSize& rectSize)
{
    return QRect((containerSize.width() - rectSize.width()) / 2,
                 (containerSize.height() - rectSize.height()) / 2,
                 rectSize.width(),
                 rectSize.height());
}

QRect defaultVariableCropperRect(const QSize& labelSize, const QSize& imageSize)
{
    const int edge = int((imageSize.width() > imageSize.height() ? imageSize.height() : imageSize.width()) * 3 / 4.0);
    return centeredRect(labelSize, QSize(edge, edge));
}

QRect wheelZoomRect(const QRect& cropperRect,
                    const QRect& imageRect,
                    const QSize& minimumSize,
                    qreal factor,
                    bool squareAspect)
{
    const int centerX = cropperRect.center().x();
    const int centerY = cropperRect.center().y();
    int newW = int(cropperRect.width() * factor);
    int newH = int(cropperRect.height() * factor);

    if (newW < minimumSize.width())
        newW = minimumSize.width();
    if (newH < minimumSize.height())
        newH = minimumSize.height();

    if (centerX - newW / 2 < imageRect.left())
        newW = (centerX - imageRect.left()) * 2;
    if (centerX + newW / 2 > imageRect.right())
        newW = (imageRect.right() - centerX) * 2;
    if (centerY - newH / 2 < imageRect.top())
        newH = (centerY - imageRect.top()) * 2;
    if (centerY + newH / 2 > imageRect.bottom())
        newH = (imageRect.bottom() - centerY) * 2;

    if (squareAspect)
        newH = newW;

    if (newW < minimumSize.width() || newH < minimumSize.height())
        return QRect();

    return QRect(centerX - newW / 2, centerY - newH / 2, newW, newH);
}

QBitmap ellipseMask(const QSize& size)
{
    QBitmap mask(size);
    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(0, 0, size.width(), size.height(), Qt::white);
    painter.setBrush(QColor(0, 0, 0));
    painter.drawRoundedRect(0, 0, size.width(), size.height(), 99, 99);
    return mask;
}

} // namespace ImageCropperGeometry
