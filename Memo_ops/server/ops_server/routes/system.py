from __future__ import annotations

import json
import os
import re
import subprocess
import time
from pathlib import Path
from typing import Any, Dict, List

import psutil
from configparser import ConfigParser
from fastapi import APIRouter

from Memo_ops.server.ops_server.runtime import OpsServerRuntime


def _service_meta(name: str, cfg_dir: Path) -> Dict[str, Any]:
    patterns = [
        cfg_dir / ".." / ".." / "server" / "GateServer" / "config.ini",
        cfg_dir / ".." / ".." / "server" / "GateServerHttp1.1" / "config.ini",
        cfg_dir / ".." / ".." / "server" / "GateServerHttp2" / "config.ini",
        cfg_dir / ".." / ".." / "server" / "ChatServer" / "config.ini",
        cfg_dir / ".." / ".." / "server" / "StatusServer" / "config.ini",
        cfg_dir / ".." / ".." / "server" / "GateServer2" / "config.ini",
        cfg_dir / ".." / ".." / "server" / name / "config.ini",
        cfg_dir / ".." / "server" / name / "config.ini",
    ]
    parser = ConfigParser()
    for p in patterns:
        resolved = p.resolve()
        if resolved.exists():
            parser.read(resolved, encoding="utf-8")
            meta: Dict[str, Any] = {"name": name, "config_path": str(resolved)}

            # TCP port
            if parser.has_section("Server"):
                try:
                    meta["port"] = parser.getint("Server", "ListenPort")
                except Exception:
                    pass

            # HTTP port (GateServer variants)
            if parser.has_section("GateServer"):
                try:
                    meta["http_port"] = parser.getint("GateServer", "HttpPort")
                except Exception:
                    pass

            # Log directory
            if parser.has_section("Log"):
                meta["log_dir"] = parser.get("Log", "Dir", fallback="")

            # Telemetry service name
            if parser.has_section("Telemetry"):
                meta["telemetry_service"] = parser.get("Telemetry", "ServiceName", fallback="")

            return meta
    return {"name": name}


def _running_procs() -> Dict[str, List[Dict[str, Any]]]:
    by_service: Dict[str, List[Dict[str, Any]]] = {}
    for proc in psutil.process_iter(["pid", "name", "cmdline", "create_time"]):
        try:
            info = proc.info
            cmdline = " ".join(info.get("cmdline") or [])
            if "GateServer" in cmdline or "ChatServer" in cmdline or "StatusServer" in cmdline:
                match = re.search(r"(GateServer|ChatServer|StatusServer|GateServerHttp1|GateServerHttp2|GateServer2)", cmdline)
                if match:
                    svc = match.group(1)
                    by_service.setdefault(svc, []).append({
                        "pid": info["pid"],
                        "create_time": info["create_time"],
                    })
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
    return by_service


def _collect_system_metrics(runtime: OpsServerRuntime) -> List[Dict[str, Any]]:
    cfg_dir = Path(runtime.config["_config_dir"])
    result: List[Dict[str, Any]] = []
    proc_map = _running_procs()

    # CPU overall
    cpu_pct = psutil.cpu_percent(interval=0.1)

    # Memory overall
    vm = psutil.virtual_memory()
    mem_total_mb = vm.total / (1024 * 1024)
    mem_avail_mb = vm.available / (1024 * 1024)

    services = [
        "GateServer", "GateServerHttp1.1", "GateServerHttp2", "GateServer2",
        "ChatServer", "StatusServer", "VarifyServer",
    ]

    for svc in services:
        pids = proc_map.get(svc, [])
        svc_meta = _service_meta(svc, cfg_dir)

        entry: Dict[str, Any] = {
            "service": svc,
            "status": "up" if pids else "down",
            "cpu_pct": 0.0,
            "rss_mb": 0.0,
            "mem_total_mb": round(mem_total_mb, 1),
            "mem_avail_mb": round(mem_avail_mb, 1),
            "cpu_system_pct": round(cpu_pct, 2),
            "pid": pids[0]["pid"] if pids else 0,
            "port": svc_meta.get("port", 0),
            "http_port": svc_meta.get("http_port", 0),
            "log_dir": svc_meta.get("log_dir", ""),
            "uptime_sec": 0,
        }

        if pids:
            primary = pids[0]
            try:
                proc = psutil.Process(primary["pid"])
                entry["rss_mb"] = round(proc.memory_info().rss / (1024 * 1024), 1)
                entry["uptime_sec"] = int(time.time() - primary["create_time"])
                with proc.oneshot():
                    entry["cpu_pct"] = round(proc.cpu_percent(interval=0.05), 2)
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass

        result.append(entry)

    return result


def create_system_router(runtime: OpsServerRuntime) -> APIRouter:
    router = APIRouter()

    @router.get("/api/system/metrics")
    def api_system_metrics() -> dict:
        def action() -> dict:
            services = _collect_system_metrics(runtime)
            return {
                "services": services,
                "collected_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            }
        return runtime.guarded(action)

    return router
