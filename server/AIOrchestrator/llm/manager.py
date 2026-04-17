"""
LLM 管理器 — 统一调度所有 LLM 后端，支持自动降级
"""
import asyncio
from typing import Optional

import structlog
from .base import BaseLLM, LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage
from .ollama_llm import OllamaLLM
from .openai_llm import OpenAILLM
from .claude_llm import ClaudeLLM
from .kimi_llm import KimiLLM
from ..config import settings

logger = structlog.get_logger()


class RateLimitError(Exception):
    pass


class AllBackendsFailedError(Exception):
    pass


class LLMManager:
    """
    LLM 统一管理器。
    1. 管理多后端生命周期
    2. 提供统一 chat 接口
    3. 自动降级（FallbackChain）
    """
    _instance: "LLMManager | None" = None

    def __init__(self):
        self._backends: dict[str, BaseLLM] = {}
        self._fallback_config = settings.llm_fallback
        self._initialized = False

    @classmethod
    def get_instance(cls) -> "LLMManager":
        if cls._instance is None:
            cls._instance = LLMManager()
            cls._instance._initialize()
        return cls._instance

    def _initialize(self):
        if self._initialized:
            return

        llm_cfg = settings.llm

        if llm_cfg.ollama.enabled:
            try:
                self._backends["ollama"] = OllamaLLM(
                    base_url=llm_cfg.ollama.base_url,
                    model_name=llm_cfg.default_model if llm_cfg.default_backend == "ollama" else llm_cfg.ollama.models[0].name if llm_cfg.ollama.models else "qwen2.5:7b",
                )
                logger.info("llm.backend_loaded", backend="ollama", url=llm_cfg.ollama.base_url)
            except Exception as e:
                logger.warning("llm.backend_failed", backend="ollama", error=str(e))

        if llm_cfg.openai.enabled and llm_cfg.openai.api_key:
            try:
                self._backends["openai"] = OpenAILLM(
                    api_key=llm_cfg.openai.api_key,
                    base_url=llm_cfg.openai.base_url,
                    model_name=llm_cfg.default_model if llm_cfg.default_backend == "openai" else llm_cfg.openai.models[0].name if llm_cfg.openai.models else "gpt-4o",
                )
                logger.info("llm.backend_loaded", backend="openai")
            except Exception as e:
                logger.warning("llm.backend_failed", backend="openai", error=str(e))

        if llm_cfg.anthropic.enabled and llm_cfg.anthropic.api_key:
            try:
                self._backends["claude"] = ClaudeLLM(
                    api_key=llm_cfg.anthropic.api_key,
                    base_url=llm_cfg.anthropic.base_url,
                    model_name=llm_cfg.default_model if llm_cfg.default_backend == "claude" else llm_cfg.anthropic.models[0].name if llm_cfg.anthropic.models else "claude-3-5-sonnet-20241022",
                )
                logger.info("llm.backend_loaded", backend="claude")
            except Exception as e:
                logger.warning("llm.backend_failed", backend="claude", error=str(e))

        if llm_cfg.kimi.enabled and llm_cfg.kimi.api_key:
            try:
                self._backends["kimi"] = KimiLLM(
                    api_key=llm_cfg.kimi.api_key,
                    base_url=llm_cfg.kimi.base_url,
                    model_name=llm_cfg.default_model if llm_cfg.default_backend == "kimi" else llm_cfg.kimi.models[0].name if llm_cfg.kimi.models else "moonshot-v1-8k",
                )
                logger.info("llm.backend_loaded", backend="kimi")
            except Exception as e:
                logger.warning("llm.backend_failed", backend="kimi", error=str(e))

        self._initialized = True
        logger.info("llm.manager_ready", backends=list(self._backends.keys()))

    def get_backend(self, backend_type: str) -> Optional[BaseLLM]:
        return self._backends.get(backend_type)

    def resolve_backend(self, model_type: str) -> tuple[str, BaseLLM]:
        """根据 model_type 解析 backend"""
        if model_type in self._backends:
            return model_type, self._backends[model_type]

        default_type = settings.llm.default_backend
        if default_type in self._backends:
            return default_type, self._backends[default_type]

        for t, inst in self._backends.items():
            return t, inst

        raise AllBackendsFailedError("No LLM backend available")

    async def achat(self, messages: list[LLMMessage], prefer_backend: str = "",
                    model_name: str = "", **kwargs) -> LLMResponse:
        """
        带自动降级的异步 LLM 调用。
        """
        tried = []

        if prefer_backend and prefer_backend in self._backends:
            candidates = [prefer_backend] + self._fallback_config.model_dump().get(prefer_backend, [])
        else:
            default = settings.llm.default_backend
            candidates = [default] + self._fallback_config.model_dump().get(default, [])

        for backend_type in candidates:
            if backend_type not in self._backends:
                continue
            tried.append(backend_type)

            for attempt in range(self._fallback_config.retry_count + 1):
                try:
                    llm = self._backends[backend_type]
                    if model_name:
                        llm.model_name = model_name

                    result = await llm.chat(messages, **kwargs)
                    if len(tried) > 1:
                        logger.warning("llm.fallbacked", from_backend=tried[0], to_backend=backend_type)
                    return result

                except asyncio.TimeoutError:
                    logger.warning("llm.timeout", backend=backend_type, attempt=attempt)
                    if attempt < self._fallback_config.retry_count:
                        await asyncio.sleep(self._fallback_config.retry_delay_sec * (2 ** attempt))
                    continue
                except RateLimitError:
                    logger.warning("llm.rate_limited", backend=backend_type, attempt=attempt)
                    if attempt < self._fallback_config.retry_count:
                        await asyncio.sleep(self._fallback_config.retry_delay_sec * (2 ** attempt))
                    continue
                except Exception as e:
                    logger.warning("llm.backend_error", backend=backend_type, error=str(e), attempt=attempt)
                    break

        raise AllBackendsFailedError(f"All LLM backends failed. Tried: {tried}")

    async def astream(self, messages: list[LLMMessage], prefer_backend: str = "",
                      model_name: str = "", **kwargs) -> AsyncIterator[LLMStreamChunk]:
        """
        带自动降级的异步流式 LLM 调用。
        当主后端失败时自动切换到 fallback 后端。
        """
        tried = []

        if prefer_backend and prefer_backend in self._backends:
            candidates = [prefer_backend] + list(self._fallback_config.model_dump().get(prefer_backend, []))
        else:
            default = settings.llm.default_backend
            candidates = [default] + list(self._fallback_config.model_dump().get(default, []))

        for backend_type in candidates:
            if backend_type not in self._backends:
                continue
            tried.append(backend_type)

            try:
                llm = self._backends[backend_type]
                if model_name:
                    llm.model_name = model_name

                async for chunk in llm.chat_stream(messages, **kwargs):
                    yield chunk
                return

            except asyncio.TimeoutError:
                logger.warning("llm.astream_timeout", backend=backend_type)
                if len(tried) == len(candidates):
                    break
                continue
            except Exception as e:
                logger.warning("llm.astream_error", backend=backend_type, error=str(e))
                if len(tried) == len(candidates):
                    break
                continue

        raise AllBackendsFailedError(f"All LLM backends failed for streaming. Tried: {tried}")
