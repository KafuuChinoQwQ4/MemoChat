"""
OpenAI LLM 适配器 — 支持 GPT-4o / GPT-4o Mini 等
"""
import httpx
from typing import AsyncIterator

from .base import BaseLLM, LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage


def _wrap_thinking(reasoning: str, content: str) -> str:
    reasoning = (reasoning or "").strip()
    if not reasoning:
        return content or ""
    return f"<think>{reasoning}</think>{content or ''}"


class OpenAILLM(BaseLLM):
    _instance: "OpenAILLM | None" = None

    def __init__(self, api_key: str, base_url: str = "https://api.openai.com/v1", model_name: str = "gpt-4o"):
        super().__init__(model_name)
        self.api_key = api_key
        self.base_url = base_url.rstrip("/")
        self._client: httpx.AsyncClient | None = None
        OpenAILLM._instance = self

    async def _get_client(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(
                timeout=httpx.Timeout(120.0),
                headers={"Authorization": f"Bearer {self.api_key}"},
            )
        return self._client

    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        client = await self._get_client()
        payload = {
            "model": self.model_name,
            "messages": self._messages_to_dict(messages),
            "stream": False,
            "temperature": kwargs.get("temperature", 0.7),
            "max_tokens": kwargs.get("max_tokens", 2048),
        }

        try:
            resp = await client.post(f"{self.base_url}/chat/completions", json=payload)
            resp.raise_for_status()
            data = resp.json()

            choice = data["choices"][0]
            message = choice.get("message", {})
            content = message.get("content", "")
            reasoning = message.get("reasoning_content") or message.get("reasoning") or ""
            usage_data = data.get("usage", {})

            return LLMResponse(
                content=_wrap_thinking(reasoning, content) if kwargs.get("think", False) else content,
                reasoning_content=reasoning if kwargs.get("think", False) else "",
                usage=LLMUsage(
                    prompt_tokens=usage_data.get("prompt_tokens", 0),
                    completion_tokens=usage_data.get("completion_tokens", 0),
                    total_tokens=usage_data.get("total_tokens", 0),
                ),
                model=self.model_name,
                finish_reason=choice.get("finish_reason", ""),
            )
        except httpx.HTTPStatusError as e:
            raise RuntimeError(f"OpenAI HTTP error: {e.response.status_code}")
        except Exception as e:
            raise RuntimeError(f"OpenAI request failed: {e}")

    async def chat_stream(self, messages: list[LLMMessage], **kwargs) -> AsyncIterator[LLMStreamChunk]:
        client = await self._get_client()
        payload = {
            "model": self.model_name,
            "messages": self._messages_to_dict(messages),
            "stream": True,
            "temperature": kwargs.get("temperature", 0.7),
            "max_tokens": kwargs.get("max_tokens", 2048),
        }

        async with client.stream("POST", f"{self.base_url}/chat/completions", json=payload) as resp:
            resp.raise_for_status()
            accumulated = ""
            total_usage = LLMUsage()

            async for line in resp.aiter_lines():
                if not line.startswith("data: "):
                    continue
                data_str = line[6:].strip()
                if data_str == "[DONE]":
                    yield LLMStreamChunk(content="", is_final=True, usage=total_usage, model=self.model_name)
                    break

                import json
                try:
                    data = json.loads(data_str)
                except json.JSONDecodeError:
                    continue

                delta_obj = data.get("choices", [{}])[0].get("delta", {})
                reasoning = delta_obj.get("reasoning_content") or delta_obj.get("reasoning") or ""
                delta = delta_obj.get("content", "")
                accumulated += delta

                usage_data = data.get("usage", {})
                if usage_data:
                    total_usage = LLMUsage(
                        prompt_tokens=usage_data.get("prompt_tokens", 0),
                        completion_tokens=usage_data.get("completion_tokens", 0),
                        total_tokens=usage_data.get("total_tokens", 0),
                    )

                if reasoning and kwargs.get("think", False):
                    yield LLMStreamChunk(content=_wrap_thinking(reasoning, ""), reasoning_content=reasoning, is_final=False, model=self.model_name)
                if delta:
                    yield LLMStreamChunk(content=delta, is_final=False, model=self.model_name)

    async def list_models(self) -> list[str]:
        client = await self._get_client()
        try:
            resp = await client.get(f"{self.base_url}/models")
            resp.raise_for_status()
            data = resp.json()
            return [m["id"] for m in data.get("data", []) if m["id"].startswith("gpt")]
        except Exception:
            return [self.model_name]

    def close(self):
        if self._client and not self._client.is_closed:
            import asyncio
            asyncio.create_task(self._client.aclose())
