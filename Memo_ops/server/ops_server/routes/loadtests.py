from __future__ import annotations

from fastapi import APIRouter, Query

from Memo_ops.server.ops_common.analytics import query_loadtest_trends
from Memo_ops.server.ops_common.repositories import (
    get_loadtest_run,
    list_loadtest_cases,
    list_loadtest_error_buckets,
    list_loadtest_runs,
)
from Memo_ops.server.ops_server.runtime import OpsServerRuntime


def create_loadtests_router(runtime: OpsServerRuntime) -> APIRouter:
    router = APIRouter()

    @router.get("/api/loadtests/runs")
    def api_loadtest_runs(limit: int = Query(50, ge=1, le=500)) -> dict:
        return runtime.with_conn(lambda conn: {"items": list_loadtest_runs(conn, limit)})

    @router.get("/api/loadtests/runs/{run_id}")
    def api_loadtest_run(run_id: str) -> dict:
        return runtime.with_conn(
            lambda conn: {
                "run": get_loadtest_run(conn, run_id),
                "cases": list_loadtest_cases(conn, run_id),
                "errors": list_loadtest_error_buckets(conn, run_id),
            }
        )

    @router.get("/api/loadtests/trends")
    def api_loadtests_trend(
        from_utc: str = "",
        to_utc: str = "",
        group_by: str = Query("day", pattern="^(day|suite)$"),
    ) -> dict:
        return runtime.with_conn(
            lambda conn: query_loadtest_trends(
                conn,
                from_utc=from_utc or None,
                to_utc=to_utc or None,
                group_by=group_by,
            )
        )

    return router
