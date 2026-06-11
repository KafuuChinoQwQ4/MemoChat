#include "MainQmlBootstrap.h"

#include "AppComposition.h"
#include "global.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>

void configureMemoChatEngine(QQmlApplicationEngine& engine, AppComposition& composition)
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
    composition.configureQmlContext(*engine.rootContext());
    engine.rootContext()->setContextProperty("gateUrlPrefix", gate_url_prefix);
    engine.rootContext()->setContextProperty("gateMediaUrlPrefix", gate_media_url_prefix);
}
