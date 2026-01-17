#include "src/MainWindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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