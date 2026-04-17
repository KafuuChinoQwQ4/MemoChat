"""
聊天对话 API 路由
POST /chat — 支持普通对话和流式输出（SSE）
"""
import asyncio
import uuid
from typing import AsyncIterator

import structlog
from fastapi import APIRouter, HTTPException, BackgroundTasks
from fastapi.responses import StreamingResponse

from schemas.api import ChatReq, ChatRsp, ChatChunk
from llm import LLMManager, LLMMessage
from langgraph.agent import build_agent, run_agent, run_agent_stream
from memory.conversation_memory import MemoryManager

logger = structlog.get_logger()
router = APIRouter()


async def generate_chat_stream(uid: int, session_id: str, content: str,
                                model_type: str, model_name: str) -> AsyncIterator[ChatChunk]:
    """流式生成聊天响应"""
    msg_id = str(uuid.uuid4())
    total_tokens = 0
    accumulated = ""

    try:
        memory = MemoryManager(session_id=session_id, uid=uid)
        history = await memory.load_context()

        messages = history + [LLMMessage(role="user", content=content)]

        agent = build_agent()
        accumulated_text = ""

        async for delta in run_agent(agent, messages, model_type=model_type, model_name=model_name):
            accumulated_text += delta
            chunk = ChatChunk(
                chunk=delta,
                is_final=False,
                msg_id=msg_id,
                total_tokens=0,
            )
            yield chunk

        total_tokens = len(accumulated_text) // 4

        yield ChatChunk(
            chunk="",
            is_final=True,
            msg_id=msg_id,
            total_tokens=total_tokens,
        )

        await memory.save_episodic(session_id, content, accumulated_text)

    except Exception as e:
        logger.error("chat.stream.error", error=str(e))
        yield ChatChunk(
            chunk=f"发生错误: {str(e)}",
            is_final=True,
            msg_id=msg_id,
            total_tokens=0,
        )


@router.post("", response_model=ChatRsp)
async def chat(req: ChatReq):
    """
    普通（非流式）AI 对话。
    """
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")

    uid = req.uid
    session_id = req.session_id or str(uuid.uuid4())

    try:
        memory = MemoryManager(session_id=session_id, uid=uid)
        history = await memory.load_context()
        messages = history + [LLMMessage(role="user", content=req.content)]

        agent = build_agent()
        response = ""
        async for delta in run_agent(agent, messages, model_type=req.model_type or "ollama",
                                     model_name=req.model_name):
            response += delta

        tokens = len(response) // 4

        await memory.save_episodic(session_id, req.content, response)

        return ChatRsp(
            code=0,
            message="ok",
            session_id=session_id,
            content=response,
            tokens=tokens,
            model=req.model_name or req.model_type or "ollama",
        )

    except Exception as e:
        logger.error("chat.error", error=str(e), uid=uid, session_id=session_id)
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/stream")
async def chat_stream(req: ChatReq):
    """
    流式 AI 对话（SSE）。
    返回 Server-Sent Events 格式，逐 token 流式输出。
    """
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")

    uid = req.uid
    session_id = req.session_id or str(uuid.uuid4())

    async def event_generator() -> AsyncIterator[bytes]:
        msg_id = str(uuid.uuid4())
        accumulated = ""

        try:
            memory = MemoryManager(session_id=session_id, uid=uid)
            history = await memory.load_context()

            from langchain_core.messages import HumanMessage
            messages = history + [HumanMessage(content=req.content)]

            async for delta in run_agent_stream(
                agent=None,  # stream 模式不使用完整 agent
                messages=messages,
                model_type=req.model_type or "ollama",
                model_name=req.model_name or "",
            ):
                accumulated += delta
                yield f"data: {json.dumps({
                    'chunk': delta,
                    'is_final': False,
                    'msg_id': msg_id,
                    'total_tokens': len(accumulated) // 4
                }, ensure_ascii=False)}\n\n".encode("utf-8")

            await memory.save_episodic(session_id, req.content, accumulated)

            yield f"data: {json.dumps({
                'chunk': '',
                'is_final': True,
                'msg_id': msg_id,
                'total_tokens': len(accumulated) // 4
            }, ensure_ascii=False)}\n\n".encode("utf-8")

        except Exception as e:
            logger.error("chat.stream.error", error=str(e))
            yield f"data: {json.dumps({
                'chunk': f'发生错误: {str(e)}',
                'is_final': True,
                'msg_id': msg_id,
                'total_tokens': 0
            }, ensure_ascii=False)}\n\n".encode("utf-8")

    return StreamingResponse(
        event_generator(),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "Connection": "keep-alive",
            "X-Accel-Buffering": "no",
        },
    )
