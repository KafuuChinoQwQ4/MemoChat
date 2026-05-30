#include <QApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QQuickStyle>
#include <QIcon>
#include "AppController.h"
#include "MainLogging.h"
#include "MainPlatformBootstrap.h"
#include "MainQmlBootstrap.h"
#include "MainRuntimeConfig.h"
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char* argv[])
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
    const QString startupAppPath = resolveStartupAppPath(argv[0]);
    const QString startupConfigPath = configPathForAppPath(startupAppPath);
    loadRuntimeLogConfig(startupConfigPath, startupAppPath);
    qInstallMessageHandler(fileMessageHandler);
    QQuickStyle::setStyle("Basic");
    QtWebEngineQuick::initialize();

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon(QStringLiteral(":/app/icon.ico")));

    configureGateUrlPrefixes(configPathForAppPath(QCoreApplication::applicationDirPath()));
    registerMemoChatQmlTypes();

    AppController controller;
    QQmlApplicationEngine engine;
    configureMemoChatEngine(engine, controller);
    if (!loadMemoChatMainWindow(engine, app, controller))
    {
        return -1;
    }

    return app.exec();
}
