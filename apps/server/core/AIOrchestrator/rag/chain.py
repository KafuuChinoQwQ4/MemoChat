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
from rag.retrieval import (
    CrossEncoderReranker,
    RetrievalCandidate,
    build_metadata_filter,
    candidate_from_payload,
    collection_matches_filters,
    fuse_candidates,
    metadata_matches_payload,
    rank_lexical_candidates,
)

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
        self._reranker: CrossEncoderReranker | None = None

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

    def _get_reranker(self) -> CrossEncoderReranker:
        if self._reranker is None:
            self._reranker = CrossEncoderReranker(
                model_name=settings.rag.retrieval.rerank_model,
                enabled=settings.rag.retrieval.rerank_enabled,
                batch_size=settings.rag.retrieval.rerank_batch_size,
            )
        return self._reranker

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
                            "source": chunk.metadata.get("source", collection),
                            "file_type": chunk.metadata.get("file_type", ""),
                            "page": chunk.metadata.get("page"),
                            "total_chunks": chunk.metadata.get("total_chunks", len(chunks)),
                            "content_length": len(chunk.page_content),
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

    async def retrieve(
        self,
        uid: int,
        query: str,
        top_k: int,
        embedder,
        metadata_filters: dict[str, Any] | None = None,
    ) -> list[dict]:
        """
        检索与查询最相关的文档片段。
        """
        with trace_context(
            "rag_retrieve",
            run_type="retriever",
            inputs={"uid": uid, "query": query, "top_k": top_k, "metadata_filters": metadata_filters or {}},
            metadata={
                "operation": "hybrid_retrieval",
                "score_threshold": settings.rag.score_threshold,
                "dense_top_k": settings.rag.retrieval.dense_top_k,
                "lexical_top_k": settings.rag.retrieval.lexical_top_k,
            },
            tags=["rag", "qdrant", "bm25", "rerank"],
        ) as run:
            try:
                results = await self._retrieve_inner(uid, query, top_k, embedder, metadata_filters or {})
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

    def _collection_candidates(self, collections: list[str], uid: int, prefix: str, metadata_filters: dict[str, Any]) -> list[str]:
        return [collection for collection in collections if collection_matches_filters(collection, prefix, uid, metadata_filters)]

    def _search_points(
        self,
        client: QdrantClient,
        collection: str,
        query_vector: list[float],
        limit: int,
        query_filter,
        threshold: float | None,
    ):
        search_method = getattr(client, "search", None)
        if callable(search_method):
            try:
                return search_method(
                    collection_name=collection,
                    query_vector=query_vector,
                    limit=limit,
                    score_threshold=threshold,
                    query_filter=query_filter,
                )
            except TypeError:
                return search_method(
                    collection_name=collection,
                    query_vector=query_vector,
                    limit=limit,
                    score_threshold=threshold,
                )

        response = client.query_points(
            collection_name=collection,
            query=query_vector,
            query_filter=query_filter,
            limit=limit,
            score_threshold=threshold,
            with_payload=True,
        )
        return getattr(response, "points", response)

    def _scroll_points(self, client: QdrantClient, collection: str, scroll_filter, limit: int) -> list[Any]:
        collected: list[Any] = []
        offset = None
        batch_size = min(128, max(limit, 1))
        while len(collected) < limit:
            batch, offset = client.scroll(
                collection_name=collection,
                scroll_filter=scroll_filter,
                limit=min(batch_size, limit - len(collected)),
                offset=offset,
                with_payload=True,
                with_vectors=False,
            )
            if not batch:
                break
            collected.extend(batch)
            if offset is None:
                break
        return collected

    async def _retrieve_inner(
        self,
        uid: int,
        query: str,
        top_k: int,
        embedder,
        metadata_filters: dict[str, Any],
    ) -> list[dict]:
        client = self._get_client()
        prefix = settings.rag.qdrant.collection_prefix
        hybrid_config = settings.rag.retrieval

        threshold = settings.rag.score_threshold

        try:
            collections = [c.name for c in client.get_collections().collections]
        except Exception:
            collections = []

        user_collections = [
            collection
            for collection in collections
            if collection.startswith(prefix) and f"_{uid}_" in collection
        ]
        user_collections = self._collection_candidates(user_collections, uid, prefix, metadata_filters)
        if not user_collections:
            return []

        query_vector = await embedder.embed_query(query)
        query_filter = build_metadata_filter(uid, metadata_filters)

        dense_candidates: list[RetrievalCandidate] = []
        lexical_candidates: list[RetrievalCandidate] = []
        dense_limit = max(top_k * 4, hybrid_config.dense_top_k, hybrid_config.candidate_pool_size)
        lexical_scan_limit = max(top_k * 8, hybrid_config.lexical_top_k, hybrid_config.candidate_pool_size * 2)

        for collection in user_collections:
            try:
                search_results = self._search_points(
                    client,
                    collection,
                    query_vector,
                    dense_limit,
                    query_filter,
                    threshold,
                )
                if not search_results and query_filter is not None:
                    search_results = self._search_points(client, collection, query_vector, dense_limit, None, threshold)
                if not search_results and threshold:
                    search_results = self._search_points(client, collection, query_vector, dense_limit, query_filter, None)
                if not search_results and query_filter is not None:
                    search_results = self._search_points(client, collection, query_vector, dense_limit, None, None)

                for hit in search_results:
                    payload = getattr(hit, "payload", {}) or {}
                    candidate = candidate_from_payload(
                        candidate_id=getattr(hit, "id", ""),
                        payload=payload,
                        score=float(getattr(hit, "score", 0.0) or 0.0),
                        collection=collection,
                        route="dense",
                    )
                    if metadata_matches_payload(candidate.metadata, metadata_filters):
                        dense_candidates.append(candidate)
            except Exception as exc:
                logger.warning("qdrant.search_failed", collection=collection, error=str(exc))

            try:
                scroll_results = self._scroll_points(client, collection, query_filter, lexical_scan_limit)
                if not scroll_results and query_filter is not None:
                    scroll_results = self._scroll_points(client, collection, None, lexical_scan_limit)

                coll_lexical_candidates: list[RetrievalCandidate] = []
                for record in scroll_results:
                    payload = getattr(record, "payload", {}) or {}
                    candidate = candidate_from_payload(
                        candidate_id=getattr(record, "id", ""),
                        payload=payload,
                        score=0.0,
                        collection=collection,
                        route="bm25",
                    )
                    if metadata_matches_payload(candidate.metadata, metadata_filters):
                        coll_lexical_candidates.append(candidate)

                lexical_candidates.extend(rank_lexical_candidates(coll_lexical_candidates, query))
            except Exception as exc:
                logger.warning("qdrant.scroll_failed", collection=collection, error=str(exc))

        fused_candidates = fuse_candidates(
            {"dense": dense_candidates, "bm25": lexical_candidates},
            route_weights={
                "dense": hybrid_config.dense_weight,
                "bm25": hybrid_config.lexical_weight,
            },
            rrf_k=hybrid_config.rrf_k,
        )

        if metadata_filters:
            fused_candidates = [
                candidate
                for candidate in fused_candidates
                if metadata_matches_payload(candidate.metadata, metadata_filters)
            ]

        if not fused_candidates:
            return []

        candidate_pool = fused_candidates[: max(top_k, hybrid_config.candidate_pool_size)]
        rerank_limit = min(
            len(candidate_pool),
            max(top_k, hybrid_config.rerank_candidate_limit),
        )
        reranked = await self._get_reranker().rerank(query, candidate_pool[:rerank_limit], rerank_limit)
        final_candidates = reranked if reranked else candidate_pool
        return [candidate.to_dict() for candidate in final_candidates[:top_k]]

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
