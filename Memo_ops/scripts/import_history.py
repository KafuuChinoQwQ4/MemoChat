from __future__ import annotations

import sys
from pathlib import Path

from Memo_ops.server.ops_common.config import load_yaml_config
from Memo_ops.server.ops_common.ingest import import_logs, import_reports, rebuild_trace_index
from Memo_ops.server.ops_common.schema import init_schema


def main() -> int:
    config_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / "config" / "opscollector.yaml"
    cfg = load_yaml_config(config_path)
    init_schema(cfg["postgresql"])
    report_stats = import_reports(cfg)
    log_stats = import_logs(cfg)
    rebuild_trace_index(cfg)
    print({"reports": report_stats, "logs": log_stats})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
