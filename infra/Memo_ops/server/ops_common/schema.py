from __future__ import annotations

from pathlib import Path
from typing import Dict

import psycopg


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[3]


def _load_sql(path: Path, replacements: dict[str, str]) -> str:
    text = path.read_text(encoding="utf-8")
    for key, value in replacements.items():
        text = text.replace(key, value)
    return text


def init_schema(pg_cfg: Dict[str, object]) -> None:
    root = psycopg.connect(
        host=str(pg_cfg["host"]),
        port=int(pg_cfg["port"]),
        user=str(pg_cfg["user"]),
        password=str(pg_cfg["password"]),
        dbname="postgres",
        sslmode=str(pg_cfg.get("sslmode", "disable")),
        autocommit=True,
    )
    database = str(pg_cfg["database"])
    with root.cursor() as cur:
        cur.execute("SELECT 1 FROM pg_database WHERE datname = %s", (database,))
        if cur.fetchone() is None:
            cur.execute(f'CREATE DATABASE "{database}"')
    root.close()

    migration_path = _repo_root() / "migrations" / "postgresql" / "memo_ops" / "001_baseline.sql"
    sql = _load_sql(
        migration_path,
        {
            "__OPS_SCHEMA__": str(pg_cfg.get("schema", "memo_ops")),
        },
    )

    conn = psycopg.connect(
        host=str(pg_cfg["host"]),
        port=int(pg_cfg["port"]),
        user=str(pg_cfg["user"]),
        password=str(pg_cfg["password"]),
        dbname=database,
        sslmode=str(pg_cfg.get("sslmode", "disable")),
        autocommit=True,
    )
    with conn.cursor() as cur:
        cur.execute(sql)
    conn.close()
