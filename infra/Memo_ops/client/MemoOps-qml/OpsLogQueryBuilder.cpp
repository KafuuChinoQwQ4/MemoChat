#include "OpsLogQueryBuilder.h"

QJsonObject OpsLogQueryBuilder::buildLogFilterState(const QString& service,
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
    return {
        {"service", service},
        {"instance", instance},
        {"level", level},
        {"event", event},
        {"trace_id", traceId},
        {"request_id", requestId},
        {"keyword", keyword},
        {"from_utc", fromUtc},
        {"to_utc", toUtc},
        {"page", page},
        {"page_size", pageSize},
        {"sort", sort},
    };
}

QUrlQuery OpsLogQueryBuilder::buildLogQuery(const QJsonObject& filters, bool includePaging)
{
    QUrlQuery query;
    auto addIfPresent = [&query, &filters](const char* name)
    {
        const QString value = filters.value(QLatin1String(name)).toString();
        if (!value.isEmpty())
        {
            query.addQueryItem(QLatin1String(name), value);
        }
    };

    addIfPresent("service");
    addIfPresent("instance");
    addIfPresent("level");
    addIfPresent("event");
    addIfPresent("trace_id");
    addIfPresent("request_id");
    addIfPresent("keyword");
    addIfPresent("from_utc");
    addIfPresent("to_utc");

    if (includePaging)
    {
        query.addQueryItem(QStringLiteral("page"), QString::number(filters.value("page").toInt(1)));
        query.addQueryItem(QStringLiteral("page_size"), QString::number(filters.value("page_size").toInt(100)));
        query.addQueryItem(QStringLiteral("sort"), filters.value("sort").toString(QStringLiteral("ts_desc")));
    }

    return query;
}
