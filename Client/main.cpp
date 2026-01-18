#include "src/MainWindow.h"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString app_path = QCoreApplication::applicationDirPath();
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + fileName);

    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;

    // 加载 QSS
    QFile qss(":/style/stylesheet.qss");
    if(qss.open(QFile::ReadOnly)) {
        a.setStyleSheet(qss.readAll());
        qss.close();
    } else {
        // qDebug() << "Open qss failed";
    }

    MainWindow w;
    w.show();

    return a.exec();
}