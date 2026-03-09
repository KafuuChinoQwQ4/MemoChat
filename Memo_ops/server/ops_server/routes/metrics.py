from __future__ import annotations

import json

from fastapi import APIRouter

from Memo_ops.server.ops_common.analytics import query_service_trend
from Memo_ops.server.ops_common.monitoring import build_active_alerts
from Memo_ops.server.ops_common.repositories import (
    get_service_minute_series,
    get_service_snapshot_history,
    list_recent_alert_source_snapshots,
    list_recent_service_snapshots,
)
from Memo_ops.server.ops_common.storage import mysql_conn, prefixed_key, redis_client
from Memo_ops.server.ops_server.runtime import OpsServerRuntime


def create_metrics_router(runtime: OpsServerRuntime) -> APIRouter:
    router = APIRouter()

    @router.get("/api/metrics/services")
    def api_metrics_services() -> dict:
        return runtime.with_conn(lambda conn: {"items": list_recent_service_snapshots(conn)})

    @router.get("/api/metrics/services/{service_name}")
    def api_metrics_service(service_name: str) -> dict:
        return runtime.with_conn(
            lambda conn: {
                "snapshots": get_service_snapshot_history(conn, service_name),
                "minute_series": get_service_minute_series(conn, service_name),
            }
        )

    @router.get("/api/metrics/services/{service_name}/trend")
    def api_metrics_service_trend(
        service_name: str,
        instance: str = "",
        from_utc: str = "",
        to_utc: str = "",
    ) -> dict:
        return runtime.with_conn(
            lambda conn: query_service_trend(
                conn,
                service_name=service_name,
                instance=instance,
                from_utc=from_utc or None,
                to_utc=to_utc or None,
            )
        )

    @router.get("/api/alerts")
    def api_alerts() -> dict:
        def action():
            client = redis_client(runtime.config["redis"])
            raw = client.get(prefixed_key(runtime.config["redis"], "alerts:active"))
            if raw:
                return {"items": json.loads(raw)}
            with mysql_conn(runtime.config["mysql"]) as conn:
                snapshots = list_recent_alert_source_snapshots(conn)
            return {"items": build_active_alerts(runtime.config, snapshots)}

        return runtime.guarded(action)

    return router
