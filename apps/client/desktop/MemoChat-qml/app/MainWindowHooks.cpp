#include "MainWindowHooks.h"

#include "MainWindowEffects.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QtGlobal>

namespace
{
constexpr int kInitialCenterRetryCount = 6;
constexpr int kInitialCenterRetryIntervalMs = 80;
} // namespace

void centerTopLevelWindow(QWindow* window)
{
    if (!window || !window->isVisible() || window->visibility() == QWindow::Hidden ||
        window->visibility() == QWindow::Minimized || window->visibility() == QWindow::Maximized ||
        window->visibility() == QWindow::FullScreen)
    {
        return;
    }

    QScreen* screen = window->screen();
    if (!screen)
    {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen)
    {
        return;
    }

    const QRect area = screen->availableGeometry();
    const QSize size = window->size();
    if (!area.isValid() || size.width() <= 0 || size.height() <= 0)
    {
        return;
    }

    const int x = area.x() + qMax(0, (area.width() - size.width()) / 2);
    const int y = area.y() + qMax(0, (area.height() - size.height()) / 2);
    window->setPosition(x, y);
}

void scheduleInitialWindowCentering(QWindow* window, int remaining = kInitialCenterRetryCount)
{
    if (!window || window->property("_memochat_initial_centering").toBool())
    {
        return;
    }
    window->setProperty("_memochat_initial_centering", true);

    auto center_again = [window, remaining]()
    {
        centerTopLevelWindow(window);
        if (remaining <= 0 || !window->isVisible())
        {
            window->setProperty("_memochat_initial_centering", false);
            return;
        }
        window->setProperty("_memochat_initial_centering", false);
        QTimer::singleShot(kInitialCenterRetryIntervalMs,
                           window,
                           [window, remaining]()
                           {
                               scheduleInitialWindowCentering(window, remaining - 1);
                           });
    };

    QTimer::singleShot(0, window, center_again);
}

void ensureInitialCenteringHook(QQuickWindow* window)
{
    if (!window || !window->property("memochatStartupCenter").toBool() ||
        window->property("_memochat_center_hooked").toBool())
    {
        return;
    }
    window->setProperty("_memochat_center_hooked", true);
    QObject::connect(window,
                     &QWindow::visibleChanged,
                     window,
                     [window](bool visible)
                     {
                         if (visible)
                         {
                             scheduleInitialWindowCentering(window);
                         }
                     });
    QObject::connect(window,
                     &QWindow::widthChanged,
                     window,
                     [window](int)
                     {
                         scheduleInitialWindowCentering(window);
                     });
    QObject::connect(window,
                     &QWindow::heightChanged,
                     window,
                     [window](int)
                     {
                         scheduleInitialWindowCentering(window);
                     });
    if (window->isVisible())
    {
        scheduleInitialWindowCentering(window);
    }
}

void ensureTopLevelQuickWindowHooks()
{
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow* win : windows)
    {
        auto* quick_window = qobject_cast<QQuickWindow*>(win);
        if (!quick_window)
        {
            continue;
        }
        ensureInitialCenteringHook(quick_window);
#ifdef Q_OS_WIN
        ensureAcrylicVisibleHook(quick_window);
#endif
    }
}

void scheduleTopLevelQuickWindowHookRetries(QObject* context, int remaining)
{
    if (!context || remaining < 0)
    {
        return;
    }
    QTimer::singleShot(kInitialCenterRetryIntervalMs,
                       context,
                       [context, remaining]()
                       {
                           ensureTopLevelQuickWindowHooks();
                           scheduleTopLevelQuickWindowHookRetries(context, remaining - 1);
                       });
}
