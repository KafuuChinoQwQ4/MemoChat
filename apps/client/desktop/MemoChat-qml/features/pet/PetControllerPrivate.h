#ifndef PETCONTROLLERPRIVATE_H
#define PETCONTROLLERPRIVATE_H

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslSocket>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include <initializer_list>

namespace memochat::pet {
constexpr int kMaxRememberedPetControlEvents = 256;
constexpr int kVisionSegmentMaxFrames = 14;
constexpr int kVisionSegmentWindowMs = 20000;
constexpr int kVisionSegmentMinUploadMs = 15000;
constexpr int kVisionDuplicateFrameCooldownMs = 30000;
constexpr int kVisionFrameSignatureSize = 16;
constexpr int kVisionFrameSignatureDuplicateDistance = 32;

inline QByteArray visionFrameSignature(const QImage &image)
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

inline void configurePetRequest(QNetworkRequest &request)
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

inline int visionFrameSignatureDistance(const QByteArray &left, const QByteArray &right)
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

inline QString stringFromVariant(const QVariantMap &settings, const QString &key)
{
    return settings.value(key).toString().trimmed();
}

inline QString firstStringFromVariant(const QVariantMap &settings, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QString value = stringFromVariant(settings, QString::fromLatin1(key));
        if (!value.isEmpty()) {
            return value;
        }
    }
    return {};
}

inline QString localPathFromUrlText(const QString &value)
{
    QString text = value.trimmed();
    if (text.startsWith(QStringLiteral("file://"))) {
        text = QUrl(text).toLocalFile();
    }
    return text.trimmed();
}

inline QString joinedPath(const QString &directory, const QString &fileName)
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

inline bool looksLikeAudioPath(const QString &path)
{
    const QString suffix = QFileInfo(path.trimmed()).suffix().toLower();
    return suffix == QStringLiteral("wav")
           || suffix == QStringLiteral("mp3")
           || suffix == QStringLiteral("ogg")
           || suffix == QStringLiteral("flac")
           || suffix == QStringLiteral("m4a")
           || suffix == QStringLiteral("aac");
}

inline QString voiceTextLanguage(const QString &language)
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

inline bool isWslRuntime()
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

inline QString windowsPowerShellPath()
{
    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
    if (!fromPath.isEmpty()) {
        return fromPath;
    }
    const QString fallback = QStringLiteral("/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe");
    return QFileInfo::exists(fallback) ? fallback : QString();
}

inline QString powershellEncodedCommand(const QString &script)
{
    const auto *utf16 = reinterpret_cast<const char *>(script.utf16());
    const QByteArray bytes(utf16, script.size() * int(sizeof(ushort)));
    return QString::fromLatin1(bytes.toBase64());
}

inline QString windowsImeBridgeScript(const QString &initialText)
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

inline void normalizeVisionSegmentFrames(QJsonArray &frames)
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

inline QString windowsCameraCaptureScript()
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

#endif // PETCONTROLLERPRIVATE_H