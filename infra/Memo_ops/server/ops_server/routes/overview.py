from __future__ import annotations

from fastapi import APIRouter

from Memo_ops.server.ops_common.analytics import build_overview_payload
from Memo_ops.server.ops_common.monitoring import data_source_health
from Memo_ops.server.ops_server.runtime import OpsServerRuntime


def create_overview_router(runtime: OpsServerRuntime) -> APIRouter:
    router = APIRouter()

    @router.get("/api/overview")
    def api_overview() -> dict:
        return runtime.with_conn(
            lambda conn: {
                **build_overview_payload(conn, runtime.config),
                "data_sources": data_source_health(runtime.config),
            }
        )

    return router
