#include "PetController.h"
#include "PetControllerPrivate.h"

#include <QtCore/QBuffer>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QtGlobal>
#include <QtGui/QImageWriter>
#if HAVE_QT_MULTIMEDIA
#include <QtMultimedia/QVideoFrame>
#endif

using namespace memochat::pet;

void PetController::captureVisionFrame(const QString &frameBase64,
                                       const QString &frameMime,
                                       int frameWidth,
                                       int frameHeight)
{
    const QString encodedFrame = frameBase64.trimmed();
    if (_session_id.isEmpty() || encodedFrame.isEmpty()) {
        return;
    }

    postVisionCapture(encodedFrame,
                      frameMime,
                      frameWidth,
                      frameHeight,
                      QStringLiteral("qt_camera"),
                      QStringLiteral("local_frame_upload"));
}

#if HAVE_QT_MULTIMEDIA
bool PetController::captureVisionVideoFrame(const QVideoFrame &frame, int frameWidth, int frameHeight)
{
    if (_session_id.isEmpty() || !frame.isValid()) {
        return false;
    }

    const QImage image = frame.toImage();
    if (image.isNull()) {
        setError(QStringLiteral("摄像头实时帧转换失败"));
        return false;
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly)) {
        setError(QStringLiteral("摄像头实时帧编码失败"));
        return false;
    }

    QImageWriter writer(&buffer, "jpeg");
    writer.setQuality(82);
    if (!writer.write(image)) {
        setError(QStringLiteral("摄像头实时帧编码失败"));
        return false;
    }

    return postVisionCapture(QString::fromLatin1(bytes.toBase64()),
                             QStringLiteral("image/jpeg"),
                             frameWidth,
                             frameHeight,
                             QStringLiteral("qt_video_sink"),
                             QStringLiteral("live_frame_upload"));
}

QString PetController::captureVisionSegmentVideoFrame(const QVideoFrame &frame, int frameWidth, int frameHeight)
{
    if (_session_id.isEmpty() || !frame.isValid()) {
        return {};
    }

    const QImage image = frame.toImage();
    if (image.isNull()) {
        setError(QStringLiteral("摄像头实时帧转换失败"));
        return {};
    }

    const QByteArray signature = visionFrameSignature(image);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (shouldSkipVisionFrame(signature, now, QStringLiteral("qt_video_sink_segment"))) {
        return QStringLiteral("视觉画面未变化，已跳过");
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly)) {
        setError(QStringLiteral("摄像头实时帧编码失败"));
        return {};
    }

    QImageWriter writer(&buffer, "jpeg");
    writer.setQuality(72);
    if (!writer.write(image)) {
        setError(QStringLiteral("摄像头实时帧编码失败"));
        return {};
    }

    return appendVisionSegmentFrame(QString::fromLatin1(bytes.toBase64()),
                                    QStringLiteral("image/jpeg"),
                                    frameWidth,
                                    frameHeight,
                                    signature,
                                    QStringLiteral("qt_video_sink_segment"),
                                    QStringLiteral("keyframe_segment_upload"));
}
#endif

bool PetController::shouldSkipVisionFrame(const QByteArray &signature, qint64 now, const QString &source)
{
    if (signature.isEmpty()) {
        return false;
    }
    const bool duplicate = !_last_vision_frame_signature.isEmpty()
                           && visionFrameSignatureDistance(_last_vision_frame_signature, signature)
                               <= kVisionFrameSignatureDuplicateDistance
                           && _last_vision_frame_accepted_at_ms > 0
                           && (now - _last_vision_frame_accepted_at_ms) < kVisionDuplicateFrameCooldownMs;
    if (duplicate) {
        setStatusText(QStringLiteral("视觉画面未变化，已跳过"));
        return true;
    }
    _last_vision_frame_signature = signature;
    _last_vision_frame_accepted_at_ms = now;
    if (!source.trimmed().isEmpty()) {
        setStatusText(QStringLiteral("视觉画面变化，已采样"));
    }
    return false;
}

bool PetController::postVisionCapture(const QString &frameBase64,
                                      const QString &frameMime,
                                      int frameWidth,
                                      int frameHeight,
                                      const QString &source,
                                      const QString &transport)
{
    const QString encodedFrame = frameBase64.trimmed();
    if (_session_id.isEmpty() || encodedFrame.isEmpty()) {
        return false;
    }
    if (_vision_request_in_flight) {
        setStatusText(QStringLiteral("视觉分析正在进行，已保留最新帧"));
        return false;
    }

    QByteArray rawFrame = QByteArray::fromBase64(encodedFrame.toLatin1());
    QImage decodedFrame;
    if (!rawFrame.isEmpty()) {
        decodedFrame.loadFromData(rawFrame);
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!decodedFrame.isNull() && shouldSkipVisionFrame(visionFrameSignature(decodedFrame), now, source)) {
        return false;
    }

    QJsonObject metadata;
    metadata[QStringLiteral("source")] = source.trimmed().isEmpty()
                                             ? QStringLiteral("qt_camera")
                                             : source.trimmed();
    metadata[QStringLiteral("transport")] = transport.trimmed().isEmpty()
                                                ? QStringLiteral("local_frame_upload")
                                                : transport.trimmed();
    metadata[QStringLiteral("reply_language")] = _reply_language.trimmed().isEmpty()
                                                    ? QStringLiteral("zh-CN")
                                                    : _reply_language.trimmed();
    const QString speechRules = _speech_rules.trimmed();
    if (!speechRules.isEmpty()) {
        metadata[QStringLiteral("speech_rules")] = speechRules;
    }
    appendVoiceRuntimeMetadata(metadata);

    QJsonObject payload;
    payload[QStringLiteral("analyzer")] = QStringLiteral("opencv");
    payload[QStringLiteral("include_frame")] = false;
    payload[QStringLiteral("frame_base64")] = encodedFrame;
    payload[QStringLiteral("frame_mime")] = frameMime.trimmed().isEmpty()
                                                ? QStringLiteral("image/jpeg")
                                                : frameMime.trimmed();
    payload[QStringLiteral("frame_width")] = qMax(0, frameWidth);
    payload[QStringLiteral("frame_height")] = qMax(0, frameHeight);
    payload[QStringLiteral("metadata")] = metadata;

    setVisionRequestInFlight(true);
    postJson(petUrl(QStringLiteral("/sessions/%1/capture").arg(encodedSessionId())),
             payload,
             QStringLiteral("vision_capture"));
    return true;
}

QString PetController::appendVisionSegmentFrame(const QString &frameBase64,
                                                const QString &frameMime,
                                                int frameWidth,
                                                int frameHeight,
                                                const QByteArray &signature,
                                                const QString &source,
                                                const QString &transport)
{
    const QString encodedFrame = frameBase64.trimmed();
    if (_session_id.isEmpty() || encodedFrame.isEmpty()) {
        return {};
    }
    if (_vision_request_in_flight) {
        return QStringLiteral("视觉分析正在进行，已保留最新片段");
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (_vision_segment_frames.isEmpty() || _vision_segment_started_at_ms <= 0) {
        _vision_segment_started_at_ms = now;
    }

    QJsonObject frame;
    frame[QStringLiteral("frame_base64")] = encodedFrame;
    frame[QStringLiteral("frame_mime")] = frameMime.trimmed().isEmpty()
                                              ? QStringLiteral("image/jpeg")
                                              : frameMime.trimmed();
    frame[QStringLiteral("frame_width")] = qMax(0, frameWidth);
    frame[QStringLiteral("frame_height")] = qMax(0, frameHeight);
    frame[QStringLiteral("captured_at_ms")] = now;
    frame[QStringLiteral("t_ms")] = int(qMax<qint64>(0, now - _vision_segment_started_at_ms));
    if (!signature.isEmpty()) {
        frame[QStringLiteral("frame_signature")] = QString::fromLatin1(signature.toHex());
    }
    _vision_segment_frames.append(frame);

    while (_vision_segment_frames.size() > kVisionSegmentMaxFrames) {
        _vision_segment_frames.removeAt(0);
    }

    while (!_vision_segment_frames.isEmpty()) {
        const QJsonObject firstFrame = _vision_segment_frames.first().toObject();
        const qint64 capturedAt = firstFrame.value(QStringLiteral("captured_at_ms")).toVariant().toLongLong();
        if (capturedAt <= 0 || now - capturedAt <= kVisionSegmentWindowMs) {
            break;
        }
        _vision_segment_frames.removeAt(0);
    }

    normalizeVisionSegmentFrames(_vision_segment_frames);
    _vision_segment_started_at_ms = _vision_segment_frames.isEmpty()
                                        ? 0
                                        : _vision_segment_frames.first().toObject().value(QStringLiteral("captured_at_ms")).toVariant().toLongLong();

    const qint64 durationMs = qMax<qint64>(0, now - _vision_segment_started_at_ms);
    const bool uploadDue = _vision_segment_last_posted_at_ms <= 0
                           || (now - _vision_segment_last_posted_at_ms) >= kVisionSegmentMinUploadMs;
    if (_vision_segment_frames.size() >= 2 && durationMs >= kVisionSegmentMinUploadMs && uploadDue) {
        postVisionSegment(source, transport);
        return QStringLiteral("视觉片段已上传");
    }

    return QStringLiteral("视觉环形缓冲采样中 %1/%2")
        .arg(_vision_segment_frames.size())
        .arg(kVisionSegmentMaxFrames);
}

void PetController::postVisionSegment(const QString &source, const QString &transport)
{
    if (_session_id.isEmpty() || _vision_segment_frames.isEmpty()) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const int durationMs = int(qMax<qint64>(0, now - _vision_segment_started_at_ms));

    QJsonObject metadata;
    metadata[QStringLiteral("source")] = source.trimmed().isEmpty()
                                             ? QStringLiteral("qt_video_sink_segment")
                                             : source.trimmed();
    metadata[QStringLiteral("transport")] = transport.trimmed().isEmpty()
                                                ? QStringLiteral("keyframe_segment_upload")
                                                : transport.trimmed();
    metadata[QStringLiteral("reply_language")] = _reply_language.trimmed().isEmpty()
                                                    ? QStringLiteral("zh-CN")
                                                    : _reply_language.trimmed();
    const QString speechRules = _speech_rules.trimmed();
    if (!speechRules.isEmpty()) {
        metadata[QStringLiteral("speech_rules")] = speechRules;
    }
    appendVoiceRuntimeMetadata(metadata);

    QJsonObject payload;
    payload[QStringLiteral("analyzer")] = QStringLiteral("opencv");
    payload[QStringLiteral("include_frame")] = false;
    payload[QStringLiteral("segment_id")] = QStringLiteral("qml-segment-%1").arg(now);
    payload[QStringLiteral("duration_ms")] = durationMs;
    payload[QStringLiteral("frames")] = _vision_segment_frames;
    payload[QStringLiteral("metadata")] = metadata;

    _vision_segment_last_posted_at_ms = now;
    setVisionRequestInFlight(true);

    postJson(petUrl(QStringLiteral("/sessions/%1/capture-segment").arg(encodedSessionId())),
             payload,
             QStringLiteral("vision_segment_capture"));
    _vision_segment_frames = QJsonArray();
    _vision_segment_started_at_ms = 0;
}

QString PetController::nextVisionCaptureFilePath() const
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.trimmed().isEmpty()) {
        root = QDir::tempPath();
    }
    const QString dir = QDir(root).filePath(QStringLiteral("pet-vision"));
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("capture-%1.jpg").arg(QDateTime::currentMSecsSinceEpoch()));
}

void PetController::captureVisionFrameFile(const QString &filePath, int frameWidth, int frameHeight)
{
    captureVisionFrameFileWithMetadata(filePath,
                                       frameWidth,
                                       frameHeight,
                                       QStringLiteral("qt_camera"),
                                       QStringLiteral("local_frame_upload"));
}

void PetController::captureVisionFrameFileWithMetadata(const QString &filePath,
                                                       int frameWidth,
                                                       int frameHeight,
                                                       const QString &source,
                                                       const QString &transport)
{
    QString localPath = filePath.trimmed();
    if (localPath.startsWith(QStringLiteral("file://"))) {
        localPath = QUrl(localPath).toLocalFile();
    }
    if (_session_id.isEmpty() || localPath.isEmpty()) {
        return;
    }

    const QFileInfo info(localPath);
    if (info.exists() && info.size() > 8 * 1024 * 1024) {
        QFile::remove(localPath);
        setError(QStringLiteral("摄像头帧过大，已跳过"));
        return;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("摄像头帧读取失败"));
        return;
    }
    const QByteArray bytes = file.readAll();
    file.close();
    QFile::remove(localPath);
    if (bytes.isEmpty()) {
        setError(QStringLiteral("摄像头帧为空"));
        return;
    }

    QString mime = QStringLiteral("image/jpeg");
    const QString suffix = info.suffix().toLower();
    if (suffix == QStringLiteral("png")) {
        mime = QStringLiteral("image/png");
    } else if (suffix == QStringLiteral("webp")) {
        mime = QStringLiteral("image/webp");
    }
    postVisionCapture(QString::fromLatin1(bytes.toBase64()), mime, frameWidth, frameHeight, source, transport);
}
