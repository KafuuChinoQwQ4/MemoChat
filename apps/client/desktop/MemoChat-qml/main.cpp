#include "ClientWinCompat.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTimer>
#include <QQuickStyle>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QProcess>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QSysInfo>
#include <cstdio>
#include "AppController.h"
#include "CallSessionModel.h"
#include "Live2DRenderItem.h"
#include "PetAssetSettings.h"
#include "PetController.h"
#include "PetModel.h"
#include "PetSpeechSynthesizer.h"
#include "Live2DAsset.h"
#include "TelemetryUtils.h"
#include "global.h"
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

#ifdef Q_OS_WIN
// DWM functions are declared in ClientWinCompat.h, no need to re-include

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMSBT_TRANSIENTWINDOW
#define DWMSBT_TRANSIENTWINDOW 3
#endif

#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

enum class WindowCompositionAttribute : int {
    WCA_ACCENT_POLICY = 19
};

enum class AccentState : int {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};

struct AccentPolicy {
    int accent_state;
    int accent_flags;
    int gradient_color;
    int animation_id;
};

struct WindowCompositionAttributeData {
    WindowCompositionAttribute attribute;
    PVOID data;
    SIZE_T size_of_data;
};

using SetWindowCompositionAttributeFn =
    BOOL(WINAPI *)(HWND, WindowCompositionAttributeData *);

int interpolateChannel(int from, int to, qreal progress)
{
    const qreal clamped = std::clamp(progress, 0.0, 1.0);
    return qRound(from + (to - from) * clamped);
}

int makeAbgrColor(int alpha, int red, int green, int blue)
{
    return ((alpha & 0xff) << 24)
           | ((blue & 0xff) << 16)
           | ((green & 0xff) << 8)
           | (red & 0xff);
}

qreal acrylicPinkProgressForWindow(QQuickWindow *window)
{
    if (!window) {
        return 0.0;
    }
    return std::clamp(window->property("acrylicPinkProgress").toDouble(), 0.0, 1.0);
}

int acrylicGradientColorForWindow(QQuickWindow *window)
{
    const qreal progress = acrylicPinkProgressForWindow(window);
    return makeAbgrColor(interpolateChannel(0x15, 0x24, progress),
                         interpolateChannel(0xF5, 0xFF, progress),
                         interpolateChannel(0xEE, 0xD8, progress),
                         interpolateChannel(0xFF, 0xE8, progress));
}

void applyWindowsAcrylic(QQuickWindow *window)
{
    if (!window) {
        return;
    }

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        return;
    }

    const int corner_pref = DWMWCP_ROUND;
    DwmSetWindowAttribute(
        hwnd,
        DWMWA_WINDOW_CORNER_PREFERENCE,
        &corner_pref,
        sizeof(corner_pref));

    const bool has_custom_tint = window->property("acrylicPinkProgress").isValid();
    if (!has_custom_tint) {
        // Preferred path on newer Windows builds: system backdrop type (Acrylic-like transient window).
        const int backdrop_type = DWMSBT_TRANSIENTWINDOW;
        HRESULT hr = DwmSetWindowAttribute(
            hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
        if (SUCCEEDED(hr)) {
            return;
        }
    }

    // Fallback path: Windows 10 Acrylic blur via undocumented composition attribute.
    const HMODULE user32_module = GetModuleHandleW(L"user32.dll");
    if (!user32_module) {
        return;
    }

    auto set_wca = reinterpret_cast<SetWindowCompositionAttributeFn>(
        GetProcAddress(user32_module, "SetWindowCompositionAttribute"));
    if (!set_wca) {
        return;
    }

    AccentPolicy policy{};
    policy.accent_state = static_cast<int>(AccentState::ACCENT_ENABLE_ACRYLICBLURBEHIND);
    policy.accent_flags = 2;
    // ABGR (AABBGGRR): interpolate between the default cool tint and the R18 pink tint.
    policy.gradient_color = acrylicGradientColorForWindow(window);
    policy.animation_id = 0;

    WindowCompositionAttributeData data{};
    data.attribute = WindowCompositionAttribute::WCA_ACCENT_POLICY;
    data.data = &policy;
    data.size_of_data = sizeof(policy);
    set_wca(hwnd, &data);
}

void ensureAcrylicVisibleHook(QQuickWindow *window)
{
    if (!window || window->property("_memochat_acrylic_hooked").toBool()) {
        return;
    }
    window->setProperty("_memochat_acrylic_hooked", true);
    QObject::connect(window,
                     &QWindow::visibleChanged,
                     window,
                     [window](bool visible) {
                         if (!visible) {
                             return;
                         }
                          QTimer::singleShot(0, window, [window]() {
                              applyWindowsAcrylic(window);
                          });
                      });

    const int acrylic_prop_index = window->metaObject()->indexOfProperty("acrylicPinkProgress");
    if (acrylic_prop_index >= 0) {
        const QMetaProperty acrylic_prop = window->metaObject()->property(acrylic_prop_index);
        if (acrylic_prop.hasNotifySignal()) {
            auto *refresh_timer = new QTimer(window);
            refresh_timer->setSingleShot(true);
            refresh_timer->setInterval(0);
            QObject::connect(refresh_timer, &QTimer::timeout, window, [window]() {
                if (window->isVisible()) {
                    applyWindowsAcrylic(window);
                }
            });

            const int start_method_index = refresh_timer->metaObject()->indexOfMethod("start()");
            if (start_method_index >= 0) {
                const QMetaMethod start_method = refresh_timer->metaObject()->method(start_method_index);
                QObject::connect(window,
                                 acrylic_prop.notifySignal(),
                                 refresh_timer,
                                 start_method);
            }
        }
    }
}

void applyAcrylicToTopLevelQuickWindows()
{
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow *win : windows) {
        auto *quickWindow = qobject_cast<QQuickWindow *>(win);
        if (!quickWindow) {
            continue;
        }
        ensureAcrylicVisibleHook(quickWindow);
        if (quickWindow->isVisible()) {
            applyWindowsAcrylic(quickWindow);
        }
    }
}
#endif

namespace {
constexpr int kInitialCenterRetryCount = 6;
constexpr int kInitialCenterRetryIntervalMs = 80;

struct RuntimeLogConfig {
    QString level = "info";
    QString dir = "./logs";
    bool toConsole = true;
    int maxFiles = 14;
    QString env = "local";
    QString serviceName = "MemoChatQml";
    QString serviceInstance = "MemoChatQml@localhost";
};

RuntimeLogConfig g_log_cfg;
QMutex g_log_mutex;

int levelWeight(const QString &level)
{
    const QString v = level.trimmed().toLower();
    if (v == "debug") {
        return 0;
    }
    if (v == "info") {
        return 1;
    }
    if (v == "warn" || v == "warning") {
        return 2;
    }
    return 3;
}

int msgWeight(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return 0;
    case QtInfoMsg: return 1;
    case QtWarningMsg: return 2;
    case QtCriticalMsg: return 3;
    case QtFatalMsg: return 3;
    }
    return 1;
}

QString msgLevel(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return "debug";
    case QtInfoMsg: return "info";
    case QtWarningMsg: return "warn";
    case QtCriticalMsg: return "error";
    case QtFatalMsg: return "fatal";
    }
    return "info";
}

QString resolveLogDir(const QString &appPath, const QString &configuredDir)
{
    const QString dir = configuredDir.trimmed();
    if (dir.isEmpty()) {
        return QDir(appPath).filePath("logs");
    }
    QDir maybeRelative(dir);
    if (maybeRelative.isRelative()) {
        return QDir(appPath).filePath(dir);
    }
    return dir;
}

void cleanupOldLogs()
{
    QDir dir(g_log_cfg.dir);
    const QStringList files = dir.entryList(QStringList() << "MemoChatQml_*.json",
                                            QDir::Files,
                                            QDir::Name | QDir::Reversed);
    for (int i = g_log_cfg.maxFiles; i < files.size(); ++i) {
        dir.remove(files.at(i));
    }
}

void loadRuntimeLogConfig(const QString &configPath, const QString &appPath)
{
    QSettings settings(configPath, QSettings::IniFormat);
    g_log_cfg.level = settings.value("Log/Level", "info").toString().trimmed().toLower();
    g_log_cfg.dir = resolveLogDir(appPath, settings.value("Log/Dir", "./logs").toString());
    g_log_cfg.toConsole = settings.value("Log/ToConsole", true).toBool();
    g_log_cfg.maxFiles = settings.value("Log/MaxFiles", 14).toInt();
    if (g_log_cfg.maxFiles <= 0) {
        g_log_cfg.maxFiles = 14;
    }
    g_log_cfg.env = settings.value("Log/Env", "local").toString().trimmed();
    g_log_cfg.serviceName = settings.value("Telemetry/ServiceName", "MemoChatQml").toString().trimmed();
    if (g_log_cfg.serviceName.isEmpty()) {
        g_log_cfg.serviceName = "MemoChatQml";
    }
    g_log_cfg.serviceInstance = QStringLiteral("%1@%2:%3")
                                   .arg(g_log_cfg.serviceName,
                                        QSysInfo::machineHostName().isEmpty() ? QStringLiteral("localhost") : QSysInfo::machineHostName(),
                                        QString::number(QCoreApplication::applicationPid()));
    QDir().mkpath(g_log_cfg.dir);
}

void fileMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QMutexLocker locker(&g_log_mutex);
    if (msgWeight(type) < levelWeight(g_log_cfg.level)) {
        return;
    }

    cleanupOldLogs();
    const QString dateTag = QDate::currentDate().toString("yyyyMMdd");
    QFile file(QDir(g_log_cfg.dir).filePath(QString("MemoChatQml_%1.json").arg(dateTag)));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QJsonObject obj;
        obj["ts"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        obj["level"] = msgLevel(type);
        obj["service"] = g_log_cfg.serviceName;
        obj["service_instance"] = g_log_cfg.serviceInstance;
        obj["env"] = g_log_cfg.env;
        obj["event"] = "qt.message";
        obj["message"] = msg;
        obj["request_id"] = QString();
        obj["span_id"] = QString();
        obj["module"] = "qt";
        obj["peer_service"] = QString();
        obj["error_code"] = QString();
        obj["error_type"] = (type == QtCriticalMsg || type == QtFatalMsg) ? "qt" : QString();
        obj["duration_ms"] = 0;
        obj["attrs"] = QJsonObject();
        const QByteArray line = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        file.write(line);
        file.write("\n");
        file.flush();
        if (g_log_cfg.toConsole) {
            std::fprintf(stderr, "%s\n", line.constData());
        }
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void setDefaultEnv(const char *name, const char *value)
{
    if (qEnvironmentVariableIsEmpty(name)) {
        qputenv(name, value);
    }
}

#ifdef Q_OS_LINUX
bool isWslEnvironment()
{
    QFile osrelease(QStringLiteral("/proc/sys/kernel/osrelease"));
    if (!osrelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
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
    if (qmlCacheDisable == "0" || qmlCacheDisable == "false" || qmlCacheDisable == "no") {
        qunsetenv("QML_DISABLE_DISK_CACHE");
    }
    if (isWslEnvironment()) {
        setDefaultEnv("GALLIUM_DRIVER", "d3d12");
        setDefaultEnv("MESA_LOADER_DRIVER_OVERRIDE", "d3d12");
        setDefaultEnv("LIBGL_ALWAYS_SOFTWARE", "0");
    }
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
}

void startIbusDaemon(bool replace)
{
    QStringList args{QStringLiteral("-drx")};
    if (replace) {
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
    if (QStandardPaths::findExecutable(QStringLiteral("ibus")).isEmpty()) {
        return false;
    }
    for (int attempt = 0; attempt < 10; ++attempt) {
        QProcess process;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove(QStringLiteral("WAYLAND_DISPLAY"));
        process.setProcessEnvironment(env);
        process.setProgram(QStringLiteral("ibus"));
        process.setArguments({QStringLiteral("engine"), QStringLiteral("libpinyin")});
        process.start();
        if (process.waitForFinished(3000) && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
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
    if (!QStandardPaths::findExecutable(QStringLiteral("ibus")).isEmpty()) {
        QProcess process;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove(QStringLiteral("WAYLAND_DISPLAY"));
        process.setProcessEnvironment(env);
        process.setProgram(QStringLiteral("ibus"));
        process.setArguments({QStringLiteral("exit")});
        process.start();
        process.waitForFinished(3000);
    }
    if (!QStandardPaths::findExecutable(QStringLiteral("pkill")).isEmpty()) {
        QProcess::execute(QStringLiteral("pkill"), {QStringLiteral("-x"), QStringLiteral("ibus-daemon")});
    }
    QThread::msleep(200);
}

void configureLinuxInputMethod()
{
    const bool keep_existing = qEnvironmentVariableIntValue("MEMOCHAT_KEEP_QT_IM_MODULE") == 1;
    if (keep_existing && !qEnvironmentVariableIsEmpty("QT_IM_MODULE")) {
        return;
    }

    const QString plugin_path = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    const auto hasPlatformInputContext = [&plugin_path](const QString &pluginName) {
        return !plugin_path.isEmpty()
               && QFile::exists(QDir(plugin_path).filePath(
                   QStringLiteral("platforminputcontexts/%1").arg(pluginName)));
    };
    const bool has_fcitx_plugin = hasPlatformInputContext(QStringLiteral("libfcitx5platforminputcontextplugin.so"));
    const bool has_ibus_plugin = hasPlatformInputContext(QStringLiteral("libibusplatforminputcontextplugin.so"));
    const bool has_pgrep = !QStandardPaths::findExecutable(QStringLiteral("pgrep")).isEmpty();
    const bool has_fcitx = !QStandardPaths::findExecutable(QStringLiteral("fcitx5")).isEmpty();
    const bool has_ibus = !QStandardPaths::findExecutable(QStringLiteral("ibus-daemon")).isEmpty();

    if (has_fcitx && has_fcitx_plugin) {
        if (!has_pgrep || QProcess::execute(QStringLiteral("pgrep"), {QStringLiteral("-x"), QStringLiteral("fcitx5")}) != 0) {
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

    if (has_ibus && has_ibus_plugin) {
        setDefaultEnv("IBUS_ENABLE_SYNC_MODE", "1");
        qunsetenv("WAYLAND_DISPLAY");
        if (shouldRestartIbusDaemon()) {
            stopIbusDaemons();
            startIbusDaemon(false);
        } else {
            const bool ibus_running = has_pgrep
                && QProcess::execute(QStringLiteral("pgrep"), {QStringLiteral("-x"), QStringLiteral("ibus-daemon")}) == 0;
            startIbusDaemon(ibus_running);
        }
        QThread::msleep(150);
        if (!selectIbusLibpinyinEngine()) {
            startIbusDaemon(true);
            QThread::msleep(150);
            selectIbusLibpinyinEngine();
        }
        qputenv("QT_IM_MODULE", "ibus");
        qputenv("GTK_IM_MODULE", "ibus");
        qputenv("XMODIFIERS", "@im=ibus");
        return;
    }

    if (!plugin_path.isEmpty()
        && QFile::exists(QDir(plugin_path).filePath(QStringLiteral("platforminputcontexts/libqtvirtualkeyboardplugin.so")))) {
        qputenv("QT_IM_MODULE", "qtvirtualkeyboard");
        qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", "0");
    }
}
#endif

void centerTopLevelWindow(QWindow *window)
{
    if (!window || !window->isVisible() || window->visibility() != QWindow::Windowed) {
        return;
    }

    QScreen *screen = window->screen();
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const QRect area = screen->availableGeometry();
    const QSize size = window->size();
    if (!area.isValid() || size.width() <= 0 || size.height() <= 0) {
        return;
    }

    const int x = area.x() + qMax(0, (area.width() - size.width()) / 2);
    const int y = area.y() + qMax(0, (area.height() - size.height()) / 2);
    window->setPosition(x, y);
}

void scheduleInitialWindowCentering(QWindow *window, int remaining = kInitialCenterRetryCount)
{
    if (!window || window->property("_memochat_initial_centering").toBool()) {
        return;
    }
    window->setProperty("_memochat_initial_centering", true);

    auto center_again = [window, remaining]() {
        centerTopLevelWindow(window);
        if (remaining <= 0 || !window->isVisible()) {
            window->setProperty("_memochat_initial_centering", false);
            return;
        }
        window->setProperty("_memochat_initial_centering", false);
        QTimer::singleShot(kInitialCenterRetryIntervalMs, window, [window, remaining]() {
            scheduleInitialWindowCentering(window, remaining - 1);
        });
    };

    QTimer::singleShot(0, window, center_again);
}

void ensureInitialCenteringHook(QQuickWindow *window)
{
    if (!window
        || !window->property("memochatStartupCenter").toBool()
        || window->property("_memochat_center_hooked").toBool()) {
        return;
    }
    window->setProperty("_memochat_center_hooked", true);
    QObject::connect(window,
                     &QWindow::visibleChanged,
                     window,
                     [window](bool visible) {
                         if (visible) {
                             scheduleInitialWindowCentering(window);
                         }
                     });
    if (window->isVisible()) {
        scheduleInitialWindowCentering(window);
    }
}

void ensureTopLevelQuickWindowHooks()
{
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow *win : windows) {
        auto *quick_window = qobject_cast<QQuickWindow *>(win);
        if (!quick_window) {
            continue;
        }
        ensureInitialCenteringHook(quick_window);
#ifdef Q_OS_WIN
        ensureAcrylicVisibleHook(quick_window);
#endif
    }
}
}

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral("MemoChatQml"));
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#ifdef Q_OS_LINUX
    configureLinuxRendering();
    configureLinuxInputMethod();
#endif
    QSurfaceFormat format;
    format.setSamples(4);
#ifdef Q_OS_LINUX
    format.setAlphaBufferSize(8);
#endif
    QSurfaceFormat::setDefaultFormat(format);
    QString app_path = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
    if (app_path.isEmpty()) {
        app_path = QDir::currentPath();
    }
    const QString config_path = QDir::toNativeSeparators(
        app_path + QDir::separator() + QStringLiteral("config.ini"));
    loadRuntimeLogConfig(config_path, app_path);
    qInstallMessageHandler(fileMessageHandler);
    QQuickStyle::setStyle("Basic");
    QtWebEngineQuick::initialize();

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/app/icon.ico")));

    const QString runtime_app_path = QCoreApplication::applicationDirPath();
    const QString runtime_config_path = QDir::toNativeSeparators(
        runtime_app_path + QDir::separator() + QStringLiteral("config.ini"));

    QSettings settings(runtime_config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString().trimmed();
    if (gate_host.isEmpty()) {
        gate_host = settings.value("GateServer/Host").toString().trimmed();
    }
    if (gate_host.isEmpty()) {
        gate_host = "127.0.0.1";
    }
    if (gate_host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0) {
        gate_host = QStringLiteral("127.0.0.1");
    }

    QString gate_port = settings.value("GateServer/port").toString().trimmed();
    if (gate_port.isEmpty()) {
        gate_port = settings.value("GateServer/Port").toString().trimmed();
    }
    if (gate_port.isEmpty()) {
        gate_port = "8080";
    }

    QString media_gate_port = settings.value("GateServer/http_port").toString().trimmed();
    if (media_gate_port.isEmpty()) {
        media_gate_port = settings.value("GateServer/HttpPort").toString().trimmed();
    }
    if (media_gate_port.isEmpty()) {
        media_gate_port = gate_port;
    }

    gate_url_prefix = (gate_port == "8443") ? "https://" + gate_host + ":" + gate_port : "http://" + gate_host + ":" + gate_port;
    gate_media_url_prefix = "http://" + gate_host + ":" + media_gate_port;

    qmlRegisterUncreatableType<AppController>(
        "MemoChat", 1, 0, "AppController", "Enum only");
    qmlRegisterUncreatableType<CallSessionModel>(
        "MemoChat", 1, 0, "CallSessionModel", "Exposed via AppController");
    qmlRegisterUncreatableType<PetController>(
        "MemoChat", 1, 0, "PetController", "Exposed via AppController");
    qmlRegisterUncreatableType<PetModel>(
        "MemoChat", 1, 0, "PetModel", "Exposed via PetController");
    qmlRegisterType<PetSpeechSynthesizer>(
        "MemoChat", 1, 0, "PetSpeechSynthesizer");
    qmlRegisterType<PetAssetSettings>(
        "MemoChat", 1, 0, "PetAssetSettings");
    qmlRegisterType<Live2DAsset>(
        "MemoChat", 1, 0, "Live2DAsset");
    qmlRegisterType<Live2DRenderItem>(
        "MemoChat", 1, 0, "Live2DRenderItem");

    AppController controller;
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) {
            qWarning().noquote() << warning.toString();
        }
    });
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.rootContext()->setContextProperty("gateUrlPrefix", gate_url_prefix);
    engine.rootContext()->setContextProperty("gateMediaUrlPrefix", gate_media_url_prefix);
    engine.rootContext()->setContextProperty("livekitBridge", controller.livekitBridge());

#ifdef Q_OS_LINUX
    const QUrl main_url(QStringLiteral("qrc:/qml/linux/Main.qml"));
#else
    const QUrl main_url(QStringLiteral("qrc:/qml/Main.qml"));
#endif
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [main_url](QObject *obj, const QUrl &obj_url) {
            if (!obj && obj_url == main_url) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(main_url);
    if (!engine.rootObjects().isEmpty()) {
        QObject *root_object = engine.rootObjects().constFirst();
        if (auto *window = qobject_cast<QQuickWindow *>(root_object)) {
            window->setColor(Qt::transparent);
            ensureInitialCenteringHook(window);
        }
    }
    ensureTopLevelQuickWindowHooks();
    QObject::connect(&app,
                     &QGuiApplication::focusWindowChanged,
                     &app,
                     [](QWindow *) {
                         ensureTopLevelQuickWindowHooks();
                     });
#ifdef Q_OS_WIN
    QTimer::singleShot(0, &app, []() {
        applyAcrylicToTopLevelQuickWindows();
    });
    QObject::connect(&app,
                     &QGuiApplication::focusWindowChanged,
                     &app,
                     [](QWindow *) {
                         applyAcrylicToTopLevelQuickWindows();
                     });
#endif

    return app.exec();
}
