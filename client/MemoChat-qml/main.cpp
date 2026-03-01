#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QSurfaceFormat>
#include <QWindow>
#include <QQuickWindow>
#include <QTimer>
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
#endif

int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setSamples(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    const QString app_path = QCoreApplication::applicationDirPath();
    const QString file_name = "config.ini";
    const QString config_path = QDir::toNativeSeparators(
        app_path + QDir::separator() + file_name);

    QSettings settings(config_path, QSettings::IniFormat);
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
#ifdef Q_OS_WIN
            auto apply_backdrop = [window]() {
                applyWindowsAcrylic(window);
            };
            QTimer::singleShot(0, window, apply_backdrop);
            QObject::connect(
                window,
                &QWindow::visibleChanged,
                window,
                [apply_backdrop](bool visible) {
                    if (visible) {
                        apply_backdrop();
                    }
                });
#endif
        }
    }

    return app.exec();
}
