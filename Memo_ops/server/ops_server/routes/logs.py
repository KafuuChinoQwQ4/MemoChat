from __future__ import annotations

from fastapi import APIRouter, Query

from Memo_ops.server.ops_common.analytics import query_logs, query_log_trends
from Memo_ops.server.ops_common.repositories import get_trace_summary, list_trace_logs
from Memo_ops.server.ops_server.runtime import OpsServerRuntime


def create_logs_router(runtime: OpsServerRuntime) -> APIRouter:
    router = APIRouter()

    @router.get("/api/logs/search")
    def api_log_search(
        service: str = "",
        instance: str = "",
        level: str = "",
        trace_id: str = "",
        request_id: str = "",
        event: str = "",
        keyword: str = "",
        from_utc: str = "",
        to_utc: str = "",
        page: int = Query(1, ge=1),
        page_size: int = Query(100, ge=1, le=500),
        sort: str = Query("ts_desc", pattern="^(ts_desc|ts_asc)$"),
    ) -> dict:
        return runtime.with_conn(
            lambda conn: query_logs(
                conn,
                service=service,
                instance=instance,
                level=level,
                trace_id=trace_id,
                request_id=request_id,
                event=event,
                keyword=keyword,
                from_utc=from_utc or None,
                to_utc=to_utc or None,
                page=page,
                page_size=page_size,
                sort=sort,
            )
        )

    @router.get("/api/logs/trends")
    def api_log_trends(
        service: str = "",
        instance: str = "",
        level: str = "",
        trace_id: str = "",
        request_id: str = "",
        event: str = "",
        keyword: str = "",
        from_utc: str = "",
        to_utc: str = "",
    ) -> dict:
        return runtime.with_conn(
            lambda conn: query_log_trends(
                conn,
                service=service,
                instance=instance,
                level=level,
                trace_id=trace_id,
                request_id=request_id,
                event=event,
                keyword=keyword,
                from_utc=from_utc or None,
                to_utc=to_utc or None,
            )
        )

    @router.get("/api/traces/{trace_id}")
    def api_trace(trace_id: str) -> dict:
        return runtime.with_conn(
            lambda conn: {
                "trace": get_trace_summary(conn, trace_id),
                "logs": list_trace_logs(conn, trace_id),
                "tempo_url": f"{runtime.config['observability']['tempo'].rstrip('/')}/trace/{trace_id}",
            }
        )

    return router
