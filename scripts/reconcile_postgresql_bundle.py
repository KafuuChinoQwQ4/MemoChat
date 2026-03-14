#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

from postgresql_migration_common import (
    load_mysql_ini,
    load_pg_yaml,
    mysql_connect,
    pg_connect,
    quote_ident,
    read_manifest,
    table_count_mysql,
    table_count_pg,
)


ORPHAN_CHECKS = [
    {
        "name": "friend.self_id_missing_user",
        "mysql": "SELECT COUNT(*) AS cnt FROM `{schema}`.`friend` f LEFT JOIN `{schema}`.`user` u ON u.uid=f.self_id WHERE u.uid IS NULL",
        "pg": 'SELECT COUNT(*) AS cnt FROM "{schema}".friend f LEFT JOIN "{schema}"."user" u ON u.uid=f.self_id WHERE u.uid IS NULL',
    },
    {
        "name": "friend.friend_id_missing_user",
        "mysql": "SELECT COUNT(*) AS cnt FROM `{schema}`.`friend` f LEFT JOIN `{schema}`.`user` u ON u.uid=f.friend_id WHERE u.uid IS NULL",
        "pg": 'SELECT COUNT(*) AS cnt FROM "{schema}".friend f LEFT JOIN "{schema}"."user" u ON u.uid=f.friend_id WHERE u.uid IS NULL',
    },
    {
        "name": "chat_group_member.missing_group",
        "mysql": "SELECT COUNT(*) AS cnt FROM `{schema}`.`chat_group_member` gm LEFT JOIN `{schema}`.`chat_group` g ON g.group_id=gm.group_id WHERE g.group_id IS NULL",
        "pg": 'SELECT COUNT(*) AS cnt FROM "{schema}".chat_group_member gm LEFT JOIN "{schema}".chat_group g ON g.group_id=gm.group_id WHERE g.group_id IS NULL',
    },
]


def fetch_mysql_value(conn, query: str) -> int:
    with conn.cursor() as cur:
        cur.execute(query)
        row = cur.fetchone()
    return int(row["cnt"])


def fetch_pg_value(conn, query: str) -> int:
    with conn.cursor() as cur:
        cur.execute(query)
        row = cur.fetchone()
    return int(row["cnt"])


def sample_user_hashes_mysql(conn, schema: str, limit: int) -> list[dict[str, Any]]:
    sql = f"""
        SELECT uid, email, nick, user_id
          FROM `{schema}`.`user`
         ORDER BY uid ASC
         LIMIT %s
    """
    with conn.cursor() as cur:
        cur.execute(sql, (limit,))
        rows = cur.fetchall()
    return build_hashes(rows)


def sample_user_hashes_pg(conn, schema: str, limit: int) -> list[dict[str, Any]]:
    sql = f"""
        SELECT uid, email, nick, user_id
          FROM {quote_ident(schema)}.{quote_ident('user')}
         ORDER BY uid ASC
         LIMIT %s
    """
    with conn.cursor() as cur:
        cur.execute(sql, (limit,))
        rows = cur.fetchall()
    return build_hashes(rows)


def build_hashes(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    result = []
    for row in rows:
        serialized = json.dumps(row, ensure_ascii=False, sort_keys=True, default=str)
        result.append(
            {
                "uid": row.get("uid"),
                "sha256": hashlib.sha256(serialized.encode("utf-8")).hexdigest(),
            }
        )
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare MySQL source data with imported PostgreSQL data")
    parser.add_argument("--mysql-config", default="server/ChatServer/config.ini")
    parser.add_argument("--ops-config", default="Memo_ops/config/opsserver.yaml")
    parser.add_argument("--bundle-dir", required=True)
    parser.add_argument("--sample-users", type=int, default=50)
    args = parser.parse_args()

    manifest = read_manifest(Path(args.bundle_dir).resolve())
    mysql_cfg = load_mysql_ini(Path(args.mysql_config).resolve())
    pg_cfg = load_pg_yaml(Path(args.ops_config).resolve())

    report = {
        "tables": [],
        "orphans": [],
        "sample_user_hashes": {},
    }

    with mysql_connect(mysql_cfg) as mysql_conn, pg_connect(pg_cfg) as pg_conn:
        for table_info in manifest["tables"]:
            report["tables"].append(
                {
                    "scope": table_info["scope"],
                    "table": table_info["table"],
                    "source_schema": table_info["source_schema"],
                    "target_schema": table_info["target_schema"],
                    "mysql_rows": table_count_mysql(mysql_conn, table_info["source_schema"], table_info["table"]),
                    "postgres_rows": table_count_pg(pg_conn, table_info["target_schema"], table_info["table"]),
                }
            )

        app_schema = manifest["target_postgresql"]["app_schema"]
        for check in ORPHAN_CHECKS:
            report["orphans"].append(
                {
                    "name": check["name"],
                    "mysql": fetch_mysql_value(mysql_conn, check["mysql"].format(schema=app_schema)),
                    "postgres": fetch_pg_value(pg_conn, check["pg"].format(schema=app_schema)),
                }
            )

        report["sample_user_hashes"]["mysql"] = sample_user_hashes_mysql(mysql_conn, app_schema, args.sample_users)
        report["sample_user_hashes"]["postgres"] = sample_user_hashes_pg(pg_conn, app_schema, args.sample_users)

    output_path = Path(args.bundle_dir).resolve() / "reconcile_report.json"
    output_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"reconcile report written to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
