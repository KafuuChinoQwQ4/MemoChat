"""
Neo4j 客户端 — 异步连接管理
支持同步/异步查询，自动重连，连接池
"""
import asyncio
from typing import Any, Optional
import structlog
from neo4j import AsyncGraphDatabase, Driver, GraphDatabase
from neo4j.exceptions import ServiceUnavailable, AuthError

from config import settings

logger = structlog.get_logger()


class Neo4jClient:
    _instance: "Neo4jClient | None" = None

    def __init__(self):
        self._driver: Driver | None = None
        self._async_driver: Any | None = None
        self._enabled = settings.neo4j.enabled

    @classmethod
    def get_instance(cls) -> "Neo4jClient":
        if cls._instance is None:
            cls._instance = Neo4jClient()
            cls._instance._connect()
        return cls._instance

    def _connect(self):
        if not self._enabled:
            logger.warning("neo4j.disabled")
            return

        uri = f"bolt://{settings.neo4j.host}:{settings.neo4j.port}"
        self._driver = GraphDatabase.driver(
            uri,
            auth=(settings.neo4j.username, settings.neo4j.password),
            max_connection_lifetime=3600,
            max_connection_pool_size=50,
        )
        logger.info("neo4j.connected", uri=uri)

    async def _connect_async(self):
        """异步连接（按需初始化）"""
        if not self._enabled:
            return
        if self._async_driver is None:
            uri = f"bolt://{settings.neo4j.host}:{settings.neo4j.port}"
            self._async_driver = AsyncGraphDatabase.driver(
                uri,
                auth=(settings.neo4j.username, settings.neo4j.password),
                max_connection_lifetime=3600,
                max_connection_pool_size=50,
            )
            logger.info("neo4j.async_connected", uri=uri)

    def verify_connectivity(self) -> bool:
        if not self._driver:
            return False
        try:
            with self._driver.session() as session:
                session.run("RETURN 1")
            return True
        except Exception as e:
            logger.error("neo4j.ping.failed", error=str(e))
            return False

    def run_query(self, cypher: str, **params) -> list[dict]:
        """同步查询"""
        if not self._driver:
            logger.warning("neo4j.query.skipped", reason="driver_not_initialized")
            return []

        try:
            with self._driver.session(database=settings.neo4j.database) as session:
                result = session.run(cypher, **params)
                return [dict(record) for record in result]
        except ServiceUnavailable as e:
            logger.error("neo4j.query.unavailable", error=str(e))
            return []
        except AuthError as e:
            logger.error("neo4j.query.auth_failed", error=str(e))
            return []
        except Exception as e:
            logger.error("neo4j.query.error", error=str(e), cypher=cypher[:100])
            return []

    async def run_query_async(self, cypher: str, **params) -> list[dict]:
        """异步查询"""
        if not self._enabled:
            return []

        await self._connect_async()
        if not self._async_driver:
            return []

        try:
            async with self._async_driver.session(database=settings.neo4j.database) as session:
                result = await session.run(cypher, **params)
                records = await result.data()
                return records
        except ServiceUnavailable as e:
            logger.error("neo4j.async_query.unavailable", error=str(e))
            return []
        except AuthError as e:
            logger.error("neo4j.async_query.auth_failed", error=str(e))
            return []
        except Exception as e:
            logger.error("neo4j.async_query.error", error=str(e), cypher=cypher[:100])
            return []

    def close(self):
        """关闭所有连接"""
        if self._driver:
            self._driver.close()
            self._driver = None
            logger.info("neo4j.driver_closed")

        if self._async_driver:
            import asyncio
            try:
                loop = asyncio.get_event_loop()
                if loop.is_running():
                    asyncio.create_task(self._async_driver.close())
                else:
                    loop.run_until_complete(self._async_driver.close())
            except Exception:
                pass
            self._async_driver = None
