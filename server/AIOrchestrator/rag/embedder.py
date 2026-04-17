"""
Embedding 管理器 — 支持 sentence-transformers、Ollama Embedding、OpenAI Embedding
"""
from typing import Optional
import structlog
from qdrant_client import QdrantClient
from qdrant_client.http import models as qdrant_models
from langchain_qdrant import QdrantVectorStore
from langchain_core.documents import Document
from sentence_transformers import SentenceTransformer
import httpx

from config import settings

logger = structlog.get_logger()


class EmbeddingManager:
    def __init__(self, config):
        self.config = config
        self._local_model = None
        self._qdrant_client: QdrantClient | None = None
        self._local_embedder: SentenceTransformer | None = None

    async def _get_local_embedder(self) -> SentenceTransformer:
        if self._local_embedder is None:
            self._local_embedder = SentenceTransformer(self.config.local_model)
            logger.info("embedding.local_loaded", model=self.config.local_model)
        return self._local_embedder

    def _get_qdrant_client(self) -> QdrantClient:
        if self._qdrant_client is None:
            self._qdrant_client = QdrantClient(
                host=settings.rag.qdrant.host,
                port=settings.rag.qdrant.port,
            )
        return self._qdrant_client

    async def embed(self, texts: list[str], backend: str | None = None) -> list[list[float]]:
        """
        将文本列表向量化。
        backend: "local" (sentence-transformers) / "ollama" / "openai"
        """
        backend = backend or self.config.backend

        if backend == "local":
            model = await self._get_local_embedder()
            return model.encode(texts).tolist()

        elif backend == "ollama":
            return await self._embed_ollama(texts)

        elif backend == "openai":
            return await self._embed_openai(texts)

        else:
            model = await self._get_local_embedder()
            return model.encode(texts).tolist()

    async def _embed_ollama(self, texts: list[str]) -> list[list[float]]:
        base_url = f"http://127.0.0.1:11434"
        model = self.config.ollama_model
        vectors = []

        async with httpx.AsyncClient(timeout=30.0) as client:
            for text in texts:
                resp = await client.post(
                    f"{base_url}/api/embeddings",
                    json={"model": model, "prompt": text}
                )
                resp.raise_for_status()
                data = resp.json()
                vectors.append(data["embedding"])

        return vectors

    async def _embed_openai(self, texts: list[str]) -> list[list[float]]:
        import os
        api_key = os.environ.get("OPENAI_API_KEY", "")
        if not api_key:
            model = await self._get_local_embedder()
            return model.encode(texts).tolist()

        async with httpx.AsyncClient(timeout=30.0) as client:
            resp = await client.post(
                "https://api.openai.com/v1/embeddings",
                json={
                    "model": self.config.openai_model,
                    "input": texts,
                },
                headers={"Authorization": f"Bearer {api_key}"},
            )
            resp.raise_for_status()
            data = resp.json()
            return [item["embedding"] for item in data["data"]]

    async def embed_query(self, query: str) -> list[float]:
        """查询向量（单条）"""
        vectors = await self.embed([query])
        return vectors[0]

    def get_dimension(self) -> int:
        return self.config.dimension
