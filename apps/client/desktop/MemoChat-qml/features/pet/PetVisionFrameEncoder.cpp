#include "PetVisionFrameEncoder.h"

#include "PetVisionFrameUtils.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>
#include <QtCore/QUrl>
#include <QtGui/QImageWriter>

namespace memochat::pet
{

EncodedVisionFrame encodeVisionImageAsJpeg(const QImage& image, int quality)
{
    EncodedVisionFrame result;
    if (image.isNull())
    {
        result.error = QStringLiteral("摄像头实时帧转换失败");
        return result;
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly))
    {
        result.error = QStringLiteral("摄像头实时帧编码失败");
        return result;
    }

    QImageWriter writer(&buffer, "jpeg");
    writer.setQuality(quality);
    if (!writer.write(image))
    {
        result.error = QStringLiteral("摄像头实时帧编码失败");
        return result;
    }

    result.frameBase64 = QString::fromLatin1(bytes.toBase64());
    result.frameMime = QStringLiteral("image/jpeg");
    result.signature = visionFrameSignature(image);
    result.ok = true;
    return result;
}

#if HAVE_QT_MULTIMEDIA
EncodedVisionFrame encodeVisionVideoFrameAsJpeg(const QVideoFrame& frame, int quality)
{
    if (!frame.isValid())
    {
        return {};
    }
    return encodeVisionImageAsJpeg(frame.toImage(), quality);
}
#endif

QString visionFrameFileMime(const QString& suffix)
{
    const QString normalized = suffix.toLower();
    if (normalized == QStringLiteral("png"))
    {
        return QStringLiteral("image/png");
    }
    if (normalized == QStringLiteral("webp"))
    {
        return QStringLiteral("image/webp");
    }
    return QStringLiteral("image/jpeg");
}

EncodedVisionFrame readVisionFrameFile(const QString& filePath)
{
    EncodedVisionFrame result;
    QString localPath = filePath.trimmed();
    if (localPath.startsWith(QStringLiteral("file://")))
    {
        localPath = QUrl(localPath).toLocalFile();
    }
    if (localPath.isEmpty())
    {
        return result;
    }

    QFileInfo info(localPath);
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        result.error = QStringLiteral("摄像头帧读取失败");
        return result;
    }
    const QByteArray bytes = file.readAll();
    if (bytes.isEmpty())
    {
        result.error = QStringLiteral("摄像头帧为空");
        return result;
    }

    result.frameBase64 = QString::fromLatin1(bytes.toBase64());
    result.frameMime = visionFrameFileMime(info.suffix());
    result.ok = true;
    return result;
}

} // namespace memochat::pet
