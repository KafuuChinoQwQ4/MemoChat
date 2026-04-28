from __future__ import annotations

from typing import Any

from .storage import fetch_all
from .time_windows import normalize_window


def query_service_trend(
    conn,
    service_name: str,
    instance: str = "",
    from_utc: str | None = None,
    to_utc: str | None = None,
) -> dict[str, Any]:
    start, end = normalize_window(from_utc, to_utc, 6)
    clauses = ["service_name = %s", "minute_utc >= %s", "minute_utc <= %s"]
    params: list[Any] = [service_name, start.strftime("%Y-%m-%d %H:%M:%S"), end.strftime("%Y-%m-%d %H:%M:%S")]
    aggregate = {
        "cpu_percent_avg": "AVG(cpu_percent_avg)",
        "memory_rss_bytes_avg": "AVG(memory_rss_bytes_avg)",
        "online_users_avg": "AVG(online_users_avg)",
        "qps_avg": "AVG(qps_avg)",
        "error_rate_avg": "AVG(error_rate_avg)",
        "latency_p95_ms_avg": "AVG(latency_p95_ms_avg)",
    }
    if instance:
        clauses.append("instance_name = %s")
        params.append(instance)
    select_expr = ", ".join([f"{expr} AS {alias}" for alias, expr in aggregate.items()])
    rows = fetch_all(
        conn,
        f"""
        SELECT minute_utc, {select_expr}
          FROM ops_service_metric_minute
         WHERE {' AND '.join(clauses)}
         GROUP BY minute_utc
         ORDER BY minute_utc ASC
        """,
        params,
    )
    return {
        "service_name": service_name,
        "instance": instance,
        "from_utc": start.isoformat(),
        "to_utc": end.isoformat(),
        "items": rows,
    }
