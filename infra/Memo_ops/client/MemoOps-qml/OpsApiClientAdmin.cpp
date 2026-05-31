#include "OpsApiClient.h"

#include "OpsApiTransport.h"

void OpsApiClient::refreshDataSources()
{
    m_transport->getJson("/api/admin/data-sources",
                         [this](const QJsonObject& payload)
                         {
                             m_dataSources = payload;
                             emit dataSourcesChanged();
                         });
}

void OpsApiClient::collectNow()
{
    m_transport->postJson("/api/admin/collect",
                          [this](const QJsonObject&)
                          {
                              refreshOverview();
                              refreshServices();
                              if (!m_selectedService.value("service_name").toString().isEmpty())
                              {
                                  refreshServiceTrend(m_selectedService.value("service_name").toString(),
                                                      m_selectedService.value("instance").toString());
                              }
                              refreshAlerts();
                          });
}

void OpsApiClient::importReports()
{
    m_transport->postJson("/api/admin/import/reports",
                          [this](const QJsonObject&)
                          {
                              refreshOverview();
                              refreshRuns();
                              refreshLoadtestTrend();
                          });
}

void OpsApiClient::importLogs()
{
    m_transport->postJson("/api/admin/import/logs",
                          [this](const QJsonObject&)
                          {
                              refreshOverview();
                              refreshSelectedLogs();
                              refreshServices();
                          });
}
