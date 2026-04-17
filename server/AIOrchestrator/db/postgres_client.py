"""
PostgreSQL 客户端 — AI 相关表的读写操作
"""
import asyncpg
import structlog
from typing import Any, Optional

from config import settings

logger = structlog.get_logger()

_pg_pool: asyncpg.Pool | None = None


async def get_pg_pool() -> asyncpg.Pool:
    global _pg_pool
    if _pg_pool is None:
        cfg = settings
        _pg_pool = await asyncpg.create_pool(
            host="127.0.0.1",
            port=5432,
            user="memochat",
            password="123456",
            database="memo_pg",
            min_size=2,
            max_size=10,
        )
        logger.info("postgres.pool_created")
    return _pg_pool


class PostgresClient:
    """Async PostgreSQL 客户端"""

    def __init__(self, pool: asyncpg.Pool | None = None):
        self._pool = pool

    async def __aenter__(self):
        self._pool = await get_pg_pool()
        return self

    async def __aexit__(self, *args):
        pass

    async def fetchone(self, query: str, *args) -> Optional[dict]:
        pool = self._pool or await get_pg_pool()
        async with pool.acquire() as conn:
            row = await conn.fetchrow(query, *args)
            if row:
                return dict(row)
            return None

    async def fetchall(self, query: str, *args) -> list[dict]:
        pool = self._pool or await get_pg_pool()
        async with pool.acquire() as conn:
            rows = await conn.fetch(query, *args)
            return [dict(r) for r in rows]

    async def execute(self, query: str, *args) -> str:
        pool = self._pool or await get_pg_pool()
        async with pool.acquire() as conn:
            return await conn.execute(query, *args)

    async def close(self):
        global _pg_pool
        if _pg_pool:
            await _pg_pool.close()
            _pg_pool = None
