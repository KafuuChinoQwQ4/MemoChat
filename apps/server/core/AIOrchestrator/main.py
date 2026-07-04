"""
MemoChat AIOrchestrator — Python 微服务入口
FastAPI + Uvicorn，提供 /chat、/smart、/kb、/models 路由
"""

import asyncio
import hmac
import signal
import sys
from contextlib import asynccontextmanager

import structlog
import uvicorn
from api.agent_router import router as agent_router
from api.chat_router import router as chat_router
from api.kb_router import router as kb_router
from api.model_router import router as model_router
from api.pet_router import router as pet_router
from api.recommend_router import router as recommend_router
from api.smart_router import router as smart_router
from config import is_trusted_client_host, is_unauthenticated_path, resolve_internal_api_key, settings
from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse, PlainTextResponse
from harness import HarnessContainer
from observability.langsmith_instrument import init_langsmith
from observability.metrics import ai_metrics
from observability.tracer import init_tracing

logger = structlog.get_logger()


@asynccontextmanager
async def lifespan(app: FastAPI):
    """服务生命周期管理：启动初始化 + 关闭清理"""
    logger.info("service.starting", service="AIOrchestrator", port=settings.service.port)

    if settings.observability.enabled:
        init_tracing()
        init_langsmith()

    # MCP 桥接初始化
    from tools.registry import ToolRegistry

    registry = ToolRegistry.get_instance()
    await registry.initialize_mcp()
    await HarnessContainer.get_instance().startup()

    yield

    logger.info("service.stopping", service="AIOrchestrator")

    await HarnessContainer.get_instance().shutdown()

    # 关闭 MCP Server
    from tools.registry import ToolRegistry

    registry = ToolRegistry.get_instance()
    await registry.close()

    # 关闭 LLM 客户端
    from llm.claude_llm import ClaudeLLM
    from llm.kimi_llm import KimiLLM
    from llm.ollama_llm import OllamaLLM
    from llm.openai_llm import OpenAILLM

    for name, inst in [
        (n, i)
        for n, i in [
            ("ollama", OllamaLLM._instance),
            ("openai", OpenAILLM._instance),
            ("claude", ClaudeLLM._instance),
            ("kimi", KimiLLM._instance),
        ]
        if i is not None
    ]:
        try:
            if hasattr(inst, "close"):
                inst.close()
            logger.info("llm.shutdown", backend=name)
        except Exception:
            pass


def create_app() -> FastAPI:
    app = FastAPI(
        title="MemoChat AIOrchestrator",
        version="2.0.0",
        description="MemoChat AI Agent Harness：编排层 + 执行层 + 反馈层 + 记忆层",
        lifespan=lifespan,
    )

    app.add_middleware(
        CORSMiddleware,
        allow_origins=settings.security.cors_allow_origins,
        allow_origin_regex=settings.security.cors_allow_origin_regex or None,
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    @app.middleware("http")
    async def enforce_internal_auth(request: Request, call_next):
        security = settings.security
        if (
            request.method.upper() == "OPTIONS"
            or not security.enforce_internal_auth
            or is_unauthenticated_path(request.url.path, security.unauthenticated_paths)
        ):
            return await call_next(request)

        client_host = request.client.host if request.client is not None else ""
        if is_trusted_client_host(client_host, security.trusted_client_hosts):
            return await call_next(request)

        expected = resolve_internal_api_key(security)
        if not expected:
            return JSONResponse(
                status_code=503,
                content={"code": 503, "message": "AIOrchestrator internal auth is not configured"},
            )

        supplied = request.headers.get(security.internal_auth_header, "")
        if not hmac.compare_digest(supplied, expected):
            return JSONResponse(
                status_code=401,
                content={"code": 401, "message": "AIOrchestrator internal auth required"},
            )

        return await call_next(request)

    app.include_router(chat_router, prefix="/chat", tags=["chat"])
    app.include_router(smart_router, prefix="/smart", tags=["smart"])
    app.include_router(kb_router, prefix="/kb", tags=["knowledge-base"])
    app.include_router(model_router, prefix="/models", tags=["models"])
    app.include_router(recommend_router, prefix="/recommend", tags=["recommendation"])
    app.include_router(agent_router, prefix="/agent", tags=["agent-harness"])
    app.include_router(pet_router, prefix="/pet", tags=["desktop-pet"])

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

    @app.get("/metrics", response_class=PlainTextResponse)
    async def metrics():
        return PlainTextResponse(
            ai_metrics.render_text(),
            media_type="text/plain; version=0.0.4; charset=utf-8",
        )

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
