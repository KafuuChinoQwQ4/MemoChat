#ifndef PETVISIONFRAMEENCODER_H
#define PETVISIONFRAMEENCODER_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtGui/QImage>

#if HAVE_QT_MULTIMEDIA
#include <QtMultimedia/QVideoFrame>
#endif

namespace memochat::pet
{

struct EncodedVisionFrame
{
    QString frameBase64;
    QString frameMime;
    QByteArray signature;
    QString error;
    bool ok = false;
};

EncodedVisionFrame encodeVisionImageAsJpeg(const QImage& image, int quality);

#if HAVE_QT_MULTIMEDIA
EncodedVisionFrame encodeVisionVideoFrameAsJpeg(const QVideoFrame& frame, int quality);
#endif

QString visionFrameFileMime(const QString& suffix);
EncodedVisionFrame readVisionFrameFile(const QString& filePath);

} // namespace memochat::pet

#endif // PETVISIONFRAMEENCODER_H
