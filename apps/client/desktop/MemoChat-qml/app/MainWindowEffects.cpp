#include "MainWindowEffects.h"

#ifdef Q_OS_WIN
#include "ClientWinCompat.h"

#include <QGuiApplication>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QQuickWindow>
#include <QTimer>
#include <QVariant>

#include <algorithm>

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

enum class WindowCompositionAttribute : int
{
    WCA_ACCENT_POLICY = 19
};

enum class AccentState : int
{
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};

struct AccentPolicy
{
    int accent_state;
    int accent_flags;
    int gradient_color;
    int animation_id;
};

struct WindowCompositionAttributeData
{
    WindowCompositionAttribute attribute;
    PVOID data;
    SIZE_T size_of_data;
};

using SetWindowCompositionAttributeFn = BOOL(WINAPI*)(HWND, WindowCompositionAttributeData*);

int interpolateChannel(int from, int to, qreal progress)
{
    const qreal clamped = std::clamp(progress, 0.0, 1.0);
    return qRound(from + (to - from) * clamped);
}

int makeAbgrColor(int alpha, int red, int green, int blue)
{
    return ((alpha & 0xff) << 24) | ((blue & 0xff) << 16) | ((green & 0xff) << 8) | (red & 0xff);
}

qreal acrylicPinkProgressForWindow(QQuickWindow* window)
{
    if (!window)
    {
        return 0.0;
    }
    return std::clamp(window->property("acrylicPinkProgress").toDouble(), 0.0, 1.0);
}

int acrylicGradientColorForWindow(QQuickWindow* window)
{
    const qreal progress = acrylicPinkProgressForWindow(window);
    return makeAbgrColor(interpolateChannel(0x15, 0x24, progress),
                         interpolateChannel(0xF5, 0xFF, progress),
                         interpolateChannel(0xEE, 0xD8, progress),
                         interpolateChannel(0xFF, 0xE8, progress));
}

void applyWindowsAcrylic(QQuickWindow* window)
{
    if (!window)
    {
        return;
    }

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd)
    {
        return;
    }

    const int corner_pref = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_pref, sizeof(corner_pref));

    const bool has_custom_tint = window->property("acrylicPinkProgress").isValid();
    if (!has_custom_tint)
    {
        // Preferred path on newer Windows builds: system backdrop type (Acrylic-like transient window).
        const int backdrop_type = DWMSBT_TRANSIENTWINDOW;
        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
        if (SUCCEEDED(hr))
        {
            return;
        }
    }

    // Fallback path: Windows 10 Acrylic blur via undocumented composition attribute.
    const HMODULE user32_module = GetModuleHandleW(L"user32.dll");
    if (!user32_module)
    {
        return;
    }

    auto set_wca = reinterpret_cast<SetWindowCompositionAttributeFn>(
        GetProcAddress(user32_module, "SetWindowCompositionAttribute"));
    if (!set_wca)
    {
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

void ensureAcrylicVisibleHook(QQuickWindow* window)
{
    if (!window || window->property("_memochat_acrylic_hooked").toBool())
    {
        return;
    }
    window->setProperty("_memochat_acrylic_hooked", true);
    QObject::connect(window,
                     &QWindow::visibleChanged,
                     window,
                     [window](bool visible)
                     {
                         if (!visible)
                         {
                             return;
                         }
                         QTimer::singleShot(0,
                                            window,
                                            [window]()
                                            {
                                                applyWindowsAcrylic(window);
                                            });
                     });

    const int acrylic_prop_index = window->metaObject()->indexOfProperty("acrylicPinkProgress");
    if (acrylic_prop_index >= 0)
    {
        const QMetaProperty acrylic_prop = window->metaObject()->property(acrylic_prop_index);
        if (acrylic_prop.hasNotifySignal())
        {
            auto* refresh_timer = new QTimer(window);
            refresh_timer->setSingleShot(true);
            refresh_timer->setInterval(0);
            QObject::connect(refresh_timer,
                             &QTimer::timeout,
                             window,
                             [window]()
                             {
                                 if (window->isVisible())
                                 {
                                     applyWindowsAcrylic(window);
                                 }
                             });

            const int start_method_index = refresh_timer->metaObject()->indexOfMethod("start()");
            if (start_method_index >= 0)
            {
                const QMetaMethod start_method = refresh_timer->metaObject()->method(start_method_index);
                QObject::connect(window, acrylic_prop.notifySignal(), refresh_timer, start_method);
            }
        }
    }
}

void applyAcrylicToTopLevelQuickWindows()
{
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow* win : windows)
    {
        auto* quickWindow = qobject_cast<QQuickWindow*>(win);
        if (!quickWindow)
        {
            continue;
        }
        ensureAcrylicVisibleHook(quickWindow);
        if (quickWindow->isVisible())
        {
            applyWindowsAcrylic(quickWindow);
        }
    }
}
#endif
