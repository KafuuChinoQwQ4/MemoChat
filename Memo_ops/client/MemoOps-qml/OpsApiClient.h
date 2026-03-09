#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <functional>

class OpsApiClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl CONSTANT)
    Q_PROPERTY(QJsonObject overview READ overview NOTIFY overviewChanged)
    Q_PROPERTY(QJsonObject overviewKpis READ overviewKpis NOTIFY overviewChanged)
    Q_PROPERTY(QJsonArray runs READ runs NOTIFY runsChanged)
    Q_PROPERTY(QJsonObject selectedRun READ selectedRun NOTIFY selectedRunChanged)
    Q_PROPERTY(QJsonArray logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(QJsonObject logSearchResult READ logSearchResult NOTIFY logSearchResultChanged)
    Q_PROPERTY(QJsonObject logTrend READ logTrend NOTIFY logTrendChanged)
    Q_PROPERTY(QJsonObject serviceTrend READ serviceTrend NOTIFY serviceTrendChanged)
    Q_PROPERTY(QJsonObject loadtestTrend READ loadtestTrend NOTIFY loadtestTrendChanged)
    Q_PROPERTY(QJsonObject selectedTrace READ selectedTrace NOTIFY selectedTraceChanged)
    Q_PROPERTY(QJsonArray services READ services NOTIFY servicesChanged)
    Q_PROPERTY(QJsonObject selectedService READ selectedService NOTIFY selectedServiceChanged)
    Q_PROPERTY(QJsonObject selectedLogFilters READ selectedLogFilters NOTIFY selectedLogFiltersChanged)
    Q_PROPERTY(QJsonArray alerts READ alerts NOTIFY alertsChanged)
    Q_PROPERTY(QJsonObject dataSources READ dataSources NOTIFY dataSourcesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit OpsApiClient(QObject *parent = nullptr);

    QString baseUrl() const;
    QJsonObject overview() const;
    QJsonObject overviewKpis() const;
    QJsonArray runs() const;
    QJsonObject selectedRun() const;
    QJsonArray logs() const;
    QJsonObject logSearchResult() const;
    QJsonObject logTrend() const;
    QJsonObject serviceTrend() const;
    QJsonObject loadtestTrend() const;
    QJsonObject selectedTrace() const;
    QJsonArray services() const;
    QJsonObject selectedService() const;
    QJsonObject selectedLogFilters() const;
    QJsonArray alerts() const;
    QJsonObject dataSources() const;
    bool busy() const;
    QString lastError() const;

    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void refreshOverview();
    Q_INVOKABLE void refreshRuns();
    Q_INVOKABLE void refreshLogs(const QString &service = QString(),
                                 const QString &level = QString(),
                                 const QString &traceId = QString());
    Q_INVOKABLE void refreshLogSearch(const QString &service = QString(),
                                      const QString &instance = QString(),
                                      const QString &level = QString(),
                                      const QString &event = QString(),
                                      const QString &traceId = QString(),
                                      const QString &requestId = QString(),
                                      const QString &keyword = QString(),
                                      const QString &fromUtc = QString(),
                                      const QString &toUtc = QString(),
                                      int page = 1,
                                      int pageSize = 100,
                                      const QString &sort = QStringLiteral("ts_desc"));
    Q_INVOKABLE void refreshLogTrend(const QString &service = QString(),
                                     const QString &instance = QString(),
                                     const QString &level = QString(),
                                     const QString &event = QString(),
                                     const QString &traceId = QString(),
                                     const QString &requestId = QString(),
                                     const QString &keyword = QString(),
                                     const QString &fromUtc = QString(),
                                     const QString &toUtc = QString());
    Q_INVOKABLE void refreshTrace(const QString &traceId);
    Q_INVOKABLE void refreshServices();
    Q_INVOKABLE void refreshServiceTrend(const QString &serviceName,
                                         const QString &instance = QString(),
                                         const QString &fromUtc = QString(),
                                         const QString &toUtc = QString());
    Q_INVOKABLE void refreshLoadtestTrend(const QString &fromUtc = QString(),
                                          const QString &toUtc = QString(),
                                          const QString &groupBy = QStringLiteral("day"));
    Q_INVOKABLE void refreshAlerts();
    Q_INVOKABLE void refreshDataSources();
    Q_INVOKABLE void selectRun(const QString &runId);
    Q_INVOKABLE void selectService(const QString &serviceName, const QString &instance = QString());
    Q_INVOKABLE void collectNow();
    Q_INVOKABLE void importReports();
    Q_INVOKABLE void importLogs();

signals:
    void overviewChanged();
    void runsChanged();
    void selectedRunChanged();
    void logsChanged();
    void logSearchResultChanged();
    void logTrendChanged();
    void serviceTrendChanged();
    void loadtestTrendChanged();
    void selectedTraceChanged();
    void servicesChanged();
    void selectedServiceChanged();
    void selectedLogFiltersChanged();
    void alertsChanged();
    void dataSourcesChanged();
    void busyChanged();
    void lastErrorChanged();

private:
    void getJson(const QString &path, const std::function<void(const QJsonObject &)> &onObject);
    void postJson(const QString &path, const std::function<void(const QJsonObject &)> &onObject);
    QJsonObject buildLogFilterState(const QString &service,
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
                                    const QString &sort) const;
    QUrlQuery buildLogQuery(const QJsonObject &filters, bool includePaging) const;
    void applyLogFilterState(const QJsonObject &filters);
    void refreshSelectedLogs();
    void setBusy(bool value);
    void setLastError(const QString &message);

    QString m_baseUrl;
    QNetworkAccessManager m_network;
    QJsonObject m_overview;
    QJsonObject m_overviewKpis;
    QJsonArray m_runs;
    QJsonObject m_selectedRun;
    QJsonArray m_logs;
    QJsonObject m_logSearchResult;
    QJsonObject m_logTrend;
    QJsonObject m_serviceTrend;
    QJsonObject m_loadtestTrend;
    QJsonObject m_selectedTrace;
    QJsonArray m_services;
    QJsonObject m_selectedService;
    QJsonObject m_selectedLogFilters;
    QJsonArray m_alerts;
    QJsonObject m_dataSources;
    bool m_busy = false;
    int m_inFlight = 0;
    QString m_lastError;
};
