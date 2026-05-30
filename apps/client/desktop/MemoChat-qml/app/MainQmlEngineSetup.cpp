#include "MainQmlBootstrap.h"

#include "AppController.h"
#include "global.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>

void configureMemoChatEngine(QQmlApplicationEngine& engine, AppController& controller)
{
    QObject::connect(&engine,
                     &QQmlApplicationEngine::warnings,
                     [](const QList<QQmlError>& warnings)
                     {
                         for (const auto& warning : warnings)
                         {
                             qWarning().noquote() << warning.toString();
                         }
                     });
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.rootContext()->setContextProperty("gateUrlPrefix", gate_url_prefix);
    engine.rootContext()->setContextProperty("gateMediaUrlPrefix", gate_media_url_prefix);
    engine.rootContext()->setContextProperty("livekitBridge", controller.livekitBridge());
}
