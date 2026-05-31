#include "OpsApiClient.h"

#include "OpsApiTransport.h"

void OpsApiClient::refreshOverview()
{
    m_transport->getJson("/api/overview",
                         [this](const QJsonObject& payload)
                         {
                             m_overview = payload;
                             m_overviewKpis = payload.value("kpis").toObject();
                             emit overviewChanged();
                         });
}

void OpsApiClient::refreshAlerts()
{
    m_transport->getJson("/api/alerts",
                         [this](const QJsonObject& payload)
                         {
                             m_alerts = payload.value("items").toArray();
                             emit alertsChanged();
                         });
}
