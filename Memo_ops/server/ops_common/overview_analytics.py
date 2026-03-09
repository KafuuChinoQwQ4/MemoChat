from __future__ import annotations

from datetime import timedelta
from typing import Any

from .loadtest_analytics import query_loadtest_trends
from .log_analytics import query_log_trends
from .repositories import (
    list_latest_service_health,
    list_overview_recent_runs,
    list_top_error_services,
    summarize_alert_rows,
)
from .time_windows import utc_now


def build_overview_payload(conn, config: dict[str, Any]) -> dict[str, Any]:
    overview_limit = int((config.get("platform", {}) or {}).get("overview_limit", 10))
    latest_service_rows = list_latest_service_health(conn)
    recent_runs = list_overview_recent_runs(conn, overview_limit)
    alert_summary = summarize_alert_rows(conn)
    top_error_services = list_top_error_services(conn)
    now = utc_now()
    log_trend = query_log_trends(
        conn,
        from_utc=(now - timedelta(hours=24)).isoformat(),
        to_utc=now.isoformat(),
    )
    loadtest_trend = query_loadtest_trends(
        conn,
        from_utc=(now - timedelta(days=7)).isoformat(),
        to_utc=now.isoformat(),
    )
    service_up = sum(1 for item in latest_service_rows if item.get("status") == "up")
    error_logs_last_hour = sum(int(item.get("error_count", 0) or 0) for item in top_error_services)
    return {
        "generated_at": now.isoformat(),
        "kpis": {
            "active_alerts": sum(int(item.get("count_rows", 0) or 0) for item in alert_summary),
            "online_services": service_up,
            "recent_runs": len(recent_runs),
            "error_logs_1h": error_logs_last_hour,
        },
        "service_health": latest_service_rows,
        "recent_runs": recent_runs,
        "alert_summary": alert_summary,
        "loadtest_trend": loadtest_trend,
        "log_trend": log_trend,
        "top_error_services": top_error_services,
    }
