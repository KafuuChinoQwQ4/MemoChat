from __future__ import annotations

from fastapi import APIRouter

from Memo_ops.server.ops_common.ingest import import_logs, import_reports, rebuild_trace_index
from Memo_ops.server.ops_common.monitoring import collect_snapshots, data_source_health
from Memo_ops.server.ops_server.runtime import OpsServerRuntime


def create_admin_router(runtime: OpsServerRuntime) -> APIRouter:
    router = APIRouter()

    @router.post("/api/admin/import/reports")
    def api_import_reports() -> dict:
        return runtime.guarded(lambda: import_reports(runtime.config))

    @router.post("/api/admin/import/logs")
    def api_import_logs() -> dict:
        return runtime.guarded(lambda: import_logs(runtime.config))

    @router.post("/api/admin/rebuild-index")
    def api_rebuild_index() -> dict:
        return runtime.guarded(lambda: {"status": "ok", "result": rebuild_trace_index(runtime.config)})

    @router.get("/api/admin/data-sources")
    def api_data_sources() -> dict:
        return data_source_health(runtime.config)

    @router.post("/api/admin/collect")
    def api_collect() -> dict:
        return runtime.guarded(lambda: {"items": collect_snapshots(runtime.config)})

    return router
