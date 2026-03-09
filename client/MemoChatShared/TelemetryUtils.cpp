#include "TelemetryUtils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QSysInfo>
#include <QTimer>
#include <QUuid>

namespace {

QString appConfigPath()
{
    return QCoreApplication::applicationDirPath() + "/config.ini";
}

QString normalizeHexId(const QString &raw)
{
    QString out;
    out.reserve(raw.size());
    for (const QChar ch : raw) {
        if (ch.isDigit() || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f'))
            || (ch >= QLatin1Char('A') && ch <= QLatin1Char('F'))) {
            out.append(ch.toLower());
        }
    }
    return out;
}

QString defaultServiceName()
{
    const QString appName = QCoreApplication::applicationName().trimmed();
    if (!appName.isEmpty()) {
        return appName;
    }
    return QStringLiteral("MemoChatClient");
}

QNetworkAccessManager *telemetryManager()
{
    static QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
    return manager;
}

} // namespace

ClientTelemetryConfig loadClientTelemetryConfig()
{
    ClientTelemetryConfig cfg;
    QSettings settings(appConfigPath(), QSettings::IniFormat);
    cfg.enabled = settings.value("Telemetry/Enabled", true).toBool();
    cfg.endpoint = settings.value("Telemetry/OtlpEndpoint", cfg.endpoint).toString().trimmed();
    cfg.protocol = settings.value("Telemetry/Protocol", cfg.protocol).toString().trimmed().toLower();
    cfg.exportLogs = settings.value("Telemetry/ExportLogs", true).toBool();
    cfg.exportTraces = settings.value("Telemetry/ExportTraces", true).toBool();
    cfg.exportMetrics = settings.value("Telemetry/ExportMetrics", false).toBool();
    cfg.serviceName = settings.value("Telemetry/ServiceName").toString().trimmed();
    if (cfg.serviceName.isEmpty()) {
        cfg.serviceName = defaultServiceName();
    }
    cfg.serviceNamespace = settings.value("Telemetry/ServiceNamespace", cfg.serviceNamespace).toString().trimmed();
    if (cfg.serviceNamespace.isEmpty()) {
        cfg.serviceNamespace = QStringLiteral("memochat");
    }
    cfg.serviceInstance = QStringLiteral("%1@%2:%3")
                              .arg(cfg.serviceName,
                                   QSysInfo::machineHostName().isEmpty() ? QStringLiteral("localhost") : QSysInfo::machineHostName(),
                                   QString::number(QCoreApplication::applicationPid()));
    return cfg;
}

QString clientServiceName()
{
    return loadClientTelemetryConfig().serviceName;
}

QString clientServiceInstance()
{
    return loadClientTelemetryConfig().serviceInstance;
}

QString newTraceId()
{
    return normalizeHexId(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString newRequestId()
{
    return newTraceId();
}

QString newSpanId()
{
    return newTraceId().left(16);
}

void applyTraceHeaders(QNetworkRequest &request, QString *traceId, QString *requestId, QString *spanId)
{
    QString localTraceId = traceId && !traceId->trimmed().isEmpty() ? traceId->trimmed() : newTraceId();
    QString localRequestId = requestId && !requestId->trimmed().isEmpty() ? requestId->trimmed() : newRequestId();
    QString localSpanId = spanId && !spanId->trimmed().isEmpty() ? spanId->trimmed() : newSpanId();

    request.setRawHeader("X-Trace-Id", localTraceId.toUtf8());
    request.setRawHeader("X-Request-Id", localRequestId.toUtf8());
    request.setRawHeader("X-Span-Id", localSpanId.toUtf8());

    if (traceId) {
        *traceId = localTraceId;
    }
    if (requestId) {
        *requestId = localRequestId;
    }
    if (spanId) {
        *spanId = localSpanId;
    }
}

void exportZipkinSpan(const QString &name,
                      const QString &kind,
                      const QString &traceId,
                      const QString &spanId,
                      const QString &parentSpanId,
                      qint64 startTimeMs,
                      qint64 durationMs,
                      const QVariantMap &attributes)
{
    const ClientTelemetryConfig cfg = loadClientTelemetryConfig();
    if (!cfg.enabled || !cfg.exportTraces || cfg.endpoint.isEmpty() || traceId.isEmpty() || spanId.isEmpty()) {
        return;
    }

    QJsonObject spanObj;
    spanObj.insert("traceId", traceId);
    spanObj.insert("id", spanId);
    if (!parentSpanId.isEmpty()) {
        spanObj.insert("parentId", parentSpanId);
    }
    spanObj.insert("name", name);
    spanObj.insert("kind", kind);
    spanObj.insert("timestamp", QString::number(startTimeMs * 1000));
    spanObj.insert("duration", QString::number(qMax<qint64>(0, durationMs) * 1000));

    QJsonObject localEndpoint;
    localEndpoint.insert("serviceName", cfg.serviceName);
    spanObj.insert("localEndpoint", localEndpoint);

    QJsonObject tags;
    tags.insert("service.instance.id", cfg.serviceInstance);
    tags.insert("service.namespace", cfg.serviceNamespace);
    for (auto it = attributes.cbegin(); it != attributes.cend(); ++it) {
        tags.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    spanObj.insert("tags", tags);

    QJsonArray payload;
    payload.append(spanObj);

    QNetworkRequest request(QUrl(cfg.endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = telemetryManager()->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    if (!reply) {
        return;
    }
    QObject::connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}
