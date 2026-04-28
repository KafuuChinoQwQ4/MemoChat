#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "OpsApiClient.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("MemoOpsQml"));

    OpsApiClient apiClient;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("opsApi"), &apiClient);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    apiClient.refreshAll();
    return app.exec();
}
