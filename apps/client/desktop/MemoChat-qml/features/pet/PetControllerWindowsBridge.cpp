#include "PetController.h"
#include "PetControllerPrivate.h"

#include <QtCore/QProcess>
#include <QtCore/QtGlobal>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>

using namespace memochat::pet;

bool PetController::windowsImeBridgeAvailable() const
{
    return isWslRuntime() && !windowsPowerShellPath().isEmpty();
}

bool PetController::windowsCameraBridgeAvailable() const
{
    return isWslRuntime() && !windowsPowerShellPath().isEmpty();
}

void PetController::openWindowsImeBridge(const QString& initialText)
{
    if (_windows_ime_bridge_busy)
    {
        return;
    }
    const QString program = windowsPowerShellPath();
    if (!isWslRuntime() || program.isEmpty())
    {
        setError(QStringLiteral("Windows 中文输入桥只在 WSL 中可用"));
        return;
    }

    auto* process = new QProcess(this);
    _windows_ime_bridge_process = process;
    setWindowsImeBridgeBusy(true);

    connect(process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this, process](int exitCode, QProcess::ExitStatus exitStatus)
            {
                if (_windows_ime_bridge_process == process)
                {
                    _windows_ime_bridge_process.clear();
                }
                const QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
                const QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
                process->deleteLater();
                setWindowsImeBridgeBusy(false);
                if (exitStatus != QProcess::NormalExit || exitCode != 0)
                {
                    setError(error.isEmpty() ? QStringLiteral("Windows 中文输入桥已退出")
                                             : QStringLiteral("Windows 中文输入桥失败: %1").arg(error));
                    return;
                }
                if (!output.isEmpty())
                {
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
    if (!process->waitForStarted(3000))
    {
        if (_windows_ime_bridge_process == process)
        {
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
    if (_session_id.isEmpty())
    {
        setError(QStringLiteral("桌宠会话未连接"));
        return false;
    }
    if (_windows_camera_bridge_busy)
    {
        setStatusText(QStringLiteral("Windows 摄像头桥正在捕捉"));
        return false;
    }

    const QString program = windowsPowerShellPath();
    if (!isWslRuntime() || program.isEmpty())
    {
        setError(QStringLiteral("Windows 摄像头桥只在 WSL 中可用"));
        return false;
    }

    auto* process = new QProcess(this);
    _windows_camera_bridge_process = process;
    setWindowsCameraBridgeBusy(true);
    setStatusText(QStringLiteral("Windows 摄像头桥正在捕捉"));

    connect(process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this, process](int exitCode, QProcess::ExitStatus exitStatus)
            {
                if (_windows_camera_bridge_process == process)
                {
                    _windows_camera_bridge_process.clear();
                }
                const QByteArray stdoutBytes = process->readAllStandardOutput();
                const QString output = QString::fromUtf8(stdoutBytes).trimmed();
                QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
                if (error.isEmpty() && !output.startsWith(QLatin1Char('{')))
                {
                    error = output;
                }
                process->deleteLater();
                setWindowsCameraBridgeBusy(false);

                if (exitStatus != QProcess::NormalExit || exitCode != 0)
                {
                    setError(error.isEmpty() ? QStringLiteral("Windows 摄像头桥已退出")
                                             : QStringLiteral("Windows 摄像头桥失败: %1").arg(error));
                    return;
                }

                const int jsonStart = output.indexOf(QLatin1Char('{'));
                const int jsonEnd = output.lastIndexOf(QLatin1Char('}'));
                if (jsonStart < 0 || jsonEnd < jsonStart)
                {
                    setError(QStringLiteral("Windows 摄像头桥响应格式错误"));
                    return;
                }

                QJsonParseError parseError;
                const QByteArray jsonBytes = output.mid(jsonStart, jsonEnd - jsonStart + 1).toUtf8();
                const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isObject())
                {
                    setError(QStringLiteral("Windows 摄像头桥响应格式错误"));
                    return;
                }

                const QJsonObject payload = doc.object();
                const QString frameBase64 = payload.value(QStringLiteral("frame_base64")).toString().trimmed();
                if (frameBase64.isEmpty())
                {
                    setError(QStringLiteral("Windows 摄像头桥未返回画面"));
                    return;
                }

                postVisionCapture(
                    frameBase64,
                    payload.value(
                        QStringLiteral("frame_mime"))
                            .toString(
                                QStringLiteral("image/jpeg")),
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
                                                                                    powershellEncodedCommand(
                                                                                        windowsCameraCaptureScript()),
                                                      });
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->start();
    if (!process->waitForStarted(3000))
    {
        if (_windows_camera_bridge_process == process)
        {
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
