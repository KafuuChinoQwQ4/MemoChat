#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QSurfaceFormat>
#include <QWindow>
#include <QQuickWindow>
#include "AppController.h"
#include "global.h"

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
        }
    }

    return app.exec();
}
