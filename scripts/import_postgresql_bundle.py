#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

from postgresql_migration_common import (
    insert_pg_rows,
    load_ndjson,
    load_pg_yaml,
    pg_connect,
    read_manifest,
    truncate_pg_table,
)


def batched_rows(rows, batch_size: int):
    batch = []
    for row in rows:
        batch.append(row)
        if len(batch) >= batch_size:
            yield batch
            batch = []
    if batch:
        yield batch


def main() -> int:
    parser = argparse.ArgumentParser(description="Import a PostgreSQL migration bundle into MemoChat PostgreSQL")
    parser.add_argument("--ops-config", default="Memo_ops/config/opsserver.yaml")
    parser.add_argument("--bundle-dir", required=True)
    parser.add_argument("--batch-size", type=int, default=500)
    parser.add_argument("--skip-truncate", action="store_true")
    args = parser.parse_args()

    bundle_dir = Path(args.bundle_dir).resolve()
    manifest = read_manifest(bundle_dir)
    pg_cfg = load_pg_yaml(Path(args.ops_config).resolve())

    with pg_connect(pg_cfg) as conn:
        for table_info in manifest["tables"]:
            if not args.skip_truncate:
                truncate_pg_table(conn, table_info["target_schema"], table_info["table"])
            data_path = bundle_dir / table_info["file"]
            for batch in batched_rows(load_ndjson(data_path), args.batch_size):
                insert_pg_rows(conn, table_info["target_schema"], table_info["table"], batch)
        conn.commit()

    print(f"bundle imported from {bundle_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
