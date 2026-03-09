#include "OpsApiClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrlQuery>

namespace {
QString loadBaseUrl()
{
    QString appPath = QCoreApplication::applicationDirPath();
    const QString configPath = QDir(appPath).filePath("memoops-qml.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    QString baseUrl = settings.value("OpsServer/BaseUrl", "http://127.0.0.1:18080").toString().trimmed();
    if (baseUrl.endsWith('/')) {
        baseUrl.chop(1);
    }
    return baseUrl;
}
}

OpsApiClient::OpsApiClient(QObject *parent)
    : QObject(parent),
      m_baseUrl(loadBaseUrl())
{
}

QString OpsApiClient::baseUrl() const { return m_baseUrl; }
QJsonObject OpsApiClient::overview() const { return m_overview; }
QJsonObject OpsApiClient::overviewKpis() const { return m_overviewKpis; }
QJsonArray OpsApiClient::runs() const { return m_runs; }
QJsonObject OpsApiClient::selectedRun() const { return m_selectedRun; }
QJsonArray OpsApiClient::logs() const { return m_logs; }
QJsonObject OpsApiClient::logSearchResult() const { return m_logSearchResult; }
QJsonObject OpsApiClient::logTrend() const { return m_logTrend; }
QJsonObject OpsApiClient::serviceTrend() const { return m_serviceTrend; }
QJsonObject OpsApiClient::loadtestTrend() const { return m_loadtestTrend; }
QJsonObject OpsApiClient::selectedTrace() const { return m_selectedTrace; }
QJsonArray OpsApiClient::services() const { return m_services; }
QJsonObject OpsApiClient::selectedService() const { return m_selectedService; }
QJsonObject OpsApiClient::selectedLogFilters() const { return m_selectedLogFilters; }
QJsonArray OpsApiClient::alerts() const { return m_alerts; }
QJsonObject OpsApiClient::dataSources() const { return m_dataSources; }
bool OpsApiClient::busy() const { return m_busy; }
QString OpsApiClient::lastError() const { return m_lastError; }

QJsonObject OpsApiClient::buildLogFilterState(const QString &service,
                                              const QString &instance,
                                              const QString &level,
                                              const QString &event,
                                              const QString &traceId,
                                              const QString &requestId,
                                              const QString &keyword,
                                              const QString &fromUtc,
                                              const QString &toUtc,
                                              int page,
                                              int pageSize,
                                              const QString &sort) const
{
    return {
        {"service", service},
        {"instance", instance},
        {"level", level},
        {"event", event},
        {"trace_id", traceId},
        {"request_id", requestId},
        {"keyword", keyword},
        {"from_utc", fromUtc},
        {"to_utc", toUtc},
        {"page", page},
        {"page_size", pageSize},
        {"sort", sort},
    };
}

QUrlQuery OpsApiClient::buildLogQuery(const QJsonObject &filters, bool includePaging) const
{
    QUrlQuery query;
    auto addIfPresent = [&query, &filters](const char *name) {
        const QString value = filters.value(QLatin1String(name)).toString();
        if (!value.isEmpty()) {
            query.addQueryItem(QLatin1String(name), value);
        }
    };
    addIfPresent("service");
    addIfPresent("instance");
    addIfPresent("level");
    addIfPresent("event");
    addIfPresent("trace_id");
    addIfPresent("request_id");
    addIfPresent("keyword");
    addIfPresent("from_utc");
    addIfPresent("to_utc");
    if (includePaging) {
        query.addQueryItem(QStringLiteral("page"), QString::number(filters.value("page").toInt(1)));
        query.addQueryItem(QStringLiteral("page_size"), QString::number(filters.value("page_size").toInt(100)));
        query.addQueryItem(QStringLiteral("sort"), filters.value("sort").toString(QStringLiteral("ts_desc")));
    }
    return query;
}

void OpsApiClient::applyLogFilterState(const QJsonObject &filters)
{
    if (m_selectedLogFilters == filters) {
        return;
    }
    m_selectedLogFilters = filters;
    emit selectedLogFiltersChanged();
}

void OpsApiClient::refreshSelectedLogs()
{
    const QJsonObject filters = m_selectedLogFilters.isEmpty()
        ? buildLogFilterState(QString(), QString(), QString(), QString(), QString(), QString(), QString(), QString(), QString(), 1, 100, QStringLiteral("ts_desc"))
        : m_selectedLogFilters;
    const QString searchSuffix = QStringLiteral("?") + buildLogQuery(filters, true).toString(QUrl::FullyEncoded);
    getJson("/api/logs/search" + searchSuffix, [this](const QJsonObject &payload) {
        m_logSearchResult = payload;
        m_logs = payload.value("items").toArray();
        emit logSearchResultChanged();
        emit logsChanged();
    });

    const QUrlQuery trendQuery = buildLogQuery(filters, false);
    const QString trendSuffix = trendQuery.isEmpty() ? QString() : QStringLiteral("?") + trendQuery.toString(QUrl::FullyEncoded);
    getJson("/api/logs/trends" + trendSuffix, [this](const QJsonObject &payload) {
        m_logTrend = payload;
        emit logTrendChanged();
    });
}

void OpsApiClient::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }
    m_busy = value;
    emit busyChanged();
}

void OpsApiClient::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}

void OpsApiClient::getJson(const QString &path, const std::function<void(const QJsonObject &)> &onObject)
{
    ++m_inFlight;
    setBusy(true);
    QNetworkRequest request(QUrl(m_baseUrl + path));
    auto *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onObject]() {
        --m_inFlight;
        setBusy(m_inFlight > 0);
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            setLastError(reply->errorString());
            reply->deleteLater();
            return;
        }
        const auto doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            setLastError(QStringLiteral("Invalid JSON response"));
            reply->deleteLater();
            return;
        }
        setLastError(QString());
        onObject(doc.object());
        reply->deleteLater();
    });
}

void OpsApiClient::postJson(const QString &path, const std::function<void(const QJsonObject &)> &onObject)
{
    ++m_inFlight;
    setBusy(true);
    QNetworkRequest request(QUrl(m_baseUrl + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto *reply = m_network.post(request, QByteArray("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, onObject]() {
        --m_inFlight;
        setBusy(m_inFlight > 0);
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            setLastError(reply->errorString());
            reply->deleteLater();
            return;
        }
        const auto doc = QJsonDocument::fromJson(body);
        if (doc.isObject()) {
            setLastError(QString());
            onObject(doc.object());
        }
        reply->deleteLater();
    });
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

void OpsApiClient::refreshOverview()
{
    getJson("/api/overview", [this](const QJsonObject &payload) {
        m_overview = payload;
        m_overviewKpis = payload.value("kpis").toObject();
        emit overviewChanged();
    });
}

void OpsApiClient::refreshRuns()
{
    getJson("/api/loadtests/runs?limit=100", [this](const QJsonObject &payload) {
        m_runs = payload.value("items").toArray();
        emit runsChanged();
    });
}

void OpsApiClient::refreshLogs(const QString &service, const QString &level, const QString &traceId)
{
    refreshLogSearch(service, QString(), level, QString(), traceId);
}

void OpsApiClient::refreshLogSearch(const QString &service,
                                    const QString &instance,
                                    const QString &level,
                                    const QString &event,
                                    const QString &traceId,
                                    const QString &requestId,
                                    const QString &keyword,
                                    const QString &fromUtc,
                                    const QString &toUtc,
                                    int page,
                                    int pageSize,
                                    const QString &sort)
{
    const QJsonObject filters = buildLogFilterState(
        service, instance, level, event, traceId, requestId, keyword, fromUtc, toUtc, page, pageSize, sort);
    applyLogFilterState(filters);
    const QString suffix = QStringLiteral("?") + buildLogQuery(filters, true).toString(QUrl::FullyEncoded);
    getJson("/api/logs/search" + suffix, [this](const QJsonObject &payload) {
        m_logSearchResult = payload;
        m_logs = payload.value("items").toArray();
        emit logSearchResultChanged();
        emit logsChanged();
    });
}

void OpsApiClient::refreshLogTrend(const QString &service,
                                   const QString &instance,
                                   const QString &level,
                                   const QString &event,
                                   const QString &traceId,
                                   const QString &requestId,
                                   const QString &keyword,
                                   const QString &fromUtc,
                                   const QString &toUtc)
{
    const QJsonObject filters = buildLogFilterState(
        service, instance, level, event, traceId, requestId, keyword, fromUtc, toUtc, 1, 100, QStringLiteral("ts_desc"));
    const QUrlQuery query = buildLogQuery(filters, false);
    const QString suffix = query.isEmpty() ? QString() : QStringLiteral("?") + query.toString(QUrl::FullyEncoded);
    getJson("/api/logs/trends" + suffix, [this](const QJsonObject &payload) {
        m_logTrend = payload;
        emit logTrendChanged();
    });
}

void OpsApiClient::refreshTrace(const QString &traceId)
{
    if (traceId.isEmpty()) {
        return;
    }
    getJson("/api/traces/" + traceId, [this](const QJsonObject &payload) {
        m_selectedTrace = payload;
        emit selectedTraceChanged();
    });
}

void OpsApiClient::refreshServices()
{
    getJson("/api/metrics/services", [this](const QJsonObject &payload) {
        m_services = payload.value("items").toArray();
        emit servicesChanged();
    });
}

void OpsApiClient::refreshServiceTrend(const QString &serviceName,
                                       const QString &instance,
                                       const QString &fromUtc,
                                       const QString &toUtc)
{
    if (serviceName.isEmpty()) {
        return;
    }
    QUrlQuery query;
    if (!instance.isEmpty()) {
        query.addQueryItem("instance", instance);
    }
    if (!fromUtc.isEmpty()) {
        query.addQueryItem("from_utc", fromUtc);
    }
    if (!toUtc.isEmpty()) {
        query.addQueryItem("to_utc", toUtc);
    }
    const QString suffix = query.isEmpty() ? QString() : "?" + query.toString(QUrl::FullyEncoded);
    getJson("/api/metrics/services/" + serviceName + "/trend" + suffix, [this](const QJsonObject &payload) {
        m_serviceTrend = payload;
        emit serviceTrendChanged();
    });
}

void OpsApiClient::refreshLoadtestTrend(const QString &fromUtc,
                                        const QString &toUtc,
                                        const QString &groupBy)
{
    QUrlQuery query;
    if (!fromUtc.isEmpty()) {
        query.addQueryItem("from_utc", fromUtc);
    }
    if (!toUtc.isEmpty()) {
        query.addQueryItem("to_utc", toUtc);
    }
    query.addQueryItem("group_by", groupBy);
    const QString suffix = "?" + query.toString(QUrl::FullyEncoded);
    getJson("/api/loadtests/trends" + suffix, [this](const QJsonObject &payload) {
        m_loadtestTrend = payload;
        emit loadtestTrendChanged();
    });
}

void OpsApiClient::refreshAlerts()
{
    getJson("/api/alerts", [this](const QJsonObject &payload) {
        m_alerts = payload.value("items").toArray();
        emit alertsChanged();
    });
}

void OpsApiClient::refreshDataSources()
{
    getJson("/api/admin/data-sources", [this](const QJsonObject &payload) {
        m_dataSources = payload;
        emit dataSourcesChanged();
    });
}

void OpsApiClient::selectRun(const QString &runId)
{
    if (runId.isEmpty()) {
        return;
    }
    getJson("/api/loadtests/runs/" + runId, [this](const QJsonObject &payload) {
        m_selectedRun = payload;
        emit selectedRunChanged();
    });
}

void OpsApiClient::selectService(const QString &serviceName, const QString &instance)
{
    m_selectedService = {
        {"service_name", serviceName},
        {"instance", instance},
    };
    emit selectedServiceChanged();
    refreshServiceTrend(serviceName, instance);
}

void OpsApiClient::collectNow()
{
    postJson("/api/admin/collect", [this](const QJsonObject &) {
        refreshOverview();
        refreshServices();
        if (!m_selectedService.value("service_name").toString().isEmpty()) {
            refreshServiceTrend(
                m_selectedService.value("service_name").toString(),
                m_selectedService.value("instance").toString());
        }
        refreshAlerts();
    });
}

void OpsApiClient::importReports()
{
    postJson("/api/admin/import/reports", [this](const QJsonObject &) {
        refreshOverview();
        refreshRuns();
        refreshLoadtestTrend();
    });
}

void OpsApiClient::importLogs()
{
    postJson("/api/admin/import/logs", [this](const QJsonObject &) {
        refreshOverview();
        refreshSelectedLogs();
        refreshServices();
    });
}
