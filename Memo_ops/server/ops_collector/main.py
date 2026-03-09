from __future__ import annotations

import argparse
import time
from pathlib import Path

from prometheus_client import Gauge, start_http_server

from Memo_ops.server.ops_common.config import load_yaml_config
from Memo_ops.server.ops_common.ingest import import_logs, import_reports, rebuild_trace_index
from Memo_ops.server.ops_common.monitoring import collect_snapshots
from Memo_ops.server.ops_common.schema import init_schema


DEFAULT_CONFIG = Path(__file__).resolve().parents[2] / "config" / "opscollector.yaml"

CPU_GAUGE = Gauge("memo_ops_service_cpu_percent", "Service CPU usage", ["service", "instance"])
RSS_GAUGE = Gauge("memo_ops_service_rss_bytes", "Service RSS memory", ["service", "instance"])
ONLINE_GAUGE = Gauge("memo_ops_service_online_users", "Chat online users", ["service", "instance"])
QPS_GAUGE = Gauge("memo_ops_service_qps", "Recent QPS", ["service", "instance"])
ERROR_GAUGE = Gauge("memo_ops_service_error_rate", "Recent error rate", ["service", "instance"])
LATENCY_GAUGE = Gauge("memo_ops_service_latency_p95_ms", "Recent p95 latency", ["service", "instance"])
UP_GAUGE = Gauge("memo_ops_service_up", "Port/process status", ["service", "instance"])


def _update_metrics(snapshots: list[dict]) -> None:
    for item in snapshots:
        labels = (item["service_name"], item["instance_name"])
        CPU_GAUGE.labels(*labels).set(item["cpu_percent"])
        RSS_GAUGE.labels(*labels).set(item["memory_rss_bytes"])
        ONLINE_GAUGE.labels(*labels).set(item["online_users"])
        QPS_GAUGE.labels(*labels).set(item["qps"])
        ERROR_GAUGE.labels(*labels).set(item["error_rate"])
        LATENCY_GAUGE.labels(*labels).set(item["latency_p95_ms"])
        UP_GAUGE.labels(*labels).set(1 if item["status"] == "up" else 0)


def run_cycle(config: dict) -> dict:
    reports = import_reports(config)
    logs = import_logs(config)
    traces = rebuild_trace_index(config)
    snapshots = collect_snapshots(config)
    _update_metrics(snapshots)
    return {
        "reports": reports,
        "logs": logs,
        "traces": traces,
        "snapshots": len(snapshots),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", default=str(DEFAULT_CONFIG))
    parser.add_argument("--once", action="store_true")
    args = parser.parse_args()

    config = load_yaml_config(args.config)
    init_schema(config["mysql"])
    metrics_cfg = config["metrics"]
    start_http_server(int(metrics_cfg["port"]), addr=metrics_cfg["host"])

    if args.once:
        result = run_cycle(config)
        print(result)
        return

    interval = int(config.get("collector", {}).get("interval_sec", 10))
    while True:
        try:
            run_cycle(config)
        except Exception as exc:
            print(f"[memo_ops] collector cycle failed: {exc}")
        time.sleep(interval)


if __name__ == "__main__":
    main()
