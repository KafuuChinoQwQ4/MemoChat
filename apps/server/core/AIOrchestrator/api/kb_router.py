"""
RAG 知识库 API 路由
"""
import structlog
from fastapi import APIRouter, HTTPException

from harness import HarnessContainer
from observability.langsmith_instrument import set_run_error, set_run_output, trace_context
from observability.metrics import ai_metrics
from schemas.api import (
    KbChunk,
    KbDeleteRsp,
    KbListRsp,
    KbSearchReq,
    KbSearchRsp,
    KbUploadReq,
    KbUploadRsp,
)

logger = structlog.get_logger()
router = APIRouter()


@router.post("/upload", response_model=KbUploadRsp)
async def upload(req: KbUploadReq):
    with trace_context(
        "kb_upload",
        run_type="tool",
        inputs=req.model_dump(),
        metadata={"route": "/kb/upload", "file_name": req.file_name, "file_type": req.file_type},
        tags=["rag", "knowledge-base"],
    ) as run:
        try:
            container = HarnessContainer.get_instance()
            result = await container.knowledge_service.upload(req.uid, req.file_name, req.file_type, req.content)
            ai_metrics.http_requests.inc(route="/kb/upload", status="ok")
            set_run_output(run, {"chunks": result["chunks"], "kb_id": result["kb_id"]})
            return KbUploadRsp(code=0, message="ok", chunks=result["chunks"], kb_id=result["kb_id"])
        except ValueError as exc:
            set_run_error(run, exc)
            ai_metrics.http_requests.inc(route="/kb/upload", status="error")
            raise HTTPException(status_code=400, detail=str(exc)) from exc
        except Exception as exc:
            set_run_error(run, exc)
            ai_metrics.http_requests.inc(route="/kb/upload", status="error")
            logger.error("kb.upload.error", uid=req.uid, file_name=req.file_name, error=str(exc))
            raise HTTPException(status_code=500, detail=str(exc)) from exc


@router.post("/search", response_model=KbSearchRsp)
async def search(req: KbSearchReq):
    if not req.query.strip():
        ai_metrics.http_requests.inc(route="/kb/search", status="error")
        raise HTTPException(status_code=400, detail="query cannot be empty")
    with trace_context(
        "kb_search",
        run_type="retriever",
        inputs=req.model_dump(),
        metadata={"route": "/kb/search", "top_k": req.top_k},
        tags=["rag", "knowledge-base"],
    ) as run:
        try:
            container = HarnessContainer.get_instance()
            results = await container.knowledge_service.search(req.uid, req.query, req.top_k)
            ai_metrics.http_requests.inc(route="/kb/search", status="ok")
            set_run_output(run, {"chunk_count": len(results), "scores": [result.get("score", 0.0) for result in results]})
            return KbSearchRsp(
                code=0,
                chunks=[
                    KbChunk(
                        content=result.get("content", ""),
                        score=result.get("score", 0.0),
                        source=result.get("source", ""),
                        kb_id=result.get("kb_id", ""),
                    )
                    for result in results
                ],
            )
        except Exception as exc:
            set_run_error(run, exc)
            ai_metrics.http_requests.inc(route="/kb/search", status="error")
            logger.error("kb.search.error", uid=req.uid, query=req.query, error=str(exc))
            raise HTTPException(status_code=500, detail=str(exc)) from exc


@router.get("/list", response_model=KbListRsp)
async def list_kb(uid: int):
    try:
        container = HarnessContainer.get_instance()
        result = KbListRsp(code=0, knowledge_bases=await container.knowledge_service.list(uid))
        ai_metrics.http_requests.inc(route="/kb/list", status="ok")
        return result
    except Exception as exc:
        ai_metrics.http_requests.inc(route="/kb/list", status="error")
        logger.error("kb.list.error", uid=uid, error=str(exc))
        raise HTTPException(status_code=500, detail=str(exc)) from exc


@router.delete("/{kb_id}", response_model=KbDeleteRsp)
async def delete_kb(kb_id: str, uid: int):
    try:
        container = HarnessContainer.get_instance()
        await container.knowledge_service.delete(uid, kb_id)
        ai_metrics.http_requests.inc(route="/kb/delete", status="ok")
        return KbDeleteRsp(code=0, message="ok")
    except Exception as exc:
        ai_metrics.http_requests.inc(route="/kb/delete", status="error")
        logger.error("kb.delete.error", uid=uid, kb_id=kb_id, error=str(exc))
        raise HTTPException(status_code=500, detail=str(exc)) from exc
