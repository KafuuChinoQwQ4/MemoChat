from __future__ import annotations

import json
import socket
from contextlib import closing
from dataclasses import dataclass
from datetime import timedelta

import psutil

from .alerts import build_active_alerts
from .config import load_cluster_nodes
from .health_checks import data_source_health
from .repositories import (
    list_monitoring_overview_recent_log_counts,
    list_monitoring_overview_service_snapshots,
    list_overview_recent_runs,
)
from .storage import fetch_all, get_json_cache, mysql_conn, prefixed_key, redis_client, set_json_cache
from .time_windows import utc_now


@dataclass
class ServiceDescriptor:
    service_name: str
    instance_name: str
    process_match: str
    host: str
    port: int
    service_type: str


def _check_port(host: str, port: int, timeout_sec: float = 0.8) -> bool:
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        sock.settimeout(timeout_sec)
        return sock.connect_ex((host, port)) == 0


def _process_stats(process_match: str) -> dict[str, object]:
    cpu_total = 0.0
    rss_bytes = 0
    count = 0
    for proc in psutil.process_iter(["name", "cmdline", "memory_info"]):
        try:
            name = (proc.info.get("name") or "").lower()
            cmdline = " ".join(proc.info.get("cmdline") or []).lower()
            if process_match.lower() not in name and process_match.lower() not in cmdline:
                continue
            count += 1
            cpu_total += proc.cpu_percent(interval=0.0)
            mem = proc.info.get("memory_info")
            if mem:
                rss_bytes += int(mem.rss)
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            continue
    return {"process_count": count, "cpu_percent": cpu_total, "rss_bytes": rss_bytes}


def _service_catalog(config: dict) -> list[ServiceDescriptor]:
    cluster_path = config["cluster"]["config"]
    chat_nodes = load_cluster_nodes(cluster_path)
    descriptors = [
        ServiceDescriptor("GateServer", "gate-http", "GateServer", "127.0.0.1", 8080, "http"),
        ServiceDescriptor("StatusServer", "status-grpc", "StatusServer", "127.0.0.1", 50052, "grpc"),
        ServiceDescriptor("VarifyServer", "varify-grpc", "VarifyServer", "127.0.0.1", 50051, "grpc"),
    ]
    for node in chat_nodes:
        descriptors.append(
            ServiceDescriptor(
                "ChatServer",
                node["name"],
                "ChatServer",
                node["tcp_host"],
                int(node["tcp_port"]),
                "tcp",
            )
        )
    return descriptors


def _online_user_count(redis_cfg: dict, instance_name: str) -> int:
    client = redis_client(redis_cfg)
    key = f"server_online_users_{instance_name}"
    try:
        return int(client.scard(key))
    except Exception:
        return 0


def _safe_float(value: object, default: float = 0.0) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def _recent_log_metrics(conn, service_name: str, instance_name: str) -> dict[str, float]:
    since = (utc_now() - timedelta(minutes=5)).strftime("%Y-%m-%d %H:%M:%S")
    rows = fetch_all(
        conn,
        """
        SELECT level, duration_ms
          FROM ops_log_event_index
         WHERE service_name=%s
           AND (%s='' OR service_instance=%s)
           AND ts_utc >= %s
        """,
        (service_name, instance_name, instance_name, since),
    )
    total = len(rows)
    errors = sum(1 for row in rows if (row.get("level") or "").lower() in {"error", "fatal", "critical"})
    durations = sorted(_safe_float(row.get("duration_ms")) for row in rows if row.get("duration_ms") is not None)
    p95 = 0.0
    if durations:
        index = max(0, min(len(durations) - 1, int(len(durations) * 0.95) - 1))
        p95 = durations[index]
    return {
        "qps": total / 300.0,
        "error_rate": (errors / total) if total else 0.0,
        "latency_p95_ms": p95,
    }


def _insert_snapshot(conn, snapshot: dict) -> None:
    conn.cursor().execute(
        """
        INSERT INTO ops_service_snapshot(
            service_name, instance_name, observed_at, process_count, cpu_percent,
            memory_rss_bytes, online_users, qps, error_rate, latency_p95_ms,
            port_up, status, details_json
        ) VALUES (%s, %s, UTC_TIMESTAMP(), %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """,
        (
            snapshot["service_name"],
            snapshot["instance_name"],
            snapshot["process_count"],
            snapshot["cpu_percent"],
            snapshot["memory_rss_bytes"],
            snapshot["online_users"],
            snapshot["qps"],
            snapshot["error_rate"],
            snapshot["latency_p95_ms"],
            1 if snapshot["port_up"] else 0,
            snapshot["status"],
            json.dumps(snapshot["details"], ensure_ascii=True),
        ),
    )
    conn.cursor().execute(
        """
        INSERT INTO ops_service_metric_minute(
            service_name, instance_name, minute_utc, cpu_percent_avg, memory_rss_bytes_avg,
            online_users_avg, qps_avg, error_rate_avg, latency_p95_ms_avg, samples
        ) VALUES (%s, %s, DATE_FORMAT(UTC_TIMESTAMP(), '%%Y-%%m-%%d %%H:%%i:00'), %s, %s, %s, %s, %s, %s, 1)
        ON DUPLICATE KEY UPDATE
            cpu_percent_avg=((cpu_percent_avg*samples)+VALUES(cpu_percent_avg))/(samples+1),
            memory_rss_bytes_avg=((memory_rss_bytes_avg*samples)+VALUES(memory_rss_bytes_avg))/(samples+1),
            online_users_avg=((online_users_avg*samples)+VALUES(online_users_avg))/(samples+1),
            qps_avg=((qps_avg*samples)+VALUES(qps_avg))/(samples+1),
            error_rate_avg=((error_rate_avg*samples)+VALUES(error_rate_avg))/(samples+1),
            latency_p95_ms_avg=((latency_p95_ms_avg*samples)+VALUES(latency_p95_ms_avg))/(samples+1),
            samples=samples+1
        """,
        (
            snapshot["service_name"],
            snapshot["instance_name"],
            snapshot["cpu_percent"],
            snapshot["memory_rss_bytes"],
            snapshot["online_users"],
            snapshot["qps"],
            snapshot["error_rate"],
            snapshot["latency_p95_ms"],
        ),
    )


def build_overview(conn, config: dict) -> dict[str, object]:
    limit = int((config.get("platform", {}) or {}).get("overview_limit", 10))
    return {
        "generated_at": utc_now().isoformat(),
        "recent_runs": list_overview_recent_runs(conn, limit),
        "service_snapshots": list_monitoring_overview_service_snapshots(conn),
        "recent_log_counts": list_monitoring_overview_recent_log_counts(conn),
        "data_sources": data_source_health(config),
    }


def cached_overview(config: dict) -> dict[str, object]:
    redis_cfg = config.get("redis", {})
    client = redis_client(redis_cfg)
    cached = get_json_cache(client, prefixed_key(redis_cfg, "overview"))
    if cached:
        return cached
    with mysql_conn(config["mysql"]) as conn:
        overview = build_overview(conn, config)
    set_json_cache(client, prefixed_key(redis_cfg, "overview"), overview, 30)
    return overview


def collect_snapshots(config: dict) -> list[dict]:
    descriptors = _service_catalog(config)
    snapshots: list[dict] = []
    redis_cfg = config["redis"]
    redis_handle = redis_client(redis_cfg)
    for descriptor in descriptors:
        process_stats = _process_stats(descriptor.process_match)
        port_up = _check_port(descriptor.host, descriptor.port)
        with mysql_conn(config["mysql"]) as conn:
            log_metrics = _recent_log_metrics(conn, descriptor.service_name, descriptor.instance_name)
        online_users = 0
        if descriptor.service_name == "ChatServer":
            online_users = _online_user_count(redis_cfg, descriptor.instance_name)
        status = "up" if port_up and process_stats["process_count"] > 0 else "down"
        snapshot = {
            "service_name": descriptor.service_name,
            "instance_name": descriptor.instance_name,
            "process_count": int(process_stats["process_count"]),
            "cpu_percent": float(process_stats["cpu_percent"]),
            "memory_rss_bytes": int(process_stats["rss_bytes"]),
            "online_users": int(online_users),
            "qps": float(log_metrics["qps"]),
            "error_rate": float(log_metrics["error_rate"]),
            "latency_p95_ms": float(log_metrics["latency_p95_ms"]),
            "port_up": bool(port_up),
            "status": status,
            "details": {
                "host": descriptor.host,
                "port": descriptor.port,
                "service_type": descriptor.service_type,
            },
        }
        with mysql_conn(config["mysql"]) as conn:
            _insert_snapshot(conn, snapshot)
        set_json_cache(redis_handle, prefixed_key(redis_cfg, f"service:{descriptor.instance_name}:current"), snapshot, 60)
        snapshots.append(snapshot)

    alerts = build_active_alerts(config, snapshots)
    set_json_cache(redis_handle, prefixed_key(redis_cfg, "alerts:active"), alerts, 60)
    with mysql_conn(config["mysql"]) as conn:
        overview = build_overview(conn, config)
    set_json_cache(redis_handle, prefixed_key(redis_cfg, "overview"), overview, 60)
    return snapshots
