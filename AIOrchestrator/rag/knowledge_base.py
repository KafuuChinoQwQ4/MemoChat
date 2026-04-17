import logging
import base64
from typing import List, Dict, Any, Optional
from qdrant_client import QdrantClient
from qdrant_client.models import Distance, VectorParams, PointStruct
import uuid

from config import settings

logger = logging.getLogger(__name__)


class KnowledgeBaseManager:
    """RAG 知识库管理器：负责文档上传、分块、向量化、存储和检索"""

    def __init__(self):
        self._client: Optional[QdrantClient] = None
        self._collection = settings.qdrant_collection_name
        self._embedding_dim = 384

    @property
    def client(self) -> QdrantClient:
        if self._client is None:
            self._client = QdrantClient(
                host=settings.qdrant_host,
                port=settings.qdrant_port,
            )
            self._ensure_collection()
        return self._client

    def _ensure_collection(self):
        try:
            collections = [c.name for c in self.client.get_collections().collections]
            if self._collection not in collections:
                self.client.create_collection(
                    collection_name=self._collection,
                    vectors_config=VectorParams(size=self._embedding_dim, distance=Distance.COSINE),
                )
                logger.info(f"Created Qdrant collection: {self._collection}")
        except Exception as e:
            logger.warning(f"Failed to ensure collection: {e}")

    def _get_embedding(self, text: str) -> List[float]:
        """使用 sentence-transformers 生成向量"""
        from sentence_transformers import SentenceTransformer
        model = SentenceTransformer(settings.embeddings_model)
        return model.encode(text, normalize_embeddings=True).tolist()

    def _chunk_text(self, text: str, chunk_size: int = 500, overlap: int = 50) -> List[str]:
        """简单分块：按段落或固定大小分块"""
        import re
        sentences = re.split(r'(?<=[。！？.!?])\s+', text)
        chunks = []
        current = ""
        for sent in sentences:
            if len(current) + len(sent) <= chunk_size:
                current += sent
            else:
                if current:
                    chunks.append(current.strip())
                current = sent[-overlap:] + sent if len(sent) > overlap else sent
        if current:
            chunks.append(current.strip())
        return chunks

    async def upload_document(self, uid: int, file_name: str,
                               file_type: str, base64_content: str) -> str:
        """上传文档，返回 kb_id"""
        try:
            content = base64.b64decode(base64_content).decode("utf-8", errors="ignore")
        except Exception:
            content = base64_content

        chunks = self._chunk_text(content)
        kb_id = str(uuid.uuid4())

        points = []
        for i, chunk in enumerate(chunks):
            embedding = self._get_embedding(chunk)
            payload = {
                "uid": uid,
                "kb_id": kb_id,
                "file_name": file_name,
                "file_type": file_type,
                "chunk_index": i,
                "content": chunk,
            }
            points.append(PointStruct(
                id=str(uuid.uuid4()),
                vector=embedding,
                payload=payload,
            ))

        self.client.upsert(
            collection_name=self._collection,
            points=points,
        )

        logger.info(f"Uploaded document: kb_id={kb_id}, chunks={len(chunks)}")
        return kb_id

    def search_sync(self, uid: int, query: str, top_k: int = 5) -> List[Dict[str, Any]]:
        """同步检索"""
        return self._search(uid, query, top_k)

    async def search(self, uid: int, query: str, top_k: int = 5) -> List[Dict[str, Any]]:
        """异步检索"""
        import asyncio
        loop = asyncio.get_event_loop()
        return await loop.run_in_executor(None, self._search, uid, query, top_k)

    def _search(self, uid: int, query: str, top_k: int) -> List[Dict[str, Any]]:
        try:
            embedding = self._get_embedding(query)
            results = self.client.search(
                collection_name=self._collection,
                query_vector=embedding,
                query_filter=None,
                limit=top_k,
            )
            return [
                {
                    "content": r.payload.get("content", ""),
                    "score": r.score,
                    "source": r.payload.get("file_name", "unknown"),
                }
                for r in results
            ]
        except Exception as e:
            logger.error(f"Qdrant search failed: {e}")
            return []
