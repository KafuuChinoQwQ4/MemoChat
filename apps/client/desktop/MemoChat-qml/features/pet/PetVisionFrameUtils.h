#ifndef PETVISIONFRAMEUTILS_H
#define PETVISIONFRAMEUTILS_H

#include <QtCore/QByteArray>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QImage>

namespace memochat::pet
{

constexpr int kVisionSegmentMaxFrames = 14;
constexpr int kVisionSegmentWindowMs = 20000;
constexpr int kVisionSegmentMinUploadMs = 15000;
constexpr int kVisionDuplicateFrameCooldownMs = 30000;
constexpr int kVisionFrameSignatureSize = 16;
constexpr int kVisionFrameSignatureDuplicateDistance = 32;

inline QByteArray visionFrameSignature(const QImage& image)
{
    if (image.isNull())
    {
        return {};
    }
    const QImage normalized = image.convertToFormat(QImage::Format_RGB32)
                                  .scaled(kVisionFrameSignatureSize,
                                          kVisionFrameSignatureSize,
                                          Qt::IgnoreAspectRatio,
                                          Qt::FastTransformation);
    QByteArray signature;
    signature.reserve(kVisionFrameSignatureSize * kVisionFrameSignatureSize);
    for (int y = 0; y < normalized.height(); ++y)
    {
        for (int x = 0; x < normalized.width(); ++x)
        {
            const QColor pixel(normalized.pixel(x, y));
            const int luminance = (pixel.red() * 30 + pixel.green() * 59 + pixel.blue() * 11) / 100;
            signature.append(char(luminance / 16));
        }
    }
    return signature;
}

inline int visionFrameSignatureDistance(const QByteArray& left, const QByteArray& right)
{
    if (left.isEmpty() || right.isEmpty() || left.size() != right.size())
    {
        return 9999;
    }
    int distance = 0;
    for (int index = 0; index < left.size(); ++index)
    {
        distance += qAbs(int(uchar(left.at(index))) - int(uchar(right.at(index))));
    }
    return distance;
}

inline void normalizeVisionSegmentFrames(QJsonArray& frames)
{
    if (frames.isEmpty())
    {
        return;
    }
    const qint64 firstCapturedAt =
        frames.first().toObject().value(QStringLiteral("captured_at_ms")).toVariant().toLongLong();
    if (firstCapturedAt <= 0)
    {
        return;
    }
    for (int index = 0; index < frames.size(); ++index)
    {
        QJsonObject frame = frames.at(index).toObject();
        const qint64 capturedAt = frame.value(QStringLiteral("captured_at_ms")).toVariant().toLongLong();
        frame[QStringLiteral("t_ms")] = int(qMax<qint64>(0, capturedAt - firstCapturedAt));
        frames[index] = frame;
    }
}

} // namespace memochat::pet

#endif // PETVISIONFRAMEUTILS_H
