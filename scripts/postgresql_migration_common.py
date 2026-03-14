from __future__ import annotations

import configparser
import json
from dataclasses import dataclass
from datetime import date, datetime
from decimal import Decimal
from pathlib import Path
from typing import Any, Dict, Iterable, Iterator, Sequence

import psycopg
import pymysql
import yaml
from psycopg import sql
from psycopg.rows import dict_row
from psycopg.types.json import Json


BUSINESS_TABLES = [
    "user_id",
    "user",
    "friend",
    "friend_apply",
    "friend_apply_tag",
    "friend_tag",
    "chat_group",
    "chat_group_member",
    "chat_group_apply",
    "chat_private_msg",
    "chat_group_msg",
    "chat_group_msg_ext",
    "chat_dialog",
    "chat_dialog_meta",
    "chat_private_read_state",
    "chat_group_read_state",
    "chat_group_admin_permission",
    "chat_media_asset",
    "chat_call_session",
]

OPS_TABLES = [
    "ops_source",
    "ops_ingest_cursor",
    "ops_loadtest_run",
    "ops_loadtest_case",
    "ops_loadtest_error_bucket",
    "ops_service_snapshot",
    "ops_service_metric_minute",
    "ops_log_stream",
    "ops_log_event_index",
    "ops_trace_index",
    "ops_alert_rule",
    "ops_platform_setting",
]


@dataclass(frozen=True)
class TableSpec:
    scope: str
    table: str
    source_schema: str
    target_schema: str


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def load_mysql_ini(path: Path) -> Dict[str, Any]:
    parser = configparser.ConfigParser()
    parser.read(path, encoding="utf-8")
    if parser.has_section("Postgres"):
        return {
            "host": parser.get("Postgres", "Host"),
            "port": parser.getint("Postgres", "Port"),
            "user": parser.get("Postgres", "User"),
            "password": parser.get("Postgres", "Passwd"),
            "database": parser.get("Postgres", "Database"),
            "schema": parser.get("Postgres", "Schema", fallback="public"),
            "sslmode": parser.get("Postgres", "SslMode", fallback="disable"),
        }
    if not parser.has_section("Mysql"):
        raise ValueError(f"missing [Postgres] or [Mysql] section in {path}")
    return {
        "host": parser.get("Mysql", "Host").replace("tcp://", ""),
        "port": parser.getint("Mysql", "Port"),
        "user": parser.get("Mysql", "User"),
        "password": parser.get("Mysql", "Passwd"),
        "database": parser.get("Mysql", "Schema"),
    }


def load_pg_yaml(path: Path) -> Dict[str, Any]:
    payload = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    pg_cfg = payload.get("postgresql")
    if not isinstance(pg_cfg, dict):
        raise ValueError(f"missing postgresql config in {path}")
    return {
        "host": str(pg_cfg["host"]),
        "port": int(pg_cfg.get("port", 5432)),
        "user": str(pg_cfg["user"]),
        "password": str(pg_cfg.get("password", "")),
        "database": str(pg_cfg["database"]),
        "schema": str(pg_cfg.get("schema", "public")),
        "sslmode": str(pg_cfg.get("sslmode", "disable")),
    }


def build_table_specs(app_schema: str, ops_schema: str) -> list[TableSpec]:
    specs: list[TableSpec] = []
    for table in BUSINESS_TABLES:
        specs.append(TableSpec("business", table, app_schema, app_schema))
    for table in OPS_TABLES:
        specs.append(TableSpec("memo_ops", table, ops_schema, ops_schema))
    return specs


def mysql_connect(cfg: Dict[str, Any]):
    return pymysql.connect(
        host=cfg["host"],
        port=int(cfg["port"]),
        user=cfg["user"],
        password=cfg["password"],
        database=cfg["database"],
        charset="utf8mb4",
        autocommit=False,
        cursorclass=pymysql.cursors.DictCursor,
    )


def pg_connect(cfg: Dict[str, Any], *, dbname: str | None = None, autocommit: bool = False):
    return psycopg.connect(
        host=cfg["host"],
        port=int(cfg["port"]),
        user=cfg["user"],
        password=cfg["password"],
        dbname=dbname or cfg["database"],
        sslmode=cfg.get("sslmode", "disable"),
        options=f"-c search_path={cfg.get('schema', 'public')},public",
        autocommit=autocommit,
        row_factory=dict_row,
    )


def normalize_json_value(value: Any) -> Any:
    if isinstance(value, Decimal):
        if value == value.to_integral_value():
            return int(value)
        return float(value)
    if isinstance(value, (datetime, date)):
        return value.isoformat()
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return value


def iter_export_rows(mysql_conn_handle, schema: str, table: str, batch_size: int = 1000) -> Iterator[dict[str, Any]]:
    with mysql_conn_handle.cursor() as cur:
        cur.execute(f"SELECT * FROM `{schema}`.`{table}`")
        while True:
            rows = cur.fetchmany(batch_size)
            if not rows:
                break
            for row in rows:
                yield {key: normalize_json_value(value) for key, value in row.items()}


def write_ndjson(path: Path, rows: Iterable[dict[str, Any]]) -> int:
    count = 0
    with path.open("w", encoding="utf-8") as fh:
        for row in rows:
            fh.write(json.dumps(row, ensure_ascii=False, default=str))
            fh.write("\n")
            count += 1
    return count


def quote_ident(name: str) -> str:
    return '"' + name.replace('"', '""') + '"'


def load_ndjson(path: Path) -> Iterator[dict[str, Any]]:
    with path.open("r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            yield json.loads(line)


def table_count_mysql(mysql_conn_handle, schema: str, table: str) -> int:
    with mysql_conn_handle.cursor() as cur:
        cur.execute(f"SELECT COUNT(*) AS cnt FROM `{schema}`.`{table}`")
        row = cur.fetchone()
    return int(row["cnt"])


def table_count_pg(pg_conn_handle, schema: str, table: str) -> int:
    with pg_conn_handle.cursor() as cur:
        cur.execute(
            sql.SQL("SELECT COUNT(*) AS cnt FROM {}.{}").format(
                sql.Identifier(schema),
                sql.Identifier(table),
            )
        )
        row = cur.fetchone()
    return int(row["cnt"])


def truncate_pg_table(pg_conn_handle, schema: str, table: str) -> None:
    with pg_conn_handle.cursor() as cur:
        cur.execute(
            sql.SQL("TRUNCATE TABLE {}.{} RESTART IDENTITY CASCADE").format(
                sql.Identifier(schema),
                sql.Identifier(table),
            )
        )


def insert_pg_rows(pg_conn_handle, schema: str, table: str, rows: Sequence[dict[str, Any]]) -> None:
    if not rows:
        return
    columns = list(rows[0].keys())
    column_types = get_pg_column_types(pg_conn_handle, schema, table, columns)
    query = sql.SQL("INSERT INTO {}.{} ({}) VALUES ({})").format(
        sql.Identifier(schema),
        sql.Identifier(table),
        sql.SQL(", ").join(sql.Identifier(col) for col in columns),
        sql.SQL(", ").join(sql.Placeholder() for _ in columns),
    )
    with pg_conn_handle.cursor() as cur:
        cur.executemany(
            query,
            [[coerce_pg_value(row.get(col), column_types.get(col, "")) for col in columns] for row in rows],
        )


def read_manifest(bundle_dir: Path) -> dict[str, Any]:
    return json.loads((bundle_dir / "manifest.json").read_text(encoding="utf-8"))


def save_manifest(bundle_dir: Path, manifest: dict[str, Any]) -> None:
    (bundle_dir / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")


def table_file_name(spec: TableSpec) -> str:
    return f"{spec.scope}__{spec.table}.ndjson"


def get_pg_column_types(pg_conn_handle, schema: str, table: str, columns: Sequence[str]) -> dict[str, str]:
    with pg_conn_handle.cursor() as cur:
        cur.execute(
            """
            SELECT column_name, data_type
              FROM information_schema.columns
             WHERE table_schema = %s
               AND table_name = %s
               AND column_name = ANY(%s)
            """,
            (schema, table, list(columns)),
        )
        return {str(row["column_name"]): str(row["data_type"]).lower() for row in cur.fetchall()}


def coerce_pg_value(value: Any, data_type: str) -> Any:
    if value is None:
        return None
    if data_type == "boolean":
        if isinstance(value, bool):
            return value
        if isinstance(value, (int, float)):
            return bool(value)
        if isinstance(value, str):
            normalized = value.strip().lower()
            if normalized in {"1", "t", "true", "yes", "y", "on"}:
                return True
            if normalized in {"0", "f", "false", "no", "n", "off"}:
                return False
        return bool(value)
    if data_type in {"json", "jsonb"}:
        if isinstance(value, str):
            try:
                return Json(json.loads(value))
            except json.JSONDecodeError:
                return Json(value)
        return Json(value)
    return value
