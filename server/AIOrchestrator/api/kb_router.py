"""
RAG 知识库 API 路由
POST /kb/upload — 上传文档
POST /kb/search — 检索
GET  /kb/list  — 列表
DELETE /kb/{kb_id} — 删除
"""
import base64
import structlog
from fastapi import APIRouter, HTTPException

from schemas.api import (
    KbUploadReq, KbUploadRsp,
    KbSearchReq, KbSearchRsp, KbChunk,
    KbListRsp, KbDeleteReq, KbDeleteRsp,
)
from rag.chain import RAGChain
from rag.doc_processor import DocProcessor
from rag.embedder import EmbeddingManager
from config import settings

logger = structlog.get_logger()
router = APIRouter()

_rag_chain: RAGChain | None = None
_embed_manager: EmbeddingManager | None = None


def get_rag_chain() -> RAGChain:
    global _rag_chain
    if _rag_chain is None:
        _rag_chain = RAGChain()
    return _rag_chain


def get_embed_manager() -> EmbeddingManager:
    global _embed_manager
    if _embed_manager is None:
        _embed_manager = EmbeddingManager(settings.embedding)
    return _embed_manager


@router.post("/upload", response_model=KbUploadRsp)
async def upload(req: KbUploadReq):
    """
    上传文档到知识库。
    流程：Base64 解码 → 文档加载 → 分块 → Embedding → 存入 Qdrant
    """
    uid = req.uid
    file_name = req.file_name
    file_type = req.file_type.lower()
    kb_id = f"kb_{uid}_{file_name}"

    if file_type not in ("pdf", "txt", "md", "docx"):
        raise HTTPException(status_code=400, detail=f"Unsupported file_type: {file_type}")

    try:
        file_content = base64.b64decode(req.content)
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Invalid Base64 content: {e}")

    try:
        processor = DocProcessor(settings.rag)
        chunks = await processor.process(file_content, file_type, file_name)

        if not chunks:
            raise HTTPException(status_code=422, detail="No content extracted from file")

        embed_mgr = get_embed_manager()
        rag = get_rag_chain()

        await rag.add_documents(uid, kb_id, chunks, embed_mgr)

        return KbUploadRsp(
            code=0,
            message="ok",
            chunks=len(chunks),
            kb_id=kb_id,
        )

    except Exception as e:
        logger.error("kb.upload.error", uid=uid, file_name=file_name, error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/search", response_model=KbSearchRsp)
async def search(req: KbSearchReq):
    """
    在知识库中检索相关内容。
    """
    uid = req.uid
    query = req.query
    top_k = req.top_k or settings.rag.top_k

    if not query.strip():
        raise HTTPException(status_code=400, detail="query cannot be empty")

    try:
        rag = get_rag_chain()
        embed_mgr = get_embed_manager()
        results = await rag.retrieve(uid, query, top_k, embed_mgr)

        chunks = []
        for r in results:
            chunks.append(KbChunk(
                content=r["content"],
                score=r.get("score", 0.0),
                source=r.get("source", ""),
            ))

        return KbSearchRsp(code=0, chunks=chunks)

    except Exception as e:
        logger.error("kb.search.error", uid=uid, query=query, error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/list", response_model=KbListRsp)
async def list_kb(uid: int):
    """
    列出用户的所有知识库。
    """
    try:
        rag = get_rag_chain()
        kbs = await rag.list_knowledge_bases(uid)
        return KbListRsp(code=0, knowledge_bases=kbs)
    except Exception as e:
        logger.error("kb.list.error", uid=uid, error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


@router.delete("/{kb_id}", response_model=KbDeleteRsp)
async def delete_kb(kb_id: str, uid: int):
    """
    删除知识库。
    """
    try:
        rag = get_rag_chain()
        await rag.delete_knowledge_base(uid, kb_id)
        return KbDeleteRsp(code=0, message="ok")
    except Exception as e:
        logger.error("kb.delete.error", kb_id=kb_id, error=str(e))
        raise HTTPException(status_code=500, detail=str(e))
