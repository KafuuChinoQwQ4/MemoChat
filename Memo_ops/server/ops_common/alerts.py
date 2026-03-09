from __future__ import annotations

from collections.abc import Iterable


def _safe_float(value: object, default: float = 0.0) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def build_active_alerts(config: dict, snapshots: Iterable[dict]) -> list[dict]:
    collector_cfg = config.get("collector", {})
    cpu_warn = _safe_float(collector_cfg.get("cpu_warn"), 85.0)
    error_warn = _safe_float(collector_cfg.get("error_rate_warn"), 0.25)
    alerts = []
    for snapshot in snapshots:
        if snapshot["status"] != "up":
            alerts.append(
                {
                    "severity": "critical",
                    "service": snapshot["service_name"],
                    "instance": snapshot["instance_name"],
                    "message": "service is offline or port probe failed",
                }
            )
        if snapshot["cpu_percent"] >= cpu_warn:
            alerts.append(
                {
                    "severity": "warning",
                    "service": snapshot["service_name"],
                    "instance": snapshot["instance_name"],
                    "message": f"cpu usage is {snapshot['cpu_percent']:.1f}%",
                }
            )
        if snapshot["error_rate"] >= error_warn:
            alerts.append(
                {
                    "severity": "warning",
                    "service": snapshot["service_name"],
                    "instance": snapshot["instance_name"],
                    "message": f"error rate is {snapshot['error_rate']:.2f}",
                }
            )
    return alerts
