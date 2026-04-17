import asyncio
import logging
from typing import Optional, Any, List, Dict

from langchain_ollama import ChatOllama
from langchain_openai import ChatOpenAI
from langchain_anthropic import ChatAnthropic

from config import settings

logger = logging.getLogger(__name__)


class LLMAdapter:
    """统一 LLM 适配器，支持 Ollama / OpenAI / Anthropic / Kimi 自动切换"""

    def __init__(self):
        self._clients: Dict[str, Any] = {}
        self._default_type = settings.default_backend
        self._default_model = settings.default_model

    def _get_client(self, model_type: str, model_name: str):
        key = f"{model_type}:{model_name}"
        if key in self._clients:
            return self._clients[key]

        if model_type == "ollama":
            client = ChatOllama(
                base_url=settings.ollama_base_url,
                model=model_name,
                temperature=0.7,
            )
        elif model_type == "openai":
            client = ChatOpenAI(
                api_key=settings.openai_api_key or None,
                base_url=settings.openai_base_url,
                model=model_name,
                temperature=0.7,
            )
        elif model_type == "anthropic":
            client = ChatAnthropic(
                api_key=settings.anthropic_api_key or None,
                base_url=settings.anthropic_base_url,
                model=model_name,
                temperature=0.7,
            )
        elif model_type == "kimi":
            client = ChatOpenAI(
                api_key=settings.kimi_api_key or None,
                base_url=settings.kimi_base_url,
                model=model_name,
                temperature=0.7,
            )
        else:
            raise ValueError(f"Unknown model_type: {model_type}")

        self._clients[key] = client
        return client

    async def invoke(self,
                     prompt: str,
                     model_type: Optional[str] = None,
                     model_name: Optional[str] = None,
                     messages: Optional[List[Dict]] = None,
                     **kwargs) -> str:
        model_type = model_type or self._default_type
        model_name = model_name or self._default_model

        client = self._get_client(model_type, model_name)

        if messages:
            from langchain_core.messages import HumanMessage, AIMessage
            lc_messages = []
            for m in messages:
                if m.get("role") == "user":
                    lc_messages.append(HumanMessage(content=m["content"]))
                else:
                    lc_messages.append(AIMessage(content=m["content"]))

            if prompt:
                lc_messages.append(HumanMessage(content=prompt))

            try:
                response = await client.ainvoke(lc_messages)
                return response.content
            except Exception as e:
                if settings.fallback_on_error:
                    return await self._fallback(prompt, messages, str(e))
                raise
        else:
            try:
                response = await client.ainvoke(prompt)
                return response.content
            except Exception as e:
                if settings.fallback_on_error:
                    return await self._fallback(prompt, None, str(e))
                raise

    async def _fallback(self, prompt: str, messages: Optional[List[Dict]], error: str) -> str:
        """Fallback chain: 尝试备用模型"""
        fallbacks = [
            ("openai", "gpt-4o-mini"),
            ("kimi", "moonshot-v1-8k"),
        ]
        logger.warning(f"LLM error, trying fallback: {error}")

        for ft, fm in fallbacks:
            if ft == self._default_type:
                continue
            try:
                client = self._get_client(ft, fm)
                if messages:
                    from langchain_core.messages import HumanMessage, AIMessage
                    lc_messages = [
                        (AIMessage if m["role"] == "assistant" else HumanMessage)(content=m["content"])
                        for m in messages
                    ]
                    lc_messages.append(HumanMessage(content=prompt))
                    response = await client.ainvoke(lc_messages)
                else:
                    response = await client.ainvoke(prompt)
                logger.info(f"Fallback succeeded: {ft}/{fm}")
                return response.content
            except Exception as e2:
                logger.warning(f"Fallback {ft}/{fm} failed: {e2}")
                continue

        return f"抱歉，当前 AI 服务暂时不可用。请稍后再试。"
