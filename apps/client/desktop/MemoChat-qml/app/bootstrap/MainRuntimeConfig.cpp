#include "MainRuntimeConfig.h"

#include "global.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

QString resolveStartupAppPath(const char* argv0)
{
    QString appPath = QFileInfo(QString::fromLocal8Bit(argv0)).absolutePath();
    if (appPath.isEmpty())
    {
        appPath = QDir::currentPath();
    }
    return appPath;
}

QString configPathForAppPath(const QString& appPath)
{
    return QDir::toNativeSeparators(appPath + QDir::separator() + QStringLiteral("config.ini"));
}

void configureGateUrlPrefixes(const QString& configPath)
{
    QSettings settings(configPath, QSettings::IniFormat);
    QString gateHost = settings.value("GateServer/host").toString().trimmed();
    if (gateHost.isEmpty())
    {
        gateHost = QStringLiteral("127.0.0.1");
    }
    if (gateHost.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0)
    {
        gateHost = QStringLiteral("127.0.0.1");
    }

    QString gatePort = settings.value("GateServer/port").toString().trimmed();
    if (gatePort.isEmpty())
    {
        gatePort = QStringLiteral("8080");
    }

    QString mediaGatePort = settings.value("GateServer/http_port").toString().trimmed();
    if (mediaGatePort.isEmpty())
    {
        mediaGatePort = gatePort;
    }

    gate_url_prefix = gatePort == QStringLiteral("8443")
                      ? QStringLiteral("https://") + gateHost +
                                       QStringLiteral(":") + gatePort
                                       : QStringLiteral("http://") + gateHost + QStringLiteral(":") + gatePort;
    gate_media_url_prefix = QStringLiteral("http://") + gateHost + QStringLiteral(":") + mediaGatePort;
}
