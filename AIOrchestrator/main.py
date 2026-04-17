import os
import json
import asyncio
from contextlib import asynccontextmanager

from fastapi import FastAPI, Request
from fastapi.responses import StreamingResponse, JSONResponse
from pydantic import BaseModel
from sse_starlette.sse import EventSourceResponse

from config import settings
from llm.adapter import LLMAdapter
from langgraph.agent import build_agent, AgentState
from tools import search_tool, calculator_tool, knowledge_base_tool
from tools.hitl_manager import HITLManager


llm_adapter = LLMAdapter()
hitl_manager = HITLManager()
agent_executor = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    global agent_executor
    agent_graph = build_agent(
        llm=llm_adapter,
        tools=[search_tool, calculator_tool, knowledge_base_tool],
        hitl_manager=hitl_manager,
    )
    agent_executor = agent_graph.compile()
    yield


app = FastAPI(title="AIOrchestrator", version="0.1.0", lifespan=lifespan)


class ChatBody(BaseModel):
    uid: int
    session_id: str
    content: str
    model_type: str = "ollama"
    model_name: str = "qwen2.5:7b"
    stream: bool = False


class SmartBody(BaseModel):
    feature_type: str
    content: str
    target_lang: str = "zh"
    context_json: str = "{}"


class KbUploadBody(BaseModel):
    uid: int
    file_name: str
    file_type: str
    content: str  # base64


class KbSearchBody(BaseModel):
    uid: int
    query: str
    top_k: int = 5


@app.post("/chat")
async def chat(body: ChatBody):
    if body.stream:
        return StreamingResponse(
            chat_stream(body.uid, body.session_id, body.content,
                        body.model_type, body.model_name),
            media_type="text/event-stream"
        )

    result = await chat_sync(body.uid, body.session_id, body.content,
                              body.model_type, body.model_name)
    return JSONResponse(content=result)


async def chat_sync(uid: int, session_id: str, content: str,
                    model_type: str, model_name: str) -> dict:
    initial_state: AgentState = {
        "uid": uid,
        "session_id": session_id,
        "messages": [{"role": "user", "content": content}],
        "tools_used": [],
        "tokens_used": 0,
        "pending_confirm": None,
        "iteration_count": 0,
    }

    result = await agent_executor.ainvoke(initial_state)
    final_msg = result["messages"][-1]["content"]
    return {
        "content": final_msg,
        "tokens": result.get("tokens_used", 0),
        "model": model_name,
    }


async def chat_stream(uid: int, session_id: str, content: str,
                       model_type: str, model_name: str):
    initial_state: AgentState = {
        "uid": uid,
        "session_id": session_id,
        "messages": [{"role": "user", "content": content}],
        "tools_used": [],
        "tokens_used": 0,
        "pending_confirm": None,
        "iteration_count": 0,
    }

    msg_id = f"msg_{uid}_{session_id}"
    total_tokens = 0

    async for event in agent_executor.astream_events(initial_state, version="v2"):
        kind = event["event"]
        if kind == "on_chat_model_stream":
            chunk = event["data"]["chunk"].content
            if chunk:
                total_tokens += 1
                yield {
                    "event": "message",
                    "data": json.dumps({
                        "chunk": chunk,
                        "is_final": False,
                        "msg_id": msg_id,
                        "total_tokens": total_tokens,
                    })
                }

    yield {
        "event": "message",
        "data": json.dumps({
            "chunk": "",
            "is_final": True,
            "msg_id": msg_id,
            "total_tokens": total_tokens,
        })
    }


@app.post("/smart")
async def smart(body: SmartBody):
    prompt_map = {
        "summary": f"请用简洁的语言总结以下内容：\n{body.content}",
        "suggest": f"根据以下对话上下文，提供3个智能回复建议：\n{body.content}",
        "translate": f"请将以下内容翻译成{body.target_lang}：\n{body.content}",
    }
    prompt = prompt_map.get(body.feature_type, body.content)
    if not prompt:
        return JSONResponse(content={"code": 400, "content": "unknown feature_type"})

    result = await llm_adapter.invoke(prompt, model_type="ollama", model_name="qwen2.5:7b")
    return JSONResponse(content={"content": result})


@app.post("/kb/upload")
async def kb_upload(body: KbUploadBody):
    from rag.knowledge_base import KnowledgeBaseManager
    kb = KnowledgeBaseManager()
    kb_id = await kb.upload_document(body.uid, body.file_name, body.file_type, body.content)
    return JSONResponse(content={"chunks": 10, "kb_id": kb_id})


@app.post("/kb/search")
async def kb_search(body: KbSearchBody):
    from rag.knowledge_base import KnowledgeBaseManager
    kb = KnowledgeBaseManager()
    chunks = await kb.search(body.uid, body.query, body.top_k)
    return JSONResponse(content={"chunks": chunks})


@app.get("/health")
async def health():
    return JSONResponse(content={"status": "ok"})


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8096)
