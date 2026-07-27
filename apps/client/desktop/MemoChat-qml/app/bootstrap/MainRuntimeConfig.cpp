#include "MainRuntimeConfig.h"

#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

extern QString gate_url_prefix;
extern QString gate_media_url_prefix;

namespace
{
#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD
bool isValidHttpsEndpoint(const QString& value)
{
    const QUrl url(value);
    return url.isValid() && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0 &&
                                                 !url.host().isEmpty() &&
                                                 url.userInfo().isEmpty();
}

bool validateOptionalHttpsSetting(const QSettings& settings, const QString& key)
{
    const QString value = settings.value(key).toString().trimmed();
    if (value.isEmpty() || isValidHttpsEndpoint(value))
    {
        return true;
    }
    qCritical() << "Distributable client requires an HTTPS endpoint for" << key;
    return false;
}
#endif
} // namespace

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

bool configureGateUrlPrefixes(const QString& configPath)
{
    gate_url_prefix.clear();
    gate_media_url_prefix.clear();
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
    QString gateScheme = settings.value("GateServer/scheme").toString().trimmed().toLower();
#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD
    if (gateScheme != QStringLiteral("https"))
    {
        qCritical() << "Distributable client requires GateServer/scheme=https";
        return false;
    }
#else
    if (gateScheme != QStringLiteral("https") && gateScheme != QStringLiteral("http"))
    {
        gateScheme = gatePort == QStringLiteral("8443") ? QStringLiteral("https") : QStringLiteral("http");
    }
#endif

    const QString mediaBaseUrl = settings.value("Media/BaseUrl").toString().trimmed();
#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD
    if (!validateOptionalHttpsSetting(settings,
                                      QStringLiteral("Media/BaseUrl")) ||
                                      !validateOptionalHttpsSetting(
                                          settings,
                                          QStringLiteral("AI/BaseUrl")) ||
                                          !validateOptionalHttpsSetting(settings, QStringLiteral("Call/CallBaseUrl")))
    {
        return false;
    }
#endif
    QString mediaGatePort = settings.value("GateServer/media_port").toString().trimmed();
    if (mediaGatePort.isEmpty())
    {
        mediaGatePort = settings.value("GateServer/http_port").toString().trimmed();
    }
    if (mediaGatePort.isEmpty())
    {
        mediaGatePort = gatePort;
    }

    gate_url_prefix = gateScheme + QStringLiteral("://") + gateHost + QStringLiteral(":") + gatePort;
    gate_media_url_prefix = mediaBaseUrl.isEmpty()
        ? gateScheme + QStringLiteral("://") + gateHost + QStringLiteral(":") + mediaGatePort : mediaBaseUrl;
#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD
    if (!isValidHttpsEndpoint(gate_url_prefix) || !isValidHttpsEndpoint(gate_media_url_prefix))
    {
        qCritical() << "Distributable client produced an invalid HTTPS gateway endpoint";
        gate_url_prefix.clear();
        gate_media_url_prefix.clear();
        return false;
    }
#endif
    return true;
}
