#include "PetController.h"

#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QtCore/QBuffer>
#include <QtCore/QIODevice>
#include <QColor>
#include <QtGui/QImageWriter>
#if HAVE_QT_MULTIMEDIA
#include <QtMultimedia/QVideoFrame>
#endif
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QtGlobal>

#include <initializer_list>

namespace {
constexpr int kMaxRememberedPetControlEvents = 256;
constexpr int kVisionSegmentMaxFrames = 14;
constexpr int kVisionSegmentWindowMs = 20000;
constexpr int kVisionSegmentMinUploadMs = 15000;
constexpr int kVisionDuplicateFrameCooldownMs = 30000;
constexpr int kVisionFrameSignatureSize = 16;
constexpr int kVisionFrameSignatureDuplicateDistance = 32;

QByteArray visionFrameSignature(const QImage &image)
{
    if (image.isNull()) {
        return {};
    }
    const QImage normalized = image.convertToFormat(QImage::Format_RGB32)
                                  .scaled(kVisionFrameSignatureSize,
                                          kVisionFrameSignatureSize,
                                          Qt::IgnoreAspectRatio,
                                          Qt::FastTransformation);
    QByteArray signature;
    signature.reserve(kVisionFrameSignatureSize * kVisionFrameSignatureSize);
    for (int y = 0; y < normalized.height(); ++y) {
        for (int x = 0; x < normalized.width(); ++x) {
            const QColor pixel(normalized.pixel(x, y));
            const int luminance = (pixel.red() * 30 + pixel.green() * 59 + pixel.blue() * 11) / 100;
            signature.append(char(luminance / 16));
        }
    }
    return signature;
}

void configurePetRequest(QNetworkRequest &request)
{
    const QString scheme = request.url().scheme().trimmed().toLower();
    if (scheme == QLatin1String("http")) {
        request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("close"));
        return;
    }
    if (scheme != QLatin1String("https")) {
        return;
    }

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
}

int visionFrameSignatureDistance(const QByteArray &left, const QByteArray &right)
{
    if (left.isEmpty() || right.isEmpty() || left.size() != right.size()) {
        return 9999;
    }
    int distance = 0;
    for (int index = 0; index < left.size(); ++index) {
        distance += qAbs(int(uchar(left.at(index))) - int(uchar(right.at(index))));
    }
    return distance;
}

QString stringFromVariant(const QVariantMap &settings, const QString &key)
{
    return settings.value(key).toString().trimmed();
}

QString firstStringFromVariant(const QVariantMap &settings, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QString value = stringFromVariant(settings, QString::fromLatin1(key));
        if (!value.isEmpty()) {
            return value;
        }
    }
    return {};
}

QString localPathFromUrlText(const QString &value)
{
    QString text = value.trimmed();
    if (text.startsWith(QStringLiteral("file://"))) {
        text = QUrl(text).toLocalFile();
    }
    return text.trimmed();
}

QString joinedPath(const QString &directory, const QString &fileName)
{
    const QString dir = localPathFromUrlText(directory);
    const QString file = localPathFromUrlText(fileName);
    if (dir.isEmpty() || file.isEmpty()) {
        return {};
    }
    if (QDir::isAbsolutePath(file)) {
        return file;
    }
    return QDir(dir).filePath(file);
}

bool looksLikeAudioPath(const QString &path)
{
    const QString suffix = QFileInfo(path.trimmed()).suffix().toLower();
    return suffix == QStringLiteral("wav")
           || suffix == QStringLiteral("mp3")
           || suffix == QStringLiteral("ogg")
           || suffix == QStringLiteral("flac")
           || suffix == QStringLiteral("m4a")
           || suffix == QStringLiteral("aac");
}

QString voiceTextLanguage(const QString &language)
{
    const QString normalized = language.trimmed().toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (normalized == QStringLiteral("ja-jp") || normalized == QStringLiteral("jp") || normalized == QStringLiteral("ja")) {
        return QStringLiteral("ja");
    }
    if (normalized == QStringLiteral("en-us") || normalized == QStringLiteral("en-gb") || normalized == QStringLiteral("en")) {
        return QStringLiteral("en");
    }
    if (normalized == QStringLiteral("ko-kr") || normalized == QStringLiteral("ko")) {
        return QStringLiteral("ko");
    }
    if (normalized == QStringLiteral("fr-fr") || normalized == QStringLiteral("fr")) {
        return QStringLiteral("fr");
    }
    if (normalized == QStringLiteral("es-es") || normalized == QStringLiteral("es")) {
        return QStringLiteral("es");
    }
    return QStringLiteral("zh");
}

bool isWslRuntime()
{
#ifdef Q_OS_LINUX
    QFile file(QStringLiteral("/proc/sys/kernel/osrelease"));
    if (file.open(QIODevice::ReadOnly)) {
        const QString release = QString::fromLatin1(file.readAll()).toLower();
        return release.contains(QStringLiteral("microsoft")) || release.contains(QStringLiteral("wsl"));
    }
#endif
    return false;
}

QString windowsPowerShellPath()
{
    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
    if (!fromPath.isEmpty()) {
        return fromPath;
    }
    const QString fallback = QStringLiteral("/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe");
    return QFileInfo::exists(fallback) ? fallback : QString();
}

QString powershellEncodedCommand(const QString &script)
{
    const auto *utf16 = reinterpret_cast<const char *>(script.utf16());
    const QByteArray bytes(utf16, script.size() * int(sizeof(ushort)));
    return QString::fromLatin1(bytes.toBase64());
}

QString windowsImeBridgeScript(const QString &initialText)
{
    const QString initialBase64 = QString::fromLatin1(initialText.toUtf8().toBase64());
    return QString::fromUtf8(R"PS(
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$initialText = [System.Text.Encoding]::UTF8.GetString([System.Convert]::FromBase64String('%1'))
$form = New-Object System.Windows.Forms.Form
$form.Text = 'MemoChat Chinese Input'
$form.StartPosition = 'CenterScreen'
$form.Width = 560
$form.Height = 180
$form.TopMost = $true
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
$form.MaximizeBox = $false
$form.MinimizeBox = $false

$textBox = New-Object System.Windows.Forms.TextBox
$textBox.Multiline = $true
$textBox.AcceptsReturn = $true
$textBox.AcceptsTab = $false
$textBox.ImeMode = [System.Windows.Forms.ImeMode]::On
$textBox.Font = New-Object System.Drawing.Font('Microsoft YaHei UI', 12)
$textBox.Left = 12
$textBox.Top = 12
$textBox.Width = 518
$textBox.Height = 76
$textBox.Text = $initialText

$okButton = New-Object System.Windows.Forms.Button
$okButton.Text = 'OK'
$okButton.Left = 354
$okButton.Top = 100
$okButton.Width = 82
$okButton.Height = 30
$okButton.DialogResult = [System.Windows.Forms.DialogResult]::OK

$cancelButton = New-Object System.Windows.Forms.Button
$cancelButton.Text = 'Cancel'
$cancelButton.Left = 448
$cancelButton.Top = 100
$cancelButton.Width = 82
$cancelButton.Height = 30
$cancelButton.DialogResult = [System.Windows.Forms.DialogResult]::Cancel

$form.Controls.AddRange(@($textBox, $okButton, $cancelButton))
$form.AcceptButton = $okButton
$form.CancelButton = $cancelButton
$form.Add_Shown({ $textBox.Focus(); $textBox.Select($textBox.TextLength, 0) })

if ($form.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
    [Console]::Out.Write($textBox.Text)
}
)PS").arg(initialBase64);
}

void normalizeVisionSegmentFrames(QJsonArray &frames)
{
    if (frames.isEmpty()) {
        return;
    }
    const qint64 firstCapturedAt = frames.first().toObject().value(QStringLiteral("captured_at_ms")).toVariant().toLongLong();
    if (firstCapturedAt <= 0) {
        return;
    }
    for (int index = 0; index < frames.size(); ++index) {
        QJsonObject frame = frames.at(index).toObject();
        const qint64 capturedAt = frame.value(QStringLiteral("captured_at_ms")).toVariant().toLongLong();
        frame[QStringLiteral("t_ms")] = int(qMax<qint64>(0, capturedAt - firstCapturedAt));
        frames[index] = frame;
    }
}

QString windowsCameraCaptureScript()
{
    return QString::fromUtf8(R"PS(
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Add-Type -AssemblyName System.Runtime.WindowsRuntime

$null = [Windows.Media.Capture.MediaCapture,Windows.Media.Capture,ContentType=WindowsRuntime]
$null = [Windows.Media.Capture.MediaCaptureInitializationSettings,Windows.Media.Capture,ContentType=WindowsRuntime]
$null = [Windows.Media.Capture.StreamingCaptureMode,Windows.Media.Capture,ContentType=WindowsRuntime]
$null = [Windows.Media.MediaProperties.ImageEncodingProperties,Windows.Media.MediaProperties,ContentType=WindowsRuntime]
$null = [Windows.Storage.Streams.InMemoryRandomAccessStream,Windows.Storage.Streams,ContentType=WindowsRuntime]
$null = [Windows.Storage.Streams.DataReader,Windows.Storage.Streams,ContentType=WindowsRuntime]
$null = [Windows.Graphics.Imaging.BitmapDecoder,Windows.Graphics.Imaging,ContentType=WindowsRuntime]

$asTaskOperation = ([System.WindowsRuntimeSystemExtensions].GetMethods() |
    Where-Object {
        $_.Name -eq 'AsTask' -and $_.IsGenericMethodDefinition -and
        $_.GetParameters().Count -eq 1 -and
        $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
    } | Select-Object -First 1)
$asTaskAction = ([System.WindowsRuntimeSystemExtensions].GetMethods() |
    Where-Object {
        $_.Name -eq 'AsTask' -and -not $_.IsGenericMethodDefinition -and
        $_.GetParameters().Count -eq 1 -and
        $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncAction'
    } | Select-Object -First 1)

function Await-Operation($operation, [Type] $resultType) {
    $task = $asTaskOperation.MakeGenericMethod($resultType).Invoke($null, @($operation))
    $task.Wait()
    return $task.Result
}

function Await-Action($operation) {
    $task = $asTaskAction.Invoke($null, @($operation))
    $task.Wait()
}

$mediaCapture = $null
$stream = $null
try {
    $settings = [Windows.Media.Capture.MediaCaptureInitializationSettings]::new()
    $settings.StreamingCaptureMode = [Windows.Media.Capture.StreamingCaptureMode]::Video
    $mediaCapture = [Windows.Media.Capture.MediaCapture]::new()
    Await-Action $mediaCapture.InitializeAsync($settings)

    $stream = [Windows.Storage.Streams.InMemoryRandomAccessStream]::new()
    $encoding = [Windows.Media.MediaProperties.ImageEncodingProperties]::CreateJpeg()
    Await-Action $mediaCapture.CapturePhotoToStreamAsync($encoding, $stream)
    $size = [uint32] $stream.Size
    if ($size -le 0) {
        throw 'Windows camera returned an empty frame.'
    }

    $width = 0
    $height = 0
    try {
        $stream.Seek(0)
        $decoder = Await-Operation ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
        $width = [int] $decoder.PixelWidth
        $height = [int] $decoder.PixelHeight
    } catch {
        $width = 0
        $height = 0
    }

    $stream.Seek(0)
    $reader = [Windows.Storage.Streams.DataReader]::new($stream.GetInputStreamAt(0))
    [void] (Await-Operation $reader.LoadAsync($size) ([UInt32]))
    $bytes = New-Object byte[] $size
    $reader.ReadBytes($bytes)

    [pscustomobject] @{
        frame_base64 = [Convert]::ToBase64String($bytes)
        frame_mime = 'image/jpeg'
        frame_width = $width
        frame_height = $height
    } | ConvertTo-Json -Compress
} finally {
    if ($mediaCapture -is [System.IDisposable]) {
        $mediaCapture.Dispose()
    }
    if ($stream -is [System.IDisposable]) {
        $stream.Dispose()
    }
}
)PS");
}

}

PetController::PetController(ClientGateway *gateway, QObject *parent)
    : QObject(parent)
    , _gateway(gateway)
{
    connect(&_model, &PetModel::changed, this, &PetController::petStateChanged);
}

PetController::~PetController()
{
    stopStream();
    if (_windows_camera_bridge_process) {
        auto *process = _windows_camera_bridge_process.data();
        _windows_camera_bridge_process.clear();
        process->disconnect(this);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(250);
        }
        process->deleteLater();
    }
}

QString PetController::audioUrl() const
{
    const QString raw = _model.audioUrl().trimmed();
    if (raw.isEmpty()) {
        return {};
    }
    const QUrl parsed(raw);
    if (parsed.isValid() && !parsed.isRelative()) {
        return raw;
    }
    const QString path = raw.startsWith(QLatin1Char('/')) ? raw : QStringLiteral("/%1").arg(raw);
    if (path.startsWith(QStringLiteral("/audio/"))) {
        const QString mediaBase = gate_media_url_prefix.trimmed().isEmpty()
                                      ? gate_url_prefix.trimmed()
                                      : gate_media_url_prefix.trimmed();
        QUrl base(mediaBase);
        if (base.isValid() && !base.host().isEmpty()) {
            const int port = base.port(8096);
            if (port == 8096) {
                base.setPath(QStringLiteral("/pet") + path);
            } else if (port == 80 || port == 8080 || port == 8443) {
                base.setScheme(QStringLiteral("http"));
                base.setPort(8096);
                base.setPath(QStringLiteral("/pet") + path);
            } else {
                base.setPath(QStringLiteral("/ai/pet") + path);
            }
            QUrlQuery query(base);
            if (!_model.turnId().isEmpty()) {
                query.addQueryItem(QStringLiteral("turn"), _model.turnId());
            }
            if (!_model.eventId().isEmpty()) {
                query.addQueryItem(QStringLiteral("event"), _model.eventId());
            }
            base.setQuery(query);
            return base.toString();
        }
    }
    QUrl url = petUrl(path);
    QUrlQuery query(url);
    if (!_model.turnId().isEmpty()) {
        query.addQueryItem(QStringLiteral("turn"), _model.turnId());
    }
    if (!_model.eventId().isEmpty()) {
        query.addQueryItem(QStringLiteral("event"), _model.eventId());
    }
    url.setQuery(query);
    return url.toString();
}

void PetController::startSession()
{
    if (_busy) {
        return;
    }
    resetControlEventDedupe();
    setError({});
    setBusy(true, QStringLiteral("正在连接桌宠"));
    QJsonObject payload = authPayload();
    payload[QStringLiteral("profile_id")] = QStringLiteral("default");
    payload[QStringLiteral("persona")] = QStringLiteral("memo-pet");
    payload[QStringLiteral("provider")] = QStringLiteral("scripted");
    postJson(petUrl(QStringLiteral("/sessions")), payload, QStringLiteral("create_session"));
}

void PetController::sendText(const QString &text)
{
    const QString content = text.trimmed();
    if (content.isEmpty()) {
        return;
    }
    if (_input_request_in_flight) {
        setStatusText(QStringLiteral("上一条消息仍在发送"));
        return;
    }
    if (_session_id.isEmpty()) {
        setError(QStringLiteral("桌宠会话未连接"));
        return;
    }
    if (!_streaming) {
        startStream();
    }

    _model.clearSpeech();
    _input_request_in_flight = true;
    setBusy(true, QStringLiteral("正在发送"));
    QJsonObject payload = authPayload();
    payload[QStringLiteral("content")] = content;
    if (!_selected_model_type.trimmed().isEmpty()) {
        payload[QStringLiteral("model_type")] = _selected_model_type.trimmed();
    }
    if (!_selected_model_name.trimmed().isEmpty()) {
        payload[QStringLiteral("model_name")] = _selected_model_name.trimmed();
    }
    QJsonObject metadata;
    metadata[QStringLiteral("reply_language")] = _reply_language.trimmed().isEmpty()
                                                    ? QStringLiteral("zh-CN")
                                                    : _reply_language.trimmed();
    const QString speechRules = _speech_rules.trimmed();
    if (!speechRules.isEmpty()) {
        metadata[QStringLiteral("speech_rules")] = speechRules;
    }
    appendVoiceRuntimeMetadata(metadata);
    payload[QStringLiteral("metadata")] = metadata;
    postJson(petUrl(QStringLiteral("/sessions/%1/input").arg(encodedSessionId())),
             payload,
             QStringLiteral("input"));
}

void PetController::sendObservation(const QVariantMap &observation)
{
    if (_session_id.isEmpty()) {
        return;
    }
    QJsonObject payload = QJsonObject::fromVariantMap(observation);
    if (payload.isEmpty()) {
        payload = defaultObservationPayload();
    }
    QJsonObject vision = payload.value(QStringLiteral("vision")).toObject();
    appendVoiceRuntimeMetadata(vision);
    if (!vision.isEmpty()) {
        payload[QStringLiteral("vision")] = vision;
    }
    postJson(petUrl(QStringLiteral("/sessions/%1/observation").arg(encodedSessionId())),
             payload,
             QStringLiteral("observation"));
}

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

void PetController::interrupt()
{
    if (_session_id.isEmpty()) {
        return;
    }
    postJson(petUrl(QStringLiteral("/sessions/%1/interrupt").arg(encodedSessionId())),
             QJsonObject{},
             QStringLiteral("interrupt"));
}

void PetController::stopStream()
{
    if (_stream_reply) {
        auto *reply = _stream_reply.data();
        _stream_reply.clear();
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    _stream_buffer.clear();
    if (_streaming) {
        _streaming = false;
        emit stateChanged();
    }
}

void PetController::clearSpeech()
{
    _model.clearSpeech();
}

void PetController::startVoiceTraining(const QVariantMap &request)
{
    if (_voice_training_busy) {
        return;
    }
    QJsonObject payload = authPayload();
    const QJsonObject requestObject = QJsonObject::fromVariantMap(request);
    for (auto it = requestObject.constBegin(); it != requestObject.constEnd(); ++it) {
        payload.insert(it.key(), it.value());
    }
    if (!payload.value(QStringLiteral("consent_confirmed")).toBool(false)) {
        setError(QStringLiteral("声音训练需要先确认参考音频授权"));
        _voice_training_status = QStringLiteral("blocked");
        _voice_training_message = QStringLiteral("请先勾选参考音频授权确认");
        emit voiceTrainingChanged();
        return;
    }
    QString referenceAudioPath = payload.value(QStringLiteral("reference_audio_path")).toString().trimmed();
    if (referenceAudioPath.startsWith(QStringLiteral("file://"))) {
        referenceAudioPath = QUrl(referenceAudioPath).toLocalFile();
        payload[QStringLiteral("reference_audio_path")] = referenceAudioPath;
    }
    if (referenceAudioPath.isEmpty()) {
        setError(QStringLiteral("声音训练缺少参考音频路径"));
        _voice_training_status = QStringLiteral("blocked");
        _voice_training_message = QStringLiteral("请先设置默认音频文件");
        emit voiceTrainingChanged();
        return;
    }
    QFileInfo referenceAudio(referenceAudioPath);
    if (referenceAudio.exists() && referenceAudio.isFile() && referenceAudio.size() > 0
        && referenceAudio.size() <= 32 * 1024 * 1024) {
        QFile file(referenceAudio.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonObject metadata = payload.value(QStringLiteral("metadata")).toObject();
            metadata[QStringLiteral("reference_audio_base64")] =
                QString::fromLatin1(file.readAll().toBase64());
            metadata[QStringLiteral("reference_audio_file_name")] = referenceAudio.fileName();
            metadata[QStringLiteral("reference_audio_size_bytes")] = qint64(referenceAudio.size());
            metadata[QStringLiteral("reference_audio_transfer")] = QStringLiteral("client_base64");
            payload[QStringLiteral("metadata")] = metadata;
        }
    }
    setError({});
    setVoiceTrainingBusy(true, QStringLiteral("正在提交声音训练准备任务"));
    postJson(petUrl(QStringLiteral("/voice-training/jobs")),
             payload,
             QStringLiteral("voice_training_create"));
}

bool PetController::windowsImeBridgeAvailable() const
{
    return isWslRuntime() && !windowsPowerShellPath().isEmpty();
}

bool PetController::windowsCameraBridgeAvailable() const
{
    return isWslRuntime() && !windowsPowerShellPath().isEmpty();
}

void PetController::openWindowsImeBridge(const QString &initialText)
{
    if (_windows_ime_bridge_busy) {
        return;
    }
    const QString program = windowsPowerShellPath();
    if (!isWslRuntime() || program.isEmpty()) {
        setError(QStringLiteral("Windows 中文输入桥只在 WSL 中可用"));
        return;
    }

    auto *process = new QProcess(this);
    _windows_ime_bridge_process = process;
    setWindowsImeBridgeBusy(true);

    connect(process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                if (_windows_ime_bridge_process == process) {
                    _windows_ime_bridge_process.clear();
                }
                const QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
                const QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
                process->deleteLater();
                setWindowsImeBridgeBusy(false);
                if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                    setError(error.isEmpty()
                                 ? QStringLiteral("Windows 中文输入桥已退出")
                                 : QStringLiteral("Windows 中文输入桥失败: %1").arg(error));
                    return;
                }
                if (!output.isEmpty()) {
                    emit windowsImeTextCommitted(output);
                }
            });

    const QString script = windowsImeBridgeScript(initialText);
    process->setProgram(program);
    process->setArguments({
        QStringLiteral("-NoProfile"),
        QStringLiteral("-Sta"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-EncodedCommand"),
        powershellEncodedCommand(script),
    });
    process->start();
    if (!process->waitForStarted(3000)) {
        if (_windows_ime_bridge_process == process) {
            _windows_ime_bridge_process.clear();
        }
        const QString error = process->errorString();
        process->deleteLater();
        setWindowsImeBridgeBusy(false);
        setError(QStringLiteral("Windows 中文输入桥启动失败: %1").arg(error));
    }
}

bool PetController::captureVisionWindowsCameraFrame()
{
    if (_session_id.isEmpty()) {
        setError(QStringLiteral("桌宠会话未连接"));
        return false;
    }
    if (_windows_camera_bridge_busy) {
        setStatusText(QStringLiteral("Windows 摄像头桥正在捕捉"));
        return false;
    }

    const QString program = windowsPowerShellPath();
    if (!isWslRuntime() || program.isEmpty()) {
        setError(QStringLiteral("Windows 摄像头桥只在 WSL 中可用"));
        return false;
    }

    auto *process = new QProcess(this);
    _windows_camera_bridge_process = process;
    setWindowsCameraBridgeBusy(true);
    setStatusText(QStringLiteral("Windows 摄像头桥正在捕捉"));

    connect(process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                if (_windows_camera_bridge_process == process) {
                    _windows_camera_bridge_process.clear();
                }
                const QByteArray stdoutBytes = process->readAllStandardOutput();
                const QString output = QString::fromUtf8(stdoutBytes).trimmed();
                QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
                if (error.isEmpty() && !output.startsWith(QLatin1Char('{'))) {
                    error = output;
                }
                process->deleteLater();
                setWindowsCameraBridgeBusy(false);

                if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                    setError(error.isEmpty()
                                 ? QStringLiteral("Windows 摄像头桥已退出")
                                 : QStringLiteral("Windows 摄像头桥失败: %1").arg(error));
                    return;
                }

                const int jsonStart = output.indexOf(QLatin1Char('{'));
                const int jsonEnd = output.lastIndexOf(QLatin1Char('}'));
                if (jsonStart < 0 || jsonEnd < jsonStart) {
                    setError(QStringLiteral("Windows 摄像头桥响应格式错误"));
                    return;
                }

                QJsonParseError parseError;
                const QByteArray jsonBytes = output.mid(jsonStart, jsonEnd - jsonStart + 1).toUtf8();
                const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                    setError(QStringLiteral("Windows 摄像头桥响应格式错误"));
                    return;
                }

                const QJsonObject payload = doc.object();
                const QString frameBase64 = payload.value(QStringLiteral("frame_base64")).toString().trimmed();
                if (frameBase64.isEmpty()) {
                    setError(QStringLiteral("Windows 摄像头桥未返回画面"));
                    return;
                }

                postVisionCapture(frameBase64,
                                  payload.value(QStringLiteral("frame_mime")).toString(QStringLiteral("image/jpeg")),
                                  payload.value(QStringLiteral("frame_width")).toInt(0),
                                  payload.value(QStringLiteral("frame_height")).toInt(0),
                                  QStringLiteral("windows_camera_bridge"),
                                  QStringLiteral("wsl_windows_camera_bridge"));
                setStatusText(QStringLiteral("Windows 摄像头桥帧已发送"));
            });

    process->setProgram(program);
    process->setArguments({
        QStringLiteral("-NoProfile"),
        QStringLiteral("-Sta"),
        QStringLiteral("-NonInteractive"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-WindowStyle"),
        QStringLiteral("Hidden"),
        QStringLiteral("-EncodedCommand"),
        powershellEncodedCommand(windowsCameraCaptureScript()),
    });
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->start();
    if (!process->waitForStarted(3000)) {
        if (_windows_camera_bridge_process == process) {
            _windows_camera_bridge_process.clear();
        }
        const QString error = process->errorString();
        process->deleteLater();
        setWindowsCameraBridgeBusy(false);
        setError(QStringLiteral("Windows 摄像头桥启动失败: %1").arg(error));
        return false;
    }
    return true;
}

void PetController::setModelSelection(const QString &modelType, const QString &modelName)
{
    const QString nextType = modelType.trimmed();
    const QString nextName = modelName.trimmed();
    if (_selected_model_type == nextType && _selected_model_name == nextName) {
        return;
    }
    _selected_model_type = nextType;
    _selected_model_name = nextName;
    emit modelSelectionChanged();
}

void PetController::setReplyLanguage(const QString &language)
{
    QString nextLanguage = language.trimmed();
    if (nextLanguage.isEmpty()) {
        nextLanguage = QStringLiteral("zh-CN");
    }
    if (_reply_language == nextLanguage) {
        return;
    }
    _reply_language = nextLanguage;
    emit replyLanguageChanged();
}

void PetController::setSpeechRules(const QString &rules)
{
    const QString nextRules = rules.trimmed();
    if (_speech_rules == nextRules) {
        return;
    }
    _speech_rules = nextRules;
    emit speechRulesChanged();
}

void PetController::setVoiceRuntimeSettings(const QVariantMap &settings)
{
    QString referenceAudioPath = localPathFromUrlText(firstStringFromVariant(settings,
                                                                            {"referenceAudioPath",
                                                                             "reference_audio_path",
                                                                             "refAudioPath",
                                                                             "ref_audio_path",
                                                                             "voiceTrainingReferenceAudioPath"}));
    if (referenceAudioPath.isEmpty()) {
        referenceAudioPath = joinedPath(firstStringFromVariant(settings, {"voiceDirectory", "referenceAudioDirectory"}),
                                        firstStringFromVariant(settings, {"defaultVoice", "referenceAudioFile"}));
    }
    if (referenceAudioPath.isEmpty()) {
        const QString artifactPath = localPathFromUrlText(firstStringFromVariant(settings,
                                                                                 {"voiceTrainingArtifactPath",
                                                                                  "artifactPath"}));
        if (looksLikeAudioPath(artifactPath)) {
            referenceAudioPath = artifactPath;
        }
    }
    QString referenceSource = firstStringFromVariant(settings, {"referenceAudioSource", "reference_audio_source"});
    if (_voice_reference_audio_source == QStringLiteral("voice_training")
        && referenceSource != QStringLiteral("voice_training")
        && !_voice_reference_audio_path.trimmed().isEmpty()) {
        referenceAudioPath = _voice_reference_audio_path.trimmed();
        referenceSource = _voice_reference_audio_source;
    }

    QString provider = firstStringFromVariant(settings, {"voiceProvider", "provider", "voice_provider"});
    if (provider.isEmpty() && !referenceAudioPath.isEmpty()) {
        provider = QStringLiteral("gpt-sovits");
    }

    QString voiceName = firstStringFromVariant(settings, {"voiceName", "voice_name", "characterName"});
    if (voiceName.isEmpty()) {
        const QString defaultVoice = firstStringFromVariant(settings, {"defaultVoice", "referenceAudioFile"});
        if (!defaultVoice.isEmpty()) {
            voiceName = QFileInfo(defaultVoice).completeBaseName();
        }
    }
    if (voiceName.isEmpty() && !provider.isEmpty()) {
        voiceName = provider;
    }

    QString language = firstStringFromVariant(settings, {"voiceLanguage", "voice_language", "language"});
    if (language.isEmpty()) {
        language = _reply_language.trimmed().isEmpty() ? QStringLiteral("zh-CN") : _reply_language.trimmed();
    }
    const QString textLanguage = firstStringFromVariant(settings, {"textLanguage", "text_lang"});
    const QString promptLanguage = firstStringFromVariant(settings, {"promptLanguage", "prompt_lang"});
    const QString promptText = firstStringFromVariant(settings, {"promptText", "prompt_text"});

    const bool changed = _voice_provider != provider
                         || _voice_name != voiceName
                         || _voice_language != language
                         || _voice_reference_audio_path != referenceAudioPath
                         || _voice_reference_audio_source != referenceSource
                         || _voice_prompt_text != promptText
                         || _voice_prompt_language != promptLanguage
                         || _voice_text_language != textLanguage;
    if (!changed) {
        return;
    }
    _voice_provider = provider;
    _voice_name = voiceName;
    _voice_language = language;
    _voice_reference_audio_path = referenceAudioPath;
    _voice_reference_audio_source = referenceSource;
    _voice_prompt_text = promptText;
    _voice_prompt_language = promptLanguage;
    _voice_text_language = textLanguage;
    emit stateChanged();
}

void PetController::refreshVoiceTrainingJob(const QString &jobId)
{
    const QString selectedJobId = jobId.trimmed().isEmpty() ? _voice_training_job_id : jobId.trimmed();
    if (selectedJobId.isEmpty() || _voice_training_busy) {
        return;
    }
    setError({});
    setVoiceTrainingBusy(true, QStringLiteral("正在刷新声音训练任务"));
    getJson(petUrl(QStringLiteral("/voice-training/jobs/%1").arg(QString::fromLatin1(QUrl::toPercentEncoding(selectedJobId)))),
            QStringLiteral("voice_training_get"));
}

void PetController::postJson(const QUrl &url, const QJsonObject &payload, const QString &op)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    configurePetRequest(request);
    auto *reply = _network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("pet_op", op);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleJsonReply(reply);
    });
}

void PetController::getJson(const QUrl &url, const QString &op)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    configurePetRequest(request);
    auto *reply = _network.get(request);
    reply->setProperty("pet_op", op);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleJsonReply(reply);
    });
}

void PetController::startStream()
{
    if (_session_id.isEmpty() || _streaming) {
        return;
    }

    QNetworkRequest request(petUrl(QStringLiteral("/sessions/%1/stream").arg(encodedSessionId())));
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Cache-Control", "no-cache");
    configurePetRequest(request);
    _stream_reply = _stream_network.get(request);
    _streaming = true;
    _stream_buffer.clear();
    setStatusText(QStringLiteral("桌宠事件流已连接"));
    emit stateChanged();

    connect(_stream_reply.data(), &QNetworkReply::readyRead, this, [this]() {
        if (_stream_reply) {
            consumeStreamBytes(_stream_reply->readAll());
        }
    });
    connect(_stream_reply.data(), &QNetworkReply::finished, this, [this]() {
        if (!_stream_reply) {
            return;
        }
        const auto err = _stream_reply->error();
        const QString errText = _stream_reply->errorString();
        _stream_reply->deleteLater();
        _stream_reply.clear();
        _streaming = false;
        if (err != QNetworkReply::NoError && err != QNetworkReply::OperationCanceledError) {
            setError(QStringLiteral("桌宠事件流断开: %1").arg(errText));
        } else {
            setStatusText(QStringLiteral("桌宠事件流已关闭"));
        }
        emit stateChanged();
    });
}

void PetController::handleJsonReply(QNetworkReply *reply)
{
    const QString op = reply->property("pet_op").toString();
    const bool visionOp = op == QStringLiteral("vision_capture") || op == QStringLiteral("vision_segment_capture");
    const auto networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray bytes = reply->readAll();
    reply->deleteLater();

    if (op == QStringLiteral("create_session")) {
        setBusy(false);
    } else if (op == QStringLiteral("input")) {
        _input_request_in_flight = false;
        setBusy(false);
    } else if (op == QStringLiteral("voice_training_create")
               || op == QStringLiteral("voice_training_get")) {
        setVoiceTrainingBusy(false);
    }

    if (networkError != QNetworkReply::NoError) {
        if (visionOp) {
            setVisionRequestInFlight(false);
        }
        setError(QStringLiteral("桌宠请求失败: %1").arg(networkErrorText));
        return;
    }
    if (httpStatus >= 400) {
        if (visionOp) {
            setVisionRequestInFlight(false);
        }
        setError(QStringLiteral("桌宠 HTTP %1").arg(httpStatus));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) {
        if (visionOp) {
            setVisionRequestInFlight(false);
        }
        setError(QStringLiteral("桌宠响应格式错误"));
        return;
    }
    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("code")).toInt(0) != 0) {
        if (visionOp) {
            setVisionRequestInFlight(false);
        }
        setError(root.value(QStringLiteral("message")).toString(QStringLiteral("桌宠请求失败")));
        return;
    }

    if (op == QStringLiteral("create_session")) {
        const QJsonObject session = root.value(QStringLiteral("session")).toObject();
        const QString nextSessionId = session.value(QStringLiteral("session_id")).toString();
        if (nextSessionId.isEmpty()) {
            setError(QStringLiteral("桌宠会话缺少 session_id"));
            return;
        }
        if (_session_id != nextSessionId) {
            resetControlEventDedupe();
        }
        _session_id = nextSessionId;
        setStatusText(QStringLiteral("桌宠会话已创建"));
        emit stateChanged();
        startStream();
        return;
    }

    if (op == QStringLiteral("voice_training_create")
        || op == QStringLiteral("voice_training_get")) {
        applyVoiceTrainingJob(root.value(QStringLiteral("job")).toObject());
        return;
    }

    if (visionOp) {
        setVisionRequestInFlight(false);
    }

    const QJsonArray events = root.value(QStringLiteral("events")).toArray();
    for (const QJsonValue &value : events) {
        applyControlEvent(value.toObject());
    }
    const QJsonObject event = root.value(QStringLiteral("event")).toObject();
    if (!event.isEmpty()) {
        applyControlEvent(event);
    }

    if (op == QStringLiteral("vision_segment_capture")) {
        setStatusText(QStringLiteral("视觉片段已分析"));
    } else if (op == QStringLiteral("vision_capture")) {
        setStatusText(QStringLiteral("摄像头帧已分析"));
    }
}

void PetController::applyControlEvent(const QJsonObject &event)
{
    if (event.value(QStringLiteral("type")).toString() != QStringLiteral("pet.control")) {
        return;
    }
    if (!rememberControlEvent(event)) {
        return;
    }
    _model.applyControlEvent(event);
    emit controlEventReceived(event.toVariantMap());
}

bool PetController::rememberControlEvent(const QJsonObject &event)
{
    const QString sessionId = event.value(QStringLiteral("session_id")).toString(_session_id).trimmed();
    const QString eventId = event.value(QStringLiteral("event_id")).toString().trimmed();
    const QString turnId = event.value(QStringLiteral("turn_id")).toString().trimmed();
    const QString phase = event.value(QStringLiteral("phase")).toString().trimmed();
    const QString actionName = event.value(QStringLiteral("action")).toObject().value(QStringLiteral("name")).toString().trimmed();
    const QString textDelta = event.value(QStringLiteral("text")).toObject().value(QStringLiteral("delta")).toString();
    const QString audioUrl = event.value(QStringLiteral("audio")).toObject().value(QStringLiteral("url")).toString();
    QString key;
    if (!eventId.isEmpty()) {
        key = sessionId + QStringLiteral(":event:") + eventId;
    } else if (!turnId.isEmpty()) {
        key = sessionId + QStringLiteral(":turn:")
            + turnId + QStringLiteral(":") + phase + QStringLiteral(":")
            + actionName + QStringLiteral(":") + textDelta + QStringLiteral(":") + audioUrl;
    } else {
        const int sequence = event.value(QStringLiteral("seq")).toInt(-1);
        if (sequence < 0) {
            return true;
        }
        key = sessionId + QStringLiteral(":seq:") + QString::number(sequence);
    }

    if (_applied_control_event_keys.contains(key)) {
        return false;
    }
    _applied_control_event_keys.insert(key);
    _applied_control_event_order.append(key);
    while (_applied_control_event_order.size() > kMaxRememberedPetControlEvents) {
        _applied_control_event_keys.remove(_applied_control_event_order.takeFirst());
    }
    return true;
}

void PetController::resetControlEventDedupe()
{
    _applied_control_event_keys.clear();
    _applied_control_event_order.clear();
}

void PetController::consumeStreamBytes(const QByteArray &bytes)
{
    _stream_buffer += bytes;
    for (;;) {
        const int newline = _stream_buffer.indexOf('\n');
        if (newline < 0) {
            return;
        }
        QByteArray line = _stream_buffer.left(newline);
        _stream_buffer.remove(0, newline + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        consumeSseLine(line);
    }
}

void PetController::consumeSseLine(const QByteArray &line)
{
    if (!line.startsWith("data: ")) {
        return;
    }
    const QByteArray payload = line.mid(6);
    if (payload == "[DONE]") {
        stopStream();
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        return;
    }
    applyControlEvent(doc.object());
}

void PetController::setBusy(bool busy, const QString &statusText)
{
    const bool changed = _busy != busy || (!statusText.isNull() && _status_text != statusText);
    _busy = busy;
    if (!statusText.isNull()) {
        _status_text = statusText;
    }
    if (changed) {
        emit stateChanged();
    }
}

void PetController::setVoiceTrainingBusy(bool busy, const QString &message)
{
    const bool changed = _voice_training_busy != busy || (!message.isNull() && _voice_training_message != message);
    _voice_training_busy = busy;
    if (!message.isNull()) {
        _voice_training_message = message;
    }
    if (changed) {
        emit voiceTrainingChanged();
    }
}

void PetController::setVisionRequestInFlight(bool inFlight)
{
    if (_vision_request_in_flight == inFlight) {
        return;
    }
    _vision_request_in_flight = inFlight;
    emit stateChanged();
}

void PetController::applyVoiceTrainingJob(const QJsonObject &job)
{
    if (job.isEmpty()) {
        setError(QStringLiteral("声音训练响应缺少任务信息"));
        return;
    }
    _voice_training_job_id = job.value(QStringLiteral("job_id")).toString();
    _voice_training_status = job.value(QStringLiteral("status")).toString(QStringLiteral("prepared"));
    _voice_training_stage = job.value(QStringLiteral("stage")).toString();
    _voice_training_progress = qBound(0, job.value(QStringLiteral("progress")).toInt(0), 100);
    _voice_training_artifact_path = job.value(QStringLiteral("artifact_path")).toString();
    _voice_training_message = job.value(QStringLiteral("message")).toString(QStringLiteral("声音训练任务已准备"));
    const QJsonObject diagnostics = job.value(QStringLiteral("diagnostics")).toObject();
    const QString trainedReferenceAudio = diagnostics.value(QStringLiteral("gpt_sovits_reference_audio")).toString().trimmed().isEmpty()
                                              ? diagnostics.value(QStringLiteral("reference_audio_runtime_path")).toString().trimmed()
                                              : diagnostics.value(QStringLiteral("gpt_sovits_reference_audio")).toString().trimmed();
    if (!trainedReferenceAudio.isEmpty()) {
        _voice_reference_audio_path = trainedReferenceAudio;
        _voice_reference_audio_source = QStringLiteral("voice_training");
        _voice_provider = QStringLiteral("gpt-sovits");
        const QString trainedVoiceName = job.value(QStringLiteral("voice_name")).toString().trimmed();
        if (!trainedVoiceName.isEmpty()) {
            _voice_name = trainedVoiceName;
        }
        const QString trainedLanguage = job.value(QStringLiteral("language")).toString().trimmed();
        if (!trainedLanguage.isEmpty()) {
            _voice_language = trainedLanguage;
            if (_voice_text_language.isEmpty()) {
                _voice_text_language = voiceTextLanguage(trainedLanguage);
            }
        }
    }
    if (_voice_training_stage == QStringLiteral("ready_for_worker")) {
        _voice_training_stage = QStringLiteral("ready_for_gpt_sovits");
        if (_voice_training_status == QStringLiteral("prepared")
            || _voice_training_status == QStringLiteral("idle")
            || _voice_training_status == QStringLiteral("blocked")) {
            _voice_training_status = QStringLiteral("ready");
        }
        if (_voice_training_progress < 70) {
            _voice_training_progress = 70;
        }
        if (_voice_training_message.isEmpty()
            || _voice_training_message.contains(QStringLiteral("worker"), Qt::CaseInsensitive)) {
            _voice_training_message = QStringLiteral("声音参考已就绪，可直接用于 GPT-SoVITS 零样本合成。");
        }
    }
    setStatusText(_voice_training_message);
    emit voiceTrainingChanged();
}

void PetController::setWindowsImeBridgeBusy(bool busy)
{
    if (_windows_ime_bridge_busy == busy) {
        return;
    }
    _windows_ime_bridge_busy = busy;
    emit windowsImeBridgeChanged();
}

void PetController::setWindowsCameraBridgeBusy(bool busy)
{
    if (_windows_camera_bridge_busy == busy) {
        return;
    }
    _windows_camera_bridge_busy = busy;
    emit windowsCameraBridgeChanged();
}

void PetController::setStatusText(const QString &statusText)
{
    if (_status_text == statusText) {
        return;
    }
    _status_text = statusText;
    emit stateChanged();
}

void PetController::setError(const QString &error)
{
    if (_error == error) {
        return;
    }
    _error = error;
    emit stateChanged();
}

QJsonObject PetController::authPayload() const
{
    QJsonObject payload;
    if (_gateway && _gateway->userMgr()) {
        payload[QStringLiteral("uid")] = _gateway->userMgr()->GetUid();
    }
    return payload;
}

QJsonObject PetController::defaultObservationPayload() const
{
    QJsonObject audio;
    audio[QStringLiteral("vad")] = QStringLiteral("idle");
    audio[QStringLiteral("rms")] = 0.0;
    audio[QStringLiteral("interrupt")] = false;

    QJsonObject vision;
    vision[QStringLiteral("enabled")] = false;
    vision[QStringLiteral("mode")] = QStringLiteral("landmarks_only");
    vision[QStringLiteral("face_present")] = false;
    appendVoiceRuntimeMetadata(vision);

    QJsonObject privacy;
    privacy[QStringLiteral("raw_frame_sent")] = false;
    privacy[QStringLiteral("raw_audio_recorded")] = false;

    QJsonObject payload;
    payload[QStringLiteral("type")] = QStringLiteral("pet.observation");
    payload[QStringLiteral("audio")] = audio;
    payload[QStringLiteral("vision")] = vision;
    payload[QStringLiteral("privacy")] = privacy;
    return payload;
}

QJsonObject PetController::voiceRuntimeMetadata() const
{
    QJsonObject metadata;
    const QString provider = _voice_provider.trimmed();
    const QString referenceAudioPath = _voice_reference_audio_path.trimmed();
    if (provider.isEmpty() && referenceAudioPath.isEmpty()) {
        return metadata;
    }
    if (!provider.isEmpty()) {
        metadata[QStringLiteral("voice_provider")] = provider;
    }
    if (!_voice_name.trimmed().isEmpty()) {
        metadata[QStringLiteral("voice_name")] = _voice_name.trimmed();
    }
    const QString voiceLanguage = _voice_language.trimmed().isEmpty()
                                      ? (_reply_language.trimmed().isEmpty() ? QStringLiteral("zh-CN") : _reply_language.trimmed())
                                      : _voice_language.trimmed();
    if (!voiceLanguage.isEmpty()) {
        metadata[QStringLiteral("voice_language")] = voiceLanguage;
    }
    if (!referenceAudioPath.isEmpty()) {
        metadata[QStringLiteral("ref_audio_path")] = referenceAudioPath;
    }
    if (!_voice_reference_audio_source.trimmed().isEmpty()) {
        metadata[QStringLiteral("reference_audio_source")] = _voice_reference_audio_source.trimmed();
    }
    if (!_voice_prompt_text.trimmed().isEmpty()) {
        metadata[QStringLiteral("prompt_text")] = _voice_prompt_text.trimmed();
    }
    if (!_voice_prompt_language.trimmed().isEmpty()) {
        metadata[QStringLiteral("prompt_lang")] = _voice_prompt_language.trimmed();
    }
    metadata[QStringLiteral("text_lang")] = _voice_text_language.trimmed().isEmpty()
                                                ? voiceTextLanguage(voiceLanguage)
                                                : _voice_text_language.trimmed();
    return metadata;
}

void PetController::appendVoiceRuntimeMetadata(QJsonObject &metadata) const
{
    const QJsonObject voiceMetadata = voiceRuntimeMetadata();
    for (auto it = voiceMetadata.constBegin(); it != voiceMetadata.constEnd(); ++it) {
        metadata.insert(it.key(), it.value());
    }
}

QUrl PetController::petUrl(const QString &path) const
{
    return QUrl(gate_url_prefix.trimmed() + QStringLiteral("/ai/pet") + path);
}

QString PetController::encodedSessionId() const
{
    return QString::fromLatin1(QUrl::toPercentEncoding(_session_id));
}
