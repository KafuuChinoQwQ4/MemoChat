"""
RAG Chain — 检索增强生成链
文档上传 → Qdrant 存储 → 检索 → 注入 LLM
"""
from typing import Any
import uuid

import structlog
from qdrant_client import QdrantClient
from qdrant_client.http import models as qdrant_models
from langchain_core.embeddings import Embeddings
from langchain_core.documents import Document
from langchain_core.output_parsers import StrOutputParser
from langchain_core.prompts import PromptTemplate
from langchain_core.runnables import RunnablePassthrough

from config import settings
from observability.langsmith_instrument import set_run_error, set_run_output, trace_context

logger = structlog.get_logger()


class QdrantEmbeddings(Embeddings):
    """包装 EmbeddingManager 为 LangChain Embeddings 接口"""

    def __init__(self, embedder):
        self._embedder = embedder

    async def aembed_documents(self, texts: list[str]) -> list[list[float]]:
        return await self._embedder.embed(texts)

    async def aembed_query(self, text: str) -> list[float]:
        return await self._embedder.embed_query(text)

    def embed_documents(self, texts: list[str]) -> list[list[float]]:
        import asyncio
        return asyncio.get_event_loop().run_until_complete(self.aembed_documents(texts))

    def embed_query(self, text: str) -> list[float]:
        import asyncio
        return asyncio.get_event_loop().run_until_complete(self.aembed_query(text))


class RAGChain:
    """
    RAG 检索增强生成链。
    支持上传文档到 Qdrant、检索相似片段。
    """

    def __init__(self):
        self._qdrant_client: QdrantClient | None = None

    def _get_client(self) -> QdrantClient:
        if self._qdrant_client is None:
            self._qdrant_client = QdrantClient(
                host=settings.rag.qdrant.host,
                port=settings.rag.qdrant.port,
            )
        return self._qdrant_client

    def _collection_name(self, uid: int, kb_id: str) -> str:
        prefix = settings.rag.qdrant.collection_prefix
        return f"{prefix}{uid}_{kb_id}"

    async def add_documents(self, uid: int, kb_id: str, chunks: list[Document], embedder) -> None:
        """
        将文档块存入 Qdrant。
        """
        collection = self._collection_name(uid, kb_id)
        client = self._get_client()
        dim = embedder.get_dimension()
        with trace_context(
            "rag_add_documents",
            run_type="retriever",
            inputs={"uid": uid, "kb_id": kb_id, "chunk_count": len(chunks), "dimension": dim},
            metadata={"collection": collection, "operation": "qdrant_upsert"},
            tags=["rag", "qdrant"],
        ) as run:
            try:
                await self._add_documents_inner(uid, kb_id, chunks, embedder, collection, client, dim)
                set_run_output(run, {"collection": collection, "chunks": len(chunks)})
            except Exception as exc:
                set_run_error(run, exc)
                raise

    async def _add_documents_inner(self, uid: int, kb_id: str, chunks: list[Document], embedder, collection: str, client: QdrantClient, dim: int) -> None:

        try:
            collections = [c.name for c in client.get_collections().collections]
        except Exception:
            collections = []

        if collection not in collections:
            try:
                client.create_collection(
                    collection_name=collection,
                    vectors_config=qdrant_models.VectorParams(
                        size=dim,
                        distance=qdrant_models.Distance.COSINE,
                    ),
                )
                logger.info("qdrant.collection_created", collection=collection)
            except Exception as e:
                logger.warning("qdrant.collection_exists", collection=collection, error=str(e))

        try:
            vectors = await embedder.embed([chunk.page_content for chunk in chunks])
            points = []
            for index, (chunk, vector) in enumerate(zip(chunks, vectors)):
                points.append(
                    qdrant_models.PointStruct(
                        id=uuid.uuid4().hex,
                        vector=vector,
                        payload={
                            "page_content": chunk.page_content,
                            "metadata": dict(chunk.metadata),
                            "kb_id": kb_id,
                            "uid": uid,
                            "chunk_index": index,
                        },
                    )
                )
            client.upsert(collection_name=collection, points=points)
            logger.info(
                "qdrant.documents_added",
                collection=collection,
                chunks=len(points),
            )
        except Exception as e:
            logger.error("qdrant.add_failed", collection=collection, error=str(e))
            raise

    async def retrieve(self, uid: int, query: str, top_k: int, embedder) -> list[dict]:
        """
        检索与查询最相关的文档片段。
        """
        with trace_context(
            "rag_retrieve",
            run_type="retriever",
            inputs={"uid": uid, "query": query, "top_k": top_k},
            metadata={"operation": "qdrant_search", "score_threshold": settings.rag.score_threshold},
            tags=["rag", "qdrant"],
        ) as run:
            try:
                results = await self._retrieve_inner(uid, query, top_k, embedder)
                set_run_output(
                    run,
                    {
                        "hit_count": len(results),
                        "scores": [round(float(item.get("score", 0.0)), 4) for item in results],
                        "kb_ids": [item.get("kb_id", "") for item in results],
                    },
                )
                return results
            except Exception as exc:
                set_run_error(run, exc)
                raise

    async def _retrieve_inner(self, uid: int, query: str, top_k: int, embedder) -> list[dict]:
        client = self._get_client()
        prefix = settings.rag.qdrant.collection_prefix

        all_results = []
        threshold = settings.rag.score_threshold

        try:
            collections = [c.name for c in client.get_collections().collections]
        except Exception:
            collections = []

        user_collections = [c for c in collections if c.startswith(prefix) and f"_{uid}_" in c]
        query_vector = await embedder.embed_query(query) if user_collections else []

        for coll in user_collections:
            try:
                search_results = client.search(
                    collection_name=coll,
                    query_vector=query_vector,
                    limit=top_k,
                    score_threshold=threshold,
                )
                if not search_results and threshold:
                    search_results = client.search(
                        collection_name=coll,
                        query_vector=query_vector,
                        limit=top_k,
                    )

                for hit in search_results:
                    all_results.append({
                        "content": hit.payload.get("page_content", ""),
                        "score": hit.score,
                        "source": hit.payload.get("metadata", {}).get("source", coll),
                        "kb_id": coll.replace(f"{prefix}{uid}_", ""),
                    })

            except Exception as e:
                logger.warning("qdrant.search_failed", collection=coll, error=str(e))
                continue

        all_results.sort(key=lambda x: x["score"], reverse=True)
        return all_results[:top_k]

    async def list_knowledge_bases(self, uid: int) -> list[dict]:
        """列出用户的所有知识库"""
        client = self._get_client()
        prefix = settings.rag.qdrant.collection_prefix

        try:
            collections = [c.name for c in client.get_collections().collections]
        except Exception:
            return []

        user_kbs = []
        for coll in collections:
            if not coll.startswith(prefix) or f"_{uid}_" not in coll:
                continue
            kb_id = coll.replace(f"{prefix}{uid}_", "")
            try:
                info = client.get_collection(collection_name=coll)
                user_kbs.append({
                    "kb_id": kb_id,
                    "name": kb_id,
                    "chunk_count": info.vectors_count,
                    "status": "ready",
                })
            except Exception:
                user_kbs.append({"kb_id": kb_id, "name": kb_id, "chunk_count": 0, "status": "error"})

        return user_kbs

    async def delete_knowledge_base(self, uid: int, kb_id: str) -> None:
        """删除知识库"""
        collection = self._collection_name(uid, kb_id)
        client = self._get_client()
        try:
            client.delete_collection(collection_name=collection)
            logger.info("qdrant.collection_deleted", collection=collection)
        except Exception as e:
            logger.warning("qdrant.delete_failed", collection=collection, error=str(e))
