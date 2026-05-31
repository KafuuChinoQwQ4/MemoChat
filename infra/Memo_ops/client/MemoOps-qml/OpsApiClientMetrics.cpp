#include "OpsApiClient.h"

#include "OpsApiTransport.h"

#include <QUrl>
#include <QUrlQuery>

void OpsApiClient::refreshServices()
{
    m_transport->getJson("/api/metrics/services",
                         [this](const QJsonObject& payload)
                         {
                             m_services = payload.value("items").toArray();
                             emit servicesChanged();
                         });
}

void OpsApiClient::refreshServiceTrend(const QString& serviceName,
                                       const QString& instance,
                                       const QString& fromUtc,
                                       const QString& toUtc)
{
    if (serviceName.isEmpty())
    {
        return;
    }

    QUrlQuery query;
    if (!instance.isEmpty())
    {
        query.addQueryItem("instance", instance);
    }
    if (!fromUtc.isEmpty())
    {
        query.addQueryItem("from_utc", fromUtc);
    }
    if (!toUtc.isEmpty())
    {
        query.addQueryItem("to_utc", toUtc);
    }

    const QString suffix = query.isEmpty() ? QString() : "?" + query.toString(QUrl::FullyEncoded);
    m_transport->getJson("/api/metrics/services/" + serviceName + "/trend" + suffix,
                         [this](const QJsonObject& payload)
                         {
                             m_serviceTrend = payload;
                             emit serviceTrendChanged();
                         });
}

void OpsApiClient::selectService(const QString& serviceName, const QString& instance)
{
    m_selectedService = {
        {"service_name", serviceName},
        {"instance", instance},
    };
    emit selectedServiceChanged();
    refreshServiceTrend(serviceName, instance);
}

void OpsApiClient::refreshSystemMetrics()
{
    m_transport->getJson("/api/system/metrics",
                         [this](const QJsonObject& payload)
                         {
                             m_systemMetrics = payload.value("services").toArray();
                             emit systemMetricsChanged();
                         });
}
