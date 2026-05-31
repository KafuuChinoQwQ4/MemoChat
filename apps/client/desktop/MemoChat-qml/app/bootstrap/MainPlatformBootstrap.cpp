#include "MainPlatformBootstrap.h"

#include <QDir>
#include <QFile>
#include <QLibraryInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QtGlobal>

void setDefaultEnv(const char* name, const char* value)
{
    if (qEnvironmentVariableIsEmpty(name))
    {
        qputenv(name, value);
    }
}

#ifdef Q_OS_LINUX
bool isWslEnvironment()
{
    QFile osrelease(QStringLiteral("/proc/sys/kernel/osrelease"));
    if (!osrelease.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return !qEnvironmentVariableIsEmpty("WSL_DISTRO_NAME");
    }
    const QByteArray release = osrelease.readAll().toLower();
    return release.contains("microsoft") || release.contains("wsl");
}

void configureLinuxRendering()
{
    setDefaultEnv("QSG_RENDER_LOOP", "threaded");
    setDefaultEnv("QSG_RHI_BACKEND", "opengl");
    setDefaultEnv("QT_OPENGL", "desktop");
    setDefaultEnv("__GL_THREADED_OPTIMIZATIONS", "1");
    const QByteArray qmlCacheDisable = qgetenv("QML_DISABLE_DISK_CACHE").trimmed().toLower();
    if (qmlCacheDisable == "0" || qmlCacheDisable == "false" || qmlCacheDisable == "no")
    {
        qunsetenv("QML_DISABLE_DISK_CACHE");
    }
    if (isWslEnvironment())
    {
        setDefaultEnv("QT_QPA_PLATFORM", "xcb");
        setDefaultEnv("GALLIUM_DRIVER", "d3d12");
        setDefaultEnv("MESA_LOADER_DRIVER_OVERRIDE", "d3d12");
        setDefaultEnv("LIBGL_ALWAYS_SOFTWARE", "0");
    }
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
}

void startIbusDaemon(bool replace)
{
    QStringList args{QStringLiteral("-drx")};
    if (replace)
    {
        args << QStringLiteral("--replace");
    }
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove(QStringLiteral("WAYLAND_DISPLAY"));
    env.insert(QStringLiteral("GDK_BACKEND"), QStringLiteral("x11"));
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("xcb"));
    process.setProcessEnvironment(env);
    process.setProgram(QStringLiteral("ibus-daemon"));
    process.setArguments(args);
    process.startDetached();
}

bool selectIbusLibpinyinEngine()
{
    if (QStandardPaths::findExecutable(QStringLiteral("ibus")).isEmpty())
    {
        return false;
    }
    for (int attempt = 0; attempt < 10; ++attempt)
    {
        QProcess process;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove(QStringLiteral("WAYLAND_DISPLAY"));
        process.setProcessEnvironment(env);
        process.setProgram(QStringLiteral("ibus"));
        process.setArguments({QStringLiteral("engine"), QStringLiteral("libpinyin")});
        process.start();
        if (process.waitForFinished(3000) && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
        {
            return true;
        }
        QThread::msleep(100);
    }
    return false;
}

bool shouldRestartIbusDaemon()
{
    const QByteArray value = qgetenv("MEMOCHAT_RESTART_IBUS").trimmed().toLower();
    return value.isEmpty() || value == "1" || value == "true" || value == "yes";
}

void stopIbusDaemons()
{
    if (!QStandardPaths::findExecutable(QStringLiteral("ibus")).isEmpty())
    {
        QProcess process;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove(QStringLiteral("WAYLAND_DISPLAY"));
        process.setProcessEnvironment(env);
        process.setProgram(QStringLiteral("ibus"));
        process.setArguments({QStringLiteral("exit")});
        process.start();
        process.waitForFinished(3000);
    }
    if (!QStandardPaths::findExecutable(QStringLiteral("pkill")).isEmpty())
    {
        QProcess::execute(QStringLiteral("pkill"), {QStringLiteral("-x"), QStringLiteral("ibus-daemon")});
    }
    QThread::msleep(200);
}

void configureLinuxInputMethod()
{
    const bool keep_existing = qEnvironmentVariableIntValue("MEMOCHAT_KEEP_QT_IM_MODULE") == 1;
    if (keep_existing && !qEnvironmentVariableIsEmpty("QT_IM_MODULE"))
    {
        return;
    }

    const QString plugin_path = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    const auto hasPlatformInputContext = [&plugin_path](const QString& pluginName)
    {
        return !plugin_path.isEmpty() &&
               QFile::exists(QDir(plugin_path).filePath(QStringLiteral("platforminputcontexts/%1").arg(pluginName)));
    };
    const bool has_fcitx_plugin = hasPlatformInputContext(QStringLiteral("libfcitx5platforminputcontextplugin.so"));
    const bool has_ibus_plugin = hasPlatformInputContext(QStringLiteral("libibusplatforminputcontextplugin.so"));
    const bool has_pgrep = !QStandardPaths::findExecutable(QStringLiteral("pgrep")).isEmpty();
    const bool has_fcitx = !QStandardPaths::findExecutable(QStringLiteral("fcitx5")).isEmpty();
    const bool has_ibus = !QStandardPaths::findExecutable(QStringLiteral("ibus-daemon")).isEmpty();

    if (has_fcitx && has_fcitx_plugin)
    {
        if (!has_pgrep ||
            QProcess::execute(QStringLiteral("pgrep"), {QStringLiteral("-x"), QStringLiteral("fcitx5")}) != 0)
        {
            QProcess process;
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert(QStringLiteral("WAYLAND_DISPLAY"), QString());
            process.setProcessEnvironment(env);
            process.setProgram(QStringLiteral("fcitx5"));
            process.setArguments({QStringLiteral("-d")});
            process.startDetached();
        }
        qputenv("QT_IM_MODULE", "fcitx");
        qputenv("GTK_IM_MODULE", "fcitx");
        qputenv("XMODIFIERS", "@im=fcitx");
        return;
    }

    if (has_ibus && has_ibus_plugin)
    {
        setDefaultEnv("IBUS_ENABLE_SYNC_MODE", "1");
        qunsetenv("WAYLAND_DISPLAY");
        if (shouldRestartIbusDaemon())
        {
            stopIbusDaemons();
            startIbusDaemon(false);
        }
        else
        {
            const bool ibus_running =
                has_pgrep &&
                QProcess::execute(QStringLiteral("pgrep"), {QStringLiteral("-x"), QStringLiteral("ibus-daemon")}) == 0;
            startIbusDaemon(ibus_running);
        }
        QThread::msleep(150);
        if (!selectIbusLibpinyinEngine())
        {
            startIbusDaemon(true);
            QThread::msleep(150);
            selectIbusLibpinyinEngine();
        }
        qputenv("QT_IM_MODULE", "ibus");
        qputenv("GTK_IM_MODULE", "ibus");
        qputenv("XMODIFIERS", "@im=ibus");
        return;
    }

    if (!plugin_path.isEmpty() &&
        QFile::exists(
            QDir(plugin_path).filePath(QStringLiteral("platforminputcontexts/libqtvirtualkeyboardplugin.so"))))
    {
        qputenv("QT_IM_MODULE", "qtvirtualkeyboard");
        qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", "0");
    }
}
#endif
