from __future__ import annotations

import json
from contextlib import contextmanager
from typing import Any, Dict, Iterable, Iterator, Optional

import pymysql
import redis
from pymysql.cursors import DictCursor


@contextmanager
def mysql_conn(mysql_cfg: Dict[str, Any]) -> Iterator[pymysql.connections.Connection]:
    conn = pymysql.connect(
        host=str(mysql_cfg["host"]),
        port=int(mysql_cfg["port"]),
        user=str(mysql_cfg["user"]),
        password=str(mysql_cfg["password"]),
        database=str(mysql_cfg["schema"]),
        charset=str(mysql_cfg.get("charset", "utf8mb4")),
        cursorclass=DictCursor,
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


def fetch_all(conn: pymysql.connections.Connection, sql: str, params: Optional[Iterable[Any]] = None):
    with conn.cursor() as cur:
        cur.execute(sql, tuple(params or ()))
        return list(cur.fetchall())


def fetch_one(conn: pymysql.connections.Connection, sql: str, params: Optional[Iterable[Any]] = None):
    with conn.cursor() as cur:
        cur.execute(sql, tuple(params or ()))
        return cur.fetchone()


def execute(conn: pymysql.connections.Connection, sql: str, params: Optional[Iterable[Any]] = None) -> int:
    with conn.cursor() as cur:
        return cur.execute(sql, tuple(params or ()))


def set_json_cache(client: redis.Redis, key: str, value: Any, expire_seconds: int = 300) -> None:
    client.set(key, json.dumps(value, ensure_ascii=False, default=str), ex=expire_seconds)


def get_json_cache(client: redis.Redis, key: str) -> Any:
    raw = client.get(key)
    if not raw:
        return None
    return json.loads(raw)
