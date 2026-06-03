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
    engine.rootContext()->setContextProperty("shell", controller.shellViewModel());
    engine.rootContext()->setContextProperty("auth", controller.authViewModel());
    engine.rootContext()->setContextProperty("chat", controller.chatViewModel());
    engine.rootContext()->setContextProperty("contact", controller.contactController());
    engine.rootContext()->setContextProperty("group", controller.groupController());
    engine.rootContext()->setContextProperty("profile", controller.profileController());
    engine.rootContext()->setContextProperty("call", controller.callController());
    engine.rootContext()->setContextProperty("agent", controller.agentController());
    engine.rootContext()->setContextProperty("agentMessages", controller.agentMessageModel());
    engine.rootContext()->setContextProperty("moments", controller.momentsController());
    engine.rootContext()->setContextProperty("momentsModel", controller.momentsModel());
    engine.rootContext()->setContextProperty("pet", controller.petController());
    engine.rootContext()->setContextProperty("r18", controller.r18Controller());
    engine.rootContext()->setContextProperty("gateUrlPrefix", gate_url_prefix);
    engine.rootContext()->setContextProperty("gateMediaUrlPrefix", gate_media_url_prefix);
}
