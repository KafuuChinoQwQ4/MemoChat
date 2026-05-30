#include "MainQmlBootstrap.h"

#include "AppController.h"
#include "MainWindowEffects.h"
#include "MainWindowHooks.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QWindow>

QUrl memoChatMainQmlUrl()
{
#ifdef Q_OS_LINUX
    return QUrl(QStringLiteral("qrc:/qml/linux/Main.qml"));
#else
    return QUrl(QStringLiteral("qrc:/qml/Main.qml"));
#endif
}

bool loadMemoChatMainWindow(QQmlApplicationEngine& engine, QApplication& app, AppController&)
{
    const QUrl mainUrl = memoChatMainQmlUrl();
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [mainUrl](QObject* obj, const QUrl& objUrl)
        {
            if (!obj && objUrl == mainUrl)
            {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(mainUrl);
    if (engine.rootObjects().isEmpty())
    {
        return false;
    }

    QObject* rootObject = engine.rootObjects().constFirst();
    if (auto* window = qobject_cast<QQuickWindow*>(rootObject))
    {
        window->setColor(Qt::transparent);
        ensureInitialCenteringHook(window);
    }
    ensureTopLevelQuickWindowHooks();
    scheduleTopLevelQuickWindowHookRetries(&app);
    QObject::connect(&app,
                     &QGuiApplication::focusWindowChanged,
                     &app,
                     [](QWindow*)
                     {
                         ensureTopLevelQuickWindowHooks();
                     });
#ifdef Q_OS_WIN
    QTimer::singleShot(0,
                       &app,
                       []()
                       {
                           applyAcrylicToTopLevelQuickWindows();
                       });
    QObject::connect(&app,
                     &QGuiApplication::focusWindowChanged,
                     &app,
                     [](QWindow*)
                     {
                         applyAcrylicToTopLevelQuickWindows();
                     });
#endif

    return true;
}
