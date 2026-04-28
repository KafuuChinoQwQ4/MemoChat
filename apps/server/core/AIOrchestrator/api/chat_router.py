"""
聊天对话 API 路由
POST /chat — 普通对话
POST /chat/stream — SSE 流式输出
"""
import json
import uuid
from typing import AsyncIterator

import structlog
from fastapi import APIRouter, HTTPException
from fastapi.responses import StreamingResponse

from harness import HarnessContainer
from schemas.api import AgentRunReq, ChatReq, ChatRsp

logger = structlog.get_logger()
router = APIRouter()


def _to_agent_request(req: ChatReq, session_id: str) -> AgentRunReq:
    return AgentRunReq(
        uid=req.uid,
        session_id=session_id,
        content=req.content,
        model_type=req.model_type,
        model_name=req.model_name,
        deployment_preference=req.deployment_preference,
        skill_name=req.skill_name,
        requested_tools=req.requested_tools,
        tool_arguments=req.tool_arguments,
        metadata=req.metadata,
    )


@router.post("", response_model=ChatRsp)
async def chat(req: ChatReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")

    session_id = req.session_id or uuid.uuid4().hex

    try:
        container = HarnessContainer.get_instance()
        result = await container.agent_service.run_turn(_to_agent_request(req, session_id))
        return ChatRsp(
            code=0,
            message="ok",
            session_id=result.session_id,
            content=result.content,
            tokens=result.tokens,
            model=result.model,
            trace_id=result.trace_id,
            skill=result.skill,
            feedback_summary=result.feedback_summary,
            observations=result.observations,
        )
    except Exception as exc:
        logger.error("chat.error", error=str(exc), uid=req.uid, session_id=session_id)
        raise HTTPException(status_code=500, detail=str(exc)) from exc


@router.post("/stream")
async def chat_stream(req: ChatReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")

    session_id = req.session_id or uuid.uuid4().hex

    async def event_generator() -> AsyncIterator[bytes]:
        container = HarnessContainer.get_instance()
        async for payload in container.agent_service.stream_turn(_to_agent_request(req, session_id)):
            yield f"data: {json.dumps(payload, ensure_ascii=False)}\n\n".encode("utf-8")

    return StreamingResponse(
        event_generator(),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "Connection": "keep-alive",
            "X-Accel-Buffering": "no",
        },
    )
