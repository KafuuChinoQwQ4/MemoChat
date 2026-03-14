from __future__ import annotations

import json
from contextlib import contextmanager
from typing import Any, Dict, Iterable, Iterator, Optional

import psycopg
import redis
from psycopg.rows import dict_row


@contextmanager
def postgres_conn(pg_cfg: Dict[str, Any]) -> Iterator[psycopg.Connection]:
    conn = psycopg.connect(
        host=str(pg_cfg["host"]),
        port=int(pg_cfg["port"]),
        user=str(pg_cfg["user"]),
        password=str(pg_cfg["password"]),
        dbname=str(pg_cfg["database"]),
        options=f"-c search_path={pg_cfg.get('schema', 'public')},public",
        sslmode=str(pg_cfg.get("sslmode", "disable")),
        row_factory=dict_row,
        autocommit=False,
    )
    try:
        yield conn
        conn.commit()
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


@contextmanager
def mysql_conn(mysql_cfg: Dict[str, Any]) -> Iterator[psycopg.Connection]:
    # Compatibility alias for existing callers while the codebase finishes renaming to postgresql/postgres_conn.
    with postgres_conn(mysql_cfg) as conn:
        yield conn


def redis_client(redis_cfg: Dict[str, Any]) -> redis.Redis:
    return redis.Redis(
        host=str(redis_cfg["host"]),
        port=int(redis_cfg["port"]),
        password=str(redis_cfg.get("password", "")) or None,
        db=int(redis_cfg.get("db", 0)),
        decode_responses=True,
    )


def prefixed_key(redis_cfg: Dict[str, Any], suffix: str) -> str:
    prefix = str(redis_cfg.get("prefix", redis_cfg.get("key_prefix", "ops"))).strip(":")
    return f"{prefix}:{suffix}"


def fetch_all(conn: psycopg.Connection, sql: str, params: Optional[Iterable[Any]] = None):
    with conn.cursor() as cur:
        cur.execute(sql, tuple(params or ()))
        return list(cur.fetchall())


def fetch_one(conn: psycopg.Connection, sql: str, params: Optional[Iterable[Any]] = None):
    with conn.cursor() as cur:
        cur.execute(sql, tuple(params or ()))
        return cur.fetchone()


def execute(conn: psycopg.Connection, sql: str, params: Optional[Iterable[Any]] = None) -> int:
    with conn.cursor() as cur:
        cur.execute(sql, tuple(params or ()))
        return cur.rowcount


def set_json_cache(client: redis.Redis, key: str, value: Any, expire_seconds: int = 300) -> None:
    client.set(key, json.dumps(value, ensure_ascii=False, default=str), ex=expire_seconds)


def get_json_cache(client: redis.Redis, key: str) -> Any:
    raw = client.get(key)
    if not raw:
        return None
    return json.loads(raw)
