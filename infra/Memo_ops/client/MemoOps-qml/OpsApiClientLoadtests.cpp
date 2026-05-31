#include "OpsApiClient.h"

#include "OpsApiTransport.h"

#include <QUrl>
#include <QUrlQuery>

void OpsApiClient::refreshRuns()
{
    m_transport->getJson("/api/loadtests/runs?limit=100",
                         [this](const QJsonObject& payload)
                         {
                             m_runs = payload.value("items").toArray();
                             emit runsChanged();
                         });
}

void OpsApiClient::selectRun(const QString& runId)
{
    if (runId.isEmpty())
    {
        return;
    }
    m_transport->getJson("/api/loadtests/runs/" + runId,
                         [this](const QJsonObject& payload)
                         {
                             m_selectedRun = payload;
                             emit selectedRunChanged();
                         });
}

void OpsApiClient::refreshLoadtestTrend(const QString& fromUtc, const QString& toUtc, const QString& groupBy)
{
    QUrlQuery query;
    if (!fromUtc.isEmpty())
    {
        query.addQueryItem("from_utc", fromUtc);
    }
    if (!toUtc.isEmpty())
    {
        query.addQueryItem("to_utc", toUtc);
    }
    query.addQueryItem("group_by", groupBy);

    const QString suffix = "?" + query.toString(QUrl::FullyEncoded);
    m_transport->getJson("/api/loadtests/trends" + suffix,
                         [this](const QJsonObject& payload)
                         {
                             m_loadtestTrend = payload;
                             emit loadtestTrendChanged();
                         });
}

void OpsApiClient::refreshLoadtestStatus(const QString& runId)
{
    if (runId.isEmpty())
    {
        return;
    }
    m_transport->getJson("/api/loadtests/run/" + runId + "/status",
                         [this](const QJsonObject& payload)
                         {
                             m_loadtestRunStatus = payload;
                             emit loadtestRunStatusChanged();
                         });
}

void OpsApiClient::startLoadtest(const QString& scenario, int warmup, int poolSize)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("scenario"), scenario);
    query.addQueryItem(QStringLiteral("warmup"), QString::number(warmup));
    query.addQueryItem(QStringLiteral("pool_size"), QString::number(poolSize));
    m_transport->postJsonWithQuery("/api/loadtests/run",
                                   query,
                                   [this](const QJsonObject& payload)
                                   {
                                       m_loadtestRunStatus = payload;
                                       emit loadtestRunStatusChanged();
                                   });
}
