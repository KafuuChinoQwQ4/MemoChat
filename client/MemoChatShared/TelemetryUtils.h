#ifndef TELEMETRYUTILS_H
#define TELEMETRYUTILS_H

#include <QNetworkRequest>
#include <QString>
#include <QVariantMap>

struct ClientTelemetryConfig {
    bool enabled = true;
    QString endpoint = QStringLiteral("http://127.0.0.1:9411/api/v2/spans");
    QString protocol = QStringLiteral("zipkin-json");
    bool exportLogs = true;
    bool exportTraces = true;
    bool exportMetrics = false;
    QString serviceName;
    QString serviceNamespace = QStringLiteral("memochat");
    QString serviceInstance;
};

ClientTelemetryConfig loadClientTelemetryConfig();
QString clientServiceName();
QString clientServiceInstance();
QString newTraceId();
QString newRequestId();
QString newSpanId();
void applyTraceHeaders(QNetworkRequest &request, QString *traceId = nullptr, QString *requestId = nullptr, QString *spanId = nullptr);
void exportZipkinSpan(const QString &name,
                      const QString &kind,
                      const QString &traceId,
                      const QString &spanId,
                      const QString &parentSpanId,
                      qint64 startTimeMs,
                      qint64 durationMs,
                      const QVariantMap &attributes = QVariantMap());

#endif // TELEMETRYUTILS_H
