from __future__ import annotations

from configparser import ConfigParser
from pathlib import Path
from typing import Any, Dict, List

import yaml


def package_root() -> Path:
    return Path(__file__).resolve().parents[2]


def load_yaml_config(path: str | Path) -> Dict[str, Any]:
    config_path = Path(path).resolve()
    with config_path.open("r", encoding="utf-8") as fh:
        data = yaml.safe_load(fh) or {}
    data["_config_path"] = str(config_path)
    data["_config_dir"] = str(config_path.parent)
    data["_package_root"] = str(package_root())
    return normalize_config(data)


def normalize_config(data: Dict[str, Any]) -> Dict[str, Any]:
    redis_cfg = data.setdefault("redis", {})
    if "prefix" not in redis_cfg and "key_prefix" in redis_cfg:
        redis_cfg["prefix"] = redis_cfg["key_prefix"]

    repo_cfg = data.setdefault("repo", {})
    cluster_cfg = data.setdefault("cluster", {})
    cluster_path = cluster_cfg.get("config") or repo_cfg.get("cluster_config_path") or "../../server/StatusServer/config.ini"
    data["cluster"] = {"config": str(resolve_path(data, cluster_path))}

    obs_cfg = data.setdefault("observability", {})
    if "grafana" not in obs_cfg and "grafana_url" in obs_cfg:
        obs_cfg["grafana"] = obs_cfg["grafana_url"]
    if "loki" not in obs_cfg and "loki_url" in obs_cfg:
        obs_cfg["loki"] = obs_cfg["loki_url"]
    if "tempo" not in obs_cfg and "tempo_url" in obs_cfg:
        obs_cfg["tempo"] = obs_cfg["tempo_url"]

    if "collector" not in data:
        data["collector"] = {}
    collector_cfg = data["collector"]
    if "interval_sec" not in collector_cfg and "interval_seconds" in collector_cfg:
        collector_cfg["interval_sec"] = collector_cfg["interval_seconds"]
    thresholds = data.get("thresholds", {})
    for key in ("cpu_warn", "missing_service_after_sec", "error_rate_warn"):
        if key not in collector_cfg and key in thresholds:
            collector_cfg[key] = thresholds[key]
    if "metrics" not in data:
        data["metrics"] = {}
    metrics_cfg = data["metrics"]
    if "host" not in metrics_cfg and "metrics_host" in collector_cfg:
        metrics_cfg["host"] = collector_cfg["metrics_host"]
    if "port" not in metrics_cfg and "metrics_port" in collector_cfg:
        metrics_cfg["port"] = collector_cfg["metrics_port"]
    return data


def resolve_path(config: Dict[str, Any], raw_path: str) -> Path:
    candidate = Path(raw_path)
    if candidate.is_absolute():
        return candidate
    return (Path(config["_config_dir"]) / candidate).resolve()


def resolve_globs(config: Dict[str, Any], patterns: List[str]) -> List[Path]:
    roots: List[Path] = []
    for pattern in patterns:
        roots.extend(sorted(resolve_path(config, pattern).parent.glob(resolve_path(config, pattern).name)))
    return roots


def load_cluster_nodes(cluster_config_path: Path) -> List[Dict[str, Any]]:
    parser = ConfigParser()
    parser.read(cluster_config_path, encoding="utf-8")
    if not parser.has_section("Cluster"):
        return []
    nodes = [item.strip() for item in parser.get("Cluster", "Nodes", fallback="").split(",") if item.strip()]
    discovered: List[Dict[str, Any]] = []
    for section in nodes:
        if not parser.has_section(section):
            continue
        enabled = parser.get(section, "Enabled", fallback="true").lower() not in {"false", "0", "no", "off"}
        if not enabled:
            continue
        discovered.append(
            {
                "name": parser.get(section, "Name", fallback=section),
                "tcp_host": parser.get(section, "TcpHost", fallback="127.0.0.1"),
                "tcp_port": parser.getint(section, "TcpPort", fallback=0),
                "rpc_host": parser.get(section, "RpcHost", fallback="127.0.0.1"),
                "rpc_port": parser.getint(section, "RpcPort", fallback=0),
                "enabled": enabled,
            }
        )
    return discovered
