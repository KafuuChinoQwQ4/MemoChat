#include "OpsApiClient.h"

#include "OpsApiTransport.h"
#include "OpsLogQueryBuilder.h"

#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

void OpsApiClient::applyLogFilterState(const QJsonObject& filters)
{
    if (m_selectedLogFilters == filters)
    {
        return;
    }
    m_selectedLogFilters = filters;
    emit selectedLogFiltersChanged();
}

void OpsApiClient::refreshSelectedLogs()
{
    const QJsonObject filters =
        m_selectedLogFilters.isEmpty() ? OpsLogQueryBuilder::buildLogFilterState(QString(),
                                                                                 QString(),
                                                                                 QString(),
                                                                                 QString(),
                                                                                 QString(),
                                                                                 QString(),
                                                                                 QString(),
                                                                                 QString(),
                                                                                 QString(),
                                                                                 1,
                                                                                 100,
                                                                                 QStringLiteral("ts_desc"))
                                                                                 : m_selectedLogFilters;
    const QString searchSuffix =
        QStringLiteral("?") + OpsLogQueryBuilder::buildLogQuery(filters, true).toString(QUrl::FullyEncoded);
    m_transport->getJson("/api/logs/search" + searchSuffix,
                         [this](const QJsonObject& payload)
                         {
                             m_logSearchResult = payload;
                             m_logs = payload.value("items").toArray();
                             emit logSearchResultChanged();
                             emit logsChanged();
                         });

    const QUrlQuery trendQuery = OpsLogQueryBuilder::buildLogQuery(filters, false);
    const QString trendSuffix =
        trendQuery.isEmpty() ? QString() : QStringLiteral("?") + trendQuery.toString(QUrl::FullyEncoded);
    m_transport->getJson("/api/logs/trends" + trendSuffix,
                         [this](const QJsonObject& payload)
                         {
                             m_logTrend = payload;
                             emit logTrendChanged();
                         });
}

void OpsApiClient::refreshLogs(const QString& service, const QString& level, const QString& traceId)
{
    refreshLogSearch(service, QString(), level, QString(), traceId);
}

void OpsApiClient::refreshLogSearch(const QString& service,
                                    const QString& instance,
                                    const QString& level,
                                    const QString& event,
                                    const QString& traceId,
                                    const QString& requestId,
                                    const QString& keyword,
                                    const QString& fromUtc,
                                    const QString& toUtc,
                                    int page,
                                    int pageSize,
                                    const QString& sort)
{
    const QJsonObject filters = OpsLogQueryBuilder::buildLogFilterState(service,
                                                                        instance,
                                                                        level,
                                                                        event,
                                                                        traceId,
                                                                        requestId,
                                                                        keyword,
                                                                        fromUtc,
                                                                        toUtc,
                                                                        page,
                                                                        pageSize,
                                                                        sort);
    applyLogFilterState(filters);
    const QString suffix =
        QStringLiteral("?") + OpsLogQueryBuilder::buildLogQuery(filters, true).toString(QUrl::FullyEncoded);
    m_transport->getJson("/api/logs/search" + suffix,
                         [this](const QJsonObject& payload)
                         {
                             m_logSearchResult = payload;
                             m_logs = payload.value("items").toArray();
                             emit logSearchResultChanged();
                             emit logsChanged();
                         });
}

void OpsApiClient::refreshLogTrend(const QString& service,
                                   const QString& instance,
                                   const QString& level,
                                   const QString& event,
                                   const QString& traceId,
                                   const QString& requestId,
                                   const QString& keyword,
                                   const QString& fromUtc,
                                   const QString& toUtc)
{
    const QJsonObject filters = OpsLogQueryBuilder::buildLogFilterState(service,
                                                                        instance,
                                                                        level,
                                                                        event,
                                                                        traceId,
                                                                        requestId,
                                                                        keyword,
                                                                        fromUtc,
                                                                        toUtc,
                                                                        1,
                                                                        100,
                                                                        QStringLiteral("ts_desc"));
    const QUrlQuery query = OpsLogQueryBuilder::buildLogQuery(filters, false);
    const QString suffix = query.isEmpty() ? QString() : QStringLiteral("?") + query.toString(QUrl::FullyEncoded);
    m_transport->getJson("/api/logs/trends" + suffix,
                         [this](const QJsonObject& payload)
                         {
                             m_logTrend = payload;
                             emit logTrendChanged();
                         });
}

void OpsApiClient::refreshTrace(const QString& traceId)
{
    if (traceId.isEmpty())
    {
        return;
    }
    m_transport->getJson("/api/traces/" + traceId,
                         [this](const QJsonObject& payload)
                         {
                             m_selectedTrace = payload;
                             emit selectedTraceChanged();
                         });
}

void OpsApiClient::fetchTailLogs(const QString& service, const QString& level, int limit)
{
    QUrlQuery query;
    if (!service.isEmpty())
    {
        query.addQueryItem(QStringLiteral("service"), service);
    }
    if (!level.isEmpty())
    {
        query.addQueryItem(QStringLiteral("level"), level);
    }
    query.addQueryItem(QStringLiteral("limit"), QString::number(limit));

    m_transport->getJson("/api/logs/tail?" + query.toString(QUrl::FullyEncoded),
                         [this](const QJsonObject& payload)
                         {
                             m_tailLogs = payload.value("items").toArray();
                             emit tailLogsChanged();
                         });
}

void OpsApiClient::stopTailLogs()
{
    if (m_tailTimer)
    {
        m_tailTimer->stop();
    }
}
