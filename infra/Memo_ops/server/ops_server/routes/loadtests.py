from __future__ import annotations

import json
import os
import subprocess
import sys
import threading
import time
import uuid
from pathlib import Path
from typing import Any, Dict

from fastapi import APIRouter, Query, BackgroundTasks

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

    # In-memory run tracking (reset on server restart)
    _run_registry: Dict[str, Dict[str, Any]] = {}
    _run_lock = threading.Lock()

    def _find_report_dir(runtime: OpsServerRuntime) -> Path:
        # Try runtime config paths first
        report_dir = runtime.config.get("loadtest", {}).get("report_dir")
        if report_dir:
            p = Path(report_dir).resolve()
            if p.exists():
                return p
        # Fallback: look relative to repo root
        repo_root = Path(runtime.config.get("_package_root", "."))
        candidates = [
            repo_root / "Memo_ops" / "artifacts" / "loadtest" / "runtime" / "reports",
            repo_root / "artifacts" / "loadtest" / "runtime" / "reports",
            repo_root / "tools" / "loadtest" / "python-loadtest" / "reports",
            repo_root / "python-loadtest" / "reports",
        ]
        for c in candidates:
            if c.exists():
                return c
        return repo_root / "reports"

    def _find_runner(runtime: OpsServerRuntime) -> Path:
        runner_path = runtime.config.get("loadtest", {}).get("runner_path", "")
        if runner_path:
            p = Path(runner_path).resolve()
            if p.exists():
                return p
        repo_root = Path(runtime.config.get("_package_root", "."))
        candidates = [
            repo_root / "tools" / "loadtest" / "python-loadtest" / "py_loadtest.py",
            repo_root / "python-loadtest" / "py_loadtest.py",
        ]
        for c in candidates:
            if c.exists():
                return c
        return Path("py_loadtest.py")

    def _find_python() -> str:
        return runtime.config.get("loadtest", {}).get("python", "") or sys.executable

    def _scan_reports(report_dir: Path, since_mtime: float) -> list[dict]:
        reports = []
        if not report_dir.exists():
            return reports
        for f in sorted(report_dir.rglob("*.json"), key=lambda x: x.stat().st_mtime, reverse=True):
            try:
                content = json.loads(f.read_text(encoding="utf-8"))
                if f.stat().st_mtime > since_mtime:
                    reports.append({"file": str(f), "data": content})
            except Exception:
                pass
        return reports

    def _run_loadtest_bg(run_id: str, scenario: str, total: int, concurrency: int,
                          runner: Path, config_path: Path, report_dir: Path) -> None:
        started_at = time.strftime("%Y%m%d_%H%M%S", time.gmtime())
        suite_dir = report_dir / f"suite_{started_at}"
        suite_dir.mkdir(parents=True, exist_ok=True)

        cmd = [
            _find_python(),
            str(runner),
            "--scenario", scenario,
            "--config", str(config_path),
            "--total", str(total),
            "--concurrency", str(concurrency),
            "--report-dir", str(suite_dir),
        ]
        try:
            env = {**os.environ}
            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=str(suite_dir),
                env=env,
            )
            proc.wait()
            with _run_lock:
                if run_id in _run_registry:
                    _run_registry[run_id]["status"] = "done"
                    _run_registry[run_id]["finished_at"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
                    _run_registry[run_id]["exit_code"] = proc.returncode
                    # collect latest reports
                    reports = _scan_reports(report_dir, _run_registry[run_id]["started_at_mtime"])
                    _run_registry[run_id]["reports"] = reports
        except Exception as exc:
            with _run_lock:
                if run_id in _run_registry:
                    _run_registry[run_id]["status"] = "failed"
                    _run_registry[run_id]["error"] = str(exc)

    @router.post("/api/loadtests/run")
    def api_start_loadtest(
        background_tasks: BackgroundTasks,
        scenario: str = "all",
        total: int = Query(20, ge=1, le=100000),
        concurrency: int = Query(5, ge=1, le=5000),
    ) -> dict:
        report_dir = _find_report_dir(runtime)
        runner = _find_runner(runtime)
        repo_root = Path(runtime.config.get("_package_root", "."))
        configured_config = runtime.config.get("loadtest", {}).get("config_path", "")
        config_path = Path(configured_config).resolve() if configured_config else repo_root / "tools" / "loadtest" / "python-loadtest" / "config.json"

        if not runner.exists():
            return {"error": f"Python loadtest runner not found: {runner}"}
        if not config_path.exists():
            return {"error": f"Config not found: {config_path}"}

        run_id = str(uuid.uuid4())[:8]
        started_at = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        started_at_mtime = time.time()

        with _run_lock:
            _run_registry[run_id] = {
                "run_id": run_id,
                "status": "running",
                "scenario": scenario,
                "total": total,
                "concurrency": concurrency,
                "started_at": started_at,
                "started_at_mtime": started_at_mtime,
                "finished_at": "",
                "exit_code": None,
                "error": "",
                "reports": [],
                "progress": 0,
                "total_scenarios": 12,
                "completed_scenarios": 0,
            }

        background_tasks.add_task(
            _run_loadtest_bg,
            run_id,
            scenario,
            total,
            concurrency,
            runner,
            config_path,
            report_dir,
        )

        return {"run_id": run_id, "status": "running", "started_at": started_at}

    @router.get("/api/loadtests/run/{run_id}/status")
    def api_loadtest_status(run_id: str) -> dict:
        with _run_lock:
            entry = _run_registry.get(run_id)
        if not entry:
            return {"error": f"Run {run_id} not found"}
        return {
            "run_id": entry["run_id"],
            "status": entry["status"],
            "scenario": entry["scenario"],
            "started_at": entry["started_at"],
            "finished_at": entry["finished_at"],
            "exit_code": entry["exit_code"],
            "error": entry["error"],
            "reports": entry.get("reports", []),
            "progress": entry.get("progress", 0),
            "total_scenarios": entry.get("total_scenarios", 12),
            "completed_scenarios": entry.get("completed_scenarios", 0),
        }

    return router
