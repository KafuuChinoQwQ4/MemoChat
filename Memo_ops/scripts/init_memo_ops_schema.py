from __future__ import annotations

import sys
from pathlib import Path

from Memo_ops.server.ops_common.config import load_yaml_config
from Memo_ops.server.ops_common.schema import init_schema


def main() -> int:
    config_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / "config" / "opsserver.yaml"
    cfg = load_yaml_config(config_path)
    init_schema(cfg["mysql"])
    print(f"Initialized memo_ops schema via {config_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
