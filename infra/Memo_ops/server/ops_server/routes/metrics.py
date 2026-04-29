from __future__ import annotations

import json
from typing import Any

from fastapi import APIRouter, Body

from Memo_ops.server.ops_common.analytics import query_service_trend
from Memo_ops.server.ops_common.monitoring import build_active_alerts
from Memo_ops.server.ops_common.repositories import (
    get_service_minute_series,
    get_service_snapshot_history,
    list_recent_alert_source_snapshots,
    list_recent_service_snapshots,
)
from Memo_ops.server.ops_common.storage import postgres_conn, prefixed_key, redis_client
from Memo_ops.server.ops_server.runtime import OpsServerRuntime


def _normalize_alertmanager_alert(alert: dict[str, Any]) -> dict:
    labels = alert.get("labels") if isinstance(alert.get("labels"), dict) else {}
    annotations = alert.get("annotations") if isinstance(alert.get("annotations"), dict) else {}
    return {
        "source": "alertmanager",
        "status": str(alert.get("status", "")),
        "severity": str(labels.get("severity", "warning")),
        "service": str(labels.get("service", labels.get("job", "unknown"))),
        "instance": str(labels.get("instance", "")),
        "alertname": str(labels.get("alertname", "")),
        "message": str(annotations.get("summary", annotations.get("description", ""))),
        "description": str(annotations.get("description", "")),
        "starts_at": str(alert.get("startsAt", "")),
        "ends_at": str(alert.get("endsAt", "")),
        "generator_url": str(alert.get("generatorURL", "")),
        "fingerprint": str(alert.get("fingerprint", "")),
    }


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
            alertmanager_raw = client.get(prefixed_key(runtime.config["redis"], "alerts:alertmanager"))
            alertmanager_items = json.loads(alertmanager_raw) if alertmanager_raw else []
            raw = client.get(prefixed_key(runtime.config["redis"], "alerts:active"))
            if raw:
                return {"items": json.loads(raw) + alertmanager_items}
            with postgres_conn(runtime.config["postgresql"]) as conn:
                snapshots = list_recent_alert_source_snapshots(conn)
            return {"items": build_active_alerts(runtime.config, snapshots) + alertmanager_items}

        return runtime.guarded(action)

    @router.post("/api/alerts/alertmanager")
    def api_alertmanager_webhook(payload: dict = Body(...)) -> dict:
        def action():
            alerts = payload.get("alerts") if isinstance(payload.get("alerts"), list) else []
            normalized = [
                _normalize_alertmanager_alert(alert)
                for alert in alerts
                if isinstance(alert, dict) and str(alert.get("status", "")) == "firing"
            ]
            client = redis_client(runtime.config["redis"])
            client.set(
                prefixed_key(runtime.config["redis"], "alerts:alertmanager"),
                json.dumps(normalized, ensure_ascii=False, default=str),
                ex=3600,
            )
            return {"accepted": len(alerts), "active": len(normalized)}

        return runtime.guarded(action)

    return router
