#ifndef PETWINDOWSBRIDGEUTILS_H
#define PETWINDOWSBRIDGEUTILS_H

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>

namespace memochat::pet
{

inline bool isWslRuntime()
{
#ifdef Q_OS_LINUX
    QFile file(QStringLiteral("/proc/sys/kernel/osrelease"));
    if (file.open(QIODevice::ReadOnly))
    {
        const QString release = QString::fromLatin1(file.readAll()).toLower();
        return release.contains(QStringLiteral("microsoft")) || release.contains(QStringLiteral("wsl"));
    }
#endif
    return false;
}

inline QString windowsPowerShellPath()
{
    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
    if (!fromPath.isEmpty())
    {
        return fromPath;
    }
    const QString fallback = QStringLiteral("/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe");
    return QFileInfo::exists(fallback) ? fallback : QString();
}

inline QString powershellEncodedCommand(const QString& script)
{
    const auto* utf16 = reinterpret_cast<const char*>(script.utf16());
    const QByteArray bytes(utf16, script.size() * int(sizeof(ushort)));
    return QString::fromLatin1(bytes.toBase64());
}

inline QString windowsImeBridgeScript(const QString& initialText)
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
)PS")
        .arg(initialBase64);
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

} // namespace memochat::pet

#endif // PETWINDOWSBRIDGEUTILS_H
