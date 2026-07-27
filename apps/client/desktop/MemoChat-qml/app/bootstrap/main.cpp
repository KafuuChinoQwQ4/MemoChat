#include <QApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QQuickStyle>
#include <QIcon>
#include "AppComposition.h"
#include "MainLogging.h"
#include "MainPlatformBootstrap.h"
#include "MainQmlBootstrap.h"
#include "MainRuntimeConfig.h"
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

#if defined(__ELF__) && MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD
namespace
{
[[gnu::used, gnu::section(".note.memochat.release")]] const char kMemoChatReleasePolicy[] =
    "MEMOCHAT_RELEASE_POLICY:v1;distributable=1;live2d_native=0;restricted_assets=0";
}
#endif

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

    if (!configureGateUrlPrefixes(configPathForAppPath(QCoreApplication::applicationDirPath())))
    {
        return -1;
    }
    registerMemoChatQmlTypes();

    AppComposition composition;
    QQmlApplicationEngine engine;
    configureMemoChatEngine(engine, composition);
    if (!loadMemoChatMainWindow(engine, app, composition))
    {
        return -1;
    }

    return app.exec();
}
