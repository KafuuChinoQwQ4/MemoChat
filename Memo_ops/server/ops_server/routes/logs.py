from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Dict, List

from fastapi import APIRouter, Query

from Memo_ops.server.ops_common.analytics import query_logs, query_log_trends
from Memo_ops.server.ops_common.repositories import get_trace_summary, list_trace_logs
from Memo_ops.server.ops_server.runtime import OpsServerRuntime


def _tail_logs(runtime: OpsServerRuntime, service: str, level: str, limit: int) -> List[Dict[str, Any]]:
    results: List[Dict[str, Any]] = []
    log_roots = [
        Path(runtime.config.get("_package_root", ".")) / "artifacts" / "logs" / "services",
        Path(runtime.config.get("_package_root", ".")) / "logs",
    ]
    for root in log_roots:
        if not root.exists():
            continue
        # Build service name to folder mapping
        for log_dir in root.iterdir():
            if not log_dir.is_dir():
                continue
            if service and service.lower() not in log_dir.name.lower():
                continue
            # Find latest .log.json files
            json_files = sorted(log_dir.glob("*.log.json"), key=lambda x: x.stat().st_mtime, reverse=True)
            for f in json_files[:3]:
                try:
                    lines = f.read_text(encoding="utf-8", errors="replace").strip().split("\n")
                    for line in reversed(lines):
                        line = line.strip()
                        if not line:
                            continue
                        try:
                            entry = json.loads(line)
                        except json.JSONDecodeError:
                            continue
                        if level and entry.get("level", "").lower() != level.lower():
                            continue
                        results.append(entry)
                        if len(results) >= limit:
                            return results
                except Exception:
                    pass
    return results


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

    @router.get("/api/logs/tail")
    def api_log_tail(
        service: str = "",
        level: str = "",
        limit: int = Query(50, ge=1, le=500),
    ) -> dict:
        def action() -> dict:
            items = _tail_logs(runtime, service, level, limit)
            return {"items": items, "total": len(items), "service": service, "level": level}
        return runtime.guarded(action)

    return router
