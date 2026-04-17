"""
MemoChat AIOrchestrator — Python 微服务入口
FastAPI + Uvicorn，提供 /chat、/smart、/kb、/models 路由
"""
import asyncio
import signal
import sys
from contextlib import asynccontextmanager

import uvicorn
import structlog
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from config import settings
from api.chat_router import router as chat_router
from api.smart_router import router as smart_router
from api.kb_router import router as kb_router
from api.model_router import router as model_router
from api.recommend_router import router as recommend_router
from observability.tracer import init_tracing
from observability.langfuse_instrument import init_langfuse

logger = structlog.get_logger()


@asynccontextmanager
async def lifespan(app: FastAPI):
    """服务生命周期管理：启动初始化 + 关闭清理"""
    logger.info("service.starting", service="AIOrchestrator", port=settings.service.port)

    if settings.observability.enabled:
        init_tracing()
        init_langfuse()

    # MCP 桥接初始化
    from tools.registry import ToolRegistry
    registry = ToolRegistry.get_instance()
    await registry.initialize_mcp()

    yield

    logger.info("service.stopping", service="AIOrchestrator")

    # 关闭 MCP Server
    from tools.registry import ToolRegistry
    registry = ToolRegistry.get_instance()
    await registry.close()

    # 关闭 LLM 客户端
    from llm.ollama_llm import OllamaLLM
    from llm.openai_llm import OpenAILLM
    from llm.claude_llm import ClaudeLLM
    from llm.kimi_llm import KimiLLM
    for name, inst in [(n, i) for n, i in [
        ("ollama", OllamaLLM._instance),
        ("openai", OpenAILLM._instance),
        ("claude", ClaudeLLM._instance),
        ("kimi", KimiLLM._instance),
    ] if i is not None]:
        try:
            if hasattr(inst, "close"):
                inst.close()
            logger.info("llm.shutdown", backend=name)
        except Exception:
            pass


def create_app() -> FastAPI:
    app = FastAPI(
        title="MemoChat AIOrchestrator",
        version="1.0.0",
        description="MemoChat AI Agent 编排层：LangGraph + LangChain + Qdrant RAG",
        lifespan=lifespan,
    )

    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    app.include_router(chat_router, prefix="/chat", tags=["chat"])
    app.include_router(smart_router, prefix="/smart", tags=["smart"])
    app.include_router(kb_router, prefix="/kb", tags=["knowledge-base"])
    app.include_router(model_router, prefix="/models", tags=["models"])
    app.include_router(recommend_router, prefix="/recommend", tags=["recommendation"])

    @app.get("/health")
    async def health():
        return {"status": "ok", "service": "ai-orchestrator"}

    @app.get("/ready")
    async def ready():
        from llm.manager import LLMManager
        manager = LLMManager.get_instance()
        available = []
        for backend in ["ollama", "openai", "claude", "kimi"]:
            inst = manager.get_backend(backend)
            if inst is not None:
                try:
                    models = await inst.list_models()
                    available.append({"backend": backend, "models": models, "ok": True})
                except Exception as e:
                    available.append({"backend": backend, "ok": False, "error": str(e)})
        return {"status": "ready", "backends": available}

    return app


app = create_app()


def main():
    uvicorn.run(
        "main:app",
        host=settings.service.host,
        port=settings.service.port,
        reload=False,
        log_level="info",
        access_log=True,
    )


if __name__ == "__main__":
    main()
