#pragma once

#include <QJsonObject>
#include <QUrlQuery>

namespace OpsLogQueryBuilder
{
QJsonObject buildLogFilterState(const QString& service,
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
                                const QString& sort);

QUrlQuery buildLogQuery(const QJsonObject& filters, bool includePaging);
} // namespace OpsLogQueryBuilder
