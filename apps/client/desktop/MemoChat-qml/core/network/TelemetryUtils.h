#ifndef TELEMETRYUTILS_H
#define TELEMETRYUTILS_H

#include <QNetworkRequest>
#include <QString>
#include <QUrl>
#include <QVariantMap>

struct ClientTelemetryConfig
{
    bool enabled = false;
    QString endpoint;
    QString protocol = QStringLiteral("zipkin-json");
    bool exportLogs = false;
    bool exportTraces = false;
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
void applyTraceHeaders(QNetworkRequest& request,
                       QString* traceId = nullptr,
                       QString* requestId = nullptr,
                       QString* spanId = nullptr);
QString redactedUrlForTelemetry(const QUrl& url);
void exportZipkinSpan(const QString& name,
                      const QString& kind,
                      const QString& traceId,
                      const QString& spanId,
                      const QString& parentSpanId,
                      qint64 startTimeMs,
                      qint64 durationMs,
                      const QVariantMap& attributes = QVariantMap());

#endif // TELEMETRYUTILS_H
