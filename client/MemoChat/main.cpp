#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QTimer>
#include "global.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss(":/style/stylesheet.qss");

    if( qss.open(QFile::ReadOnly))
    {
        qDebug("open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }else{
         qDebug("Open failed");
     }


    // 获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();
    // 拼接文件名
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path +
                             QDir::separator() + fileName);

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

    MainWindow w;
    w.showNormal();
    w.raise();
    w.activateWindow();
    QTimer::singleShot(0, &w, [&w]() {
        w.showNormal();
        w.raise();
        w.activateWindow();
    });
    return a.exec();
}
