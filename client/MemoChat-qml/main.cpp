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
#include <QWindow>
#include <QQuickWindow>
#include <QTimer>
#include <QQuickStyle>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <cstdio>
#include "AppController.h"
#include "global.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

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

    // Preferred path on newer Windows builds: system backdrop type (Acrylic-like transient window).
    const int backdrop_type = DWMSBT_TRANSIENTWINDOW;
    HRESULT hr = DwmSetWindowAttribute(
        hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
    if (SUCCEEDED(hr)) {
        return;
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
    // ABGR (AABBGGRR): very light cool tint for transparent glass look.
    policy.gradient_color = static_cast<int>(0x15FFEEF5);
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
struct RuntimeLogConfig {
    QString level = "info";
    QString dir = "./logs";
    bool toConsole = true;
    int maxFiles = 14;
    QString env = "local";
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
        obj["service"] = "MemoChatQml";
        obj["env"] = g_log_cfg.env;
        obj["event"] = "qt.message";
        obj["message"] = msg;
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
}

int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setSamples(8);
    QSurfaceFormat::setDefaultFormat(format);

    // Avoid stale QML cache after frequent qrc/page changes.
    qputenv("QML_DISABLE_DISK_CACHE", "1");
    QString app_path = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
    if (app_path.isEmpty()) {
        app_path = QDir::currentPath();
    }
    const QString config_path = QDir::toNativeSeparators(
        app_path + QDir::separator() + QStringLiteral("config.ini"));
    loadRuntimeLogConfig(config_path, app_path);
    qInstallMessageHandler(fileMessageHandler);
    QQuickStyle::setStyle("Basic");

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

    QString gate_port = settings.value("GateServer/port").toString().trimmed();
    if (gate_port.isEmpty()) {
        gate_port = settings.value("GateServer/Port").toString().trimmed();
    }
    if (gate_port.isEmpty()) {
        gate_port = "8080";
    }

    gate_url_prefix = "http://" + gate_host + ":" + gate_port;

    qmlRegisterUncreatableType<AppController>(
        "MemoChat", 1, 0, "AppController", "Enum only");

    AppController controller;
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) {
            qWarning().noquote() << warning.toString();
        }
    });
    engine.rootContext()->setContextProperty("controller", &controller);

    const QUrl main_url(QStringLiteral("qrc:/qml/Main.qml"));
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
        }
    }
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
