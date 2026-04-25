from __future__ import annotations

import base64
import time
import uuid

import structlog

from config import settings
from db.postgres_client import PostgresClient
from rag.chain import RAGChain
from rag.doc_processor import DocProcessor
from rag.embedder import EmbeddingManager

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


class KnowledgeService:
    def __init__(self):
        self._rag = RAGChain()
        self._processor = DocProcessor(settings.rag)
        self._embedder = EmbeddingManager(settings.embedding)

    async def upload(self, uid: int, file_name: str, file_type: str, content_b64: str) -> dict:
        file_type = file_type.lower().strip()
        if file_type not in {"pdf", "txt", "md", "docx"}:
            raise ValueError(f"Unsupported file_type: {file_type}")

        try:
            raw_bytes = base64.b64decode(content_b64)
        except Exception as exc:
            raise ValueError(f"Invalid Base64 content: {exc}") from exc

        kb_id = f"kb_{uuid.uuid4().hex[:12]}"
        chunks = await self._processor.process(raw_bytes, file_type, file_name)
        if not chunks:
            raise ValueError("No content extracted from file")

        await self._rag.add_documents(uid, kb_id, chunks, self._embedder)
        await self._persist_metadata(uid, kb_id, file_name, file_type, len(chunks))

        return {"kb_id": kb_id, "chunks": len(chunks)}

    async def search(self, uid: int, query: str, top_k: int | None = None) -> list[dict]:
        effective_top_k = top_k or settings.harness.knowledge_top_k or settings.rag.top_k
        return await self._rag.retrieve(uid, query, effective_top_k, self._embedder)

    async def list(self, uid: int) -> list[dict]:
        try:
            pg = PostgresClient()
            rows = await pg.fetchall(
                """
                SELECT kb_id, name, file_type, chunk_count, status, created_at, updated_at
                FROM ai_knowledge_base
                WHERE uid = $1 AND deleted_at IS NULL
                ORDER BY created_at DESC
                """,
                uid,
            )
            return rows
        except Exception as exc:
            logger.warning("knowledge.list.db_fallback", uid=uid, error=str(exc))
            return await self._rag.list_knowledge_bases(uid)

    async def delete(self, uid: int, kb_id: str) -> None:
        await self._rag.delete_knowledge_base(uid, kb_id)
        try:
            pg = PostgresClient()
            await pg.execute(
                """
                UPDATE ai_knowledge_base
                SET status = 'deleted', deleted_at = $3, updated_at = $3
                WHERE uid = $1 AND kb_id = $2 AND deleted_at IS NULL
                """,
                uid,
                kb_id,
                _now_ms(),
            )
        except Exception as exc:
            logger.warning("knowledge.delete.metadata_failed", uid=uid, kb_id=kb_id, error=str(exc))

    async def _persist_metadata(self, uid: int, kb_id: str, file_name: str, file_type: str, chunk_count: int) -> None:
        try:
            pg = PostgresClient()
            now = _now_ms()
            await pg.execute(
                """
                INSERT INTO ai_knowledge_base
                (kb_id, uid, name, file_type, chunk_count, qdrant_collection, status, created_at, updated_at)
                VALUES ($1, $2, $3, $4, $5, $6, 'ready', $7, $7)
                """,
                kb_id,
                uid,
                file_name,
                file_type,
                chunk_count,
                f"{settings.rag.qdrant.collection_prefix}{uid}_{kb_id}",
                now,
            )
        except Exception as exc:
            logger.warning("knowledge.metadata_persist_failed", uid=uid, kb_id=kb_id, error=str(exc))
