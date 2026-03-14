#!/usr/bin/env python3
from __future__ import annotations

import argparse
from datetime import datetime, timezone
from pathlib import Path

from postgresql_migration_common import (
    build_table_specs,
    load_mysql_ini,
    load_pg_yaml,
    mysql_connect,
    save_manifest,
    table_file_name,
    table_count_mysql,
    write_ndjson,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="Export MemoChat MySQL data into a PostgreSQL import bundle")
    parser.add_argument("--mysql-config", default="server/ChatServer/config.ini")
    parser.add_argument("--ops-config", default="Memo_ops/config/opsserver.yaml")
    parser.add_argument("--bundle-dir", required=True)
    parser.add_argument("--app-schema", default="memo")
    parser.add_argument("--ops-schema", default="memo_ops")
    args = parser.parse_args()

    mysql_cfg = load_mysql_ini(Path(args.mysql_config).resolve())
    pg_cfg = load_pg_yaml(Path(args.ops_config).resolve())
    bundle_dir = Path(args.bundle_dir).resolve()
    bundle_dir.mkdir(parents=True, exist_ok=True)

    manifest = {
        "exported_at_utc": datetime.now(timezone.utc).isoformat(),
        "source_mysql": {
            "host": mysql_cfg["host"],
            "port": mysql_cfg["port"],
            "database": mysql_cfg["database"],
        },
        "target_postgresql": {
            "host": pg_cfg["host"],
            "port": pg_cfg["port"],
            "database": pg_cfg["database"],
            "app_schema": args.app_schema,
            "ops_schema": args.ops_schema,
        },
        "tables": [],
    }

    specs = build_table_specs(args.app_schema, args.ops_schema)
    with mysql_connect(mysql_cfg) as conn:
        for spec in specs:
            output_path = bundle_dir / table_file_name(spec)
            row_count = write_ndjson(output_path, iter_export_rows(conn, spec.source_schema, spec.table))
            source_count = table_count_mysql(conn, spec.source_schema, spec.table)
            manifest["tables"].append(
                {
                    "scope": spec.scope,
                    "table": spec.table,
                    "source_schema": spec.source_schema,
                    "target_schema": spec.target_schema,
                    "file": output_path.name,
                    "exported_rows": row_count,
                    "source_rows": source_count,
                }
            )
    save_manifest(bundle_dir, manifest)
    print(f"bundle exported to {bundle_dir}")
    return 0


def iter_export_rows(conn, schema: str, table: str):
    from postgresql_migration_common import iter_export_rows as _iter_export_rows

    yield from _iter_export_rows(conn, schema, table)


if __name__ == "__main__":
    raise SystemExit(main())
