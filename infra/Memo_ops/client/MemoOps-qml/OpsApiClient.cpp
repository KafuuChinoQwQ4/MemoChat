#include "OpsApiClient.h"

#include "OpsApiTransport.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

namespace
{
QString loadBaseUrl()
{
    QString appPath = QCoreApplication::applicationDirPath();
    const QString configPath = QDir(appPath).filePath("memoops-qml.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    QString baseUrl = settings.value("OpsServer/BaseUrl", "http://127.0.0.1:18080").toString().trimmed();
    if (baseUrl.endsWith('/'))
    {
        baseUrl.chop(1);
    }
    return baseUrl;
}
} // namespace

OpsApiClient::OpsApiClient(QObject* parent)
    : QObject(parent)
    , m_baseUrl(loadBaseUrl())
    , m_transport(new OpsApiTransport(m_baseUrl, this))
{
    connect(m_transport,
            &OpsApiTransport::requestStarted,
            this,
            [this]()
            {
                ++m_inFlight;
                setBusy(true);
            });
    connect(m_transport,
            &OpsApiTransport::requestFinished,
            this,
            [this]()
            {
                if (m_inFlight > 0)
                {
                    --m_inFlight;
                }
                setBusy(m_inFlight > 0);
            });
    connect(m_transport,
            &OpsApiTransport::requestSucceeded,
            this,
            [this]()
            {
                setLastError(QString());
            });
    connect(m_transport, &OpsApiTransport::requestFailed, this, &OpsApiClient::setLastError);
}

QString OpsApiClient::baseUrl() const
{
    return m_baseUrl;
}
QJsonObject OpsApiClient::overview() const
{
    return m_overview;
}
QJsonObject OpsApiClient::overviewKpis() const
{
    return m_overviewKpis;
}
QJsonArray OpsApiClient::runs() const
{
    return m_runs;
}
QJsonObject OpsApiClient::selectedRun() const
{
    return m_selectedRun;
}
QJsonArray OpsApiClient::logs() const
{
    return m_logs;
}
QJsonObject OpsApiClient::logSearchResult() const
{
    return m_logSearchResult;
}
QJsonObject OpsApiClient::logTrend() const
{
    return m_logTrend;
}
QJsonObject OpsApiClient::serviceTrend() const
{
    return m_serviceTrend;
}
QJsonObject OpsApiClient::loadtestTrend() const
{
    return m_loadtestTrend;
}
QJsonObject OpsApiClient::selectedTrace() const
{
    return m_selectedTrace;
}
QJsonArray OpsApiClient::services() const
{
    return m_services;
}
QJsonObject OpsApiClient::selectedService() const
{
    return m_selectedService;
}
QJsonObject OpsApiClient::selectedLogFilters() const
{
    return m_selectedLogFilters;
}
QJsonArray OpsApiClient::alerts() const
{
    return m_alerts;
}
QJsonObject OpsApiClient::dataSources() const
{
    return m_dataSources;
}
QJsonArray OpsApiClient::systemMetrics() const
{
    return m_systemMetrics;
}
QJsonObject OpsApiClient::loadtestRunStatus() const
{
    return m_loadtestRunStatus;
}
QJsonArray OpsApiClient::tailLogItems() const
{
    return m_tailLogs;
}
bool OpsApiClient::busy() const
{
    return m_busy;
}
QString OpsApiClient::lastError() const
{
    return m_lastError;
}

void OpsApiClient::refreshAll()
{
    refreshOverview();
    refreshRuns();
    refreshLogSearch();
    refreshLogTrend();
    refreshServices();
    refreshLoadtestTrend();
    refreshAlerts();
    refreshDataSources();
}

void OpsApiClient::setBusy(bool value)
{
    if (m_busy == value)
    {
        return;
    }
    m_busy = value;
    emit busyChanged();
}

void OpsApiClient::setLastError(const QString& message)
{
    if (m_lastError == message)
    {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}
