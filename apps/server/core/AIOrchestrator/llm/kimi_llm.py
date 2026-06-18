"""
Kimi (Moonshot) LLM 适配器 — 支持 moonshot-v1 系列
"""

from typing import AsyncIterator

import httpx

from .base import BaseLLM, LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage


def _moonshot_chat_payload(model_name: str, messages: list[LLMMessage], stream: bool, **kwargs) -> dict:
    return {
        "model": model_name,
        "messages": [{"role": message.role, "content": message.content} for message in messages],
        "stream": stream,
        "max_completion_tokens": kwargs.get("max_tokens", 2048),
    }


class KimiLLM(BaseLLM):
    _instance: "KimiLLM | None" = None

    def __init__(self, api_key: str, base_url: str = "https://api.moonshot.cn/v1", model_name: str = "moonshot-v1-8k"):
        super().__init__(model_name)
        self.api_key = api_key
        self.base_url = base_url.rstrip("/")
        self._client: httpx.AsyncClient | None = None
        KimiLLM._instance = self

    async def _get_client(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(
                timeout=httpx.Timeout(120.0),
                headers={"Authorization": f"Bearer {self.api_key}"},
            )
        return self._client

    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        client = await self._get_client()
        payload = _moonshot_chat_payload(self.model_name, messages, False, **kwargs)

        try:
            resp = await client.post(f"{self.base_url}/chat/completions", json=payload)
            resp.raise_for_status()
            data = resp.json()

            choice = data["choices"][0]
            content = choice.get("message", {}).get("content", "")
            usage_data = data.get("usage", {})

            return LLMResponse(
                content=content,
                usage=LLMUsage(
                    prompt_tokens=usage_data.get("prompt_tokens", 0),
                    completion_tokens=usage_data.get("completion_tokens", 0),
                    total_tokens=usage_data.get("total_tokens", 0),
                ),
                model=self.model_name,
                finish_reason=choice.get("finish_reason", ""),
            )
        except httpx.HTTPStatusError as e:
            error_body = e.response.text[:500] if e.response is not None else ""
            raise RuntimeError(f"Kimi HTTP error: {e.response.status_code} {error_body}".strip())
        except Exception as e:
            raise RuntimeError(f"Kimi request failed: {e}")

    async def chat_stream(self, messages: list[LLMMessage], **kwargs) -> AsyncIterator[LLMStreamChunk]:
        client = await self._get_client()
        payload = _moonshot_chat_payload(self.model_name, messages, True, **kwargs)

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

                delta = data.get("choices", [{}])[0].get("delta", {}).get("content", "")
                accumulated += delta

                usage_data = data.get("usage", {})
                if usage_data:
                    total_usage = LLMUsage(
                        prompt_tokens=usage_data.get("prompt_tokens", 0),
                        completion_tokens=usage_data.get("completion_tokens", 0),
                        total_tokens=usage_data.get("total_tokens", 0),
                    )

                yield LLMStreamChunk(content=delta, is_final=False, model=self.model_name)

    async def list_models(self) -> list[str]:
        return [self.model_name]

    def close(self):
        if self._client and not self._client.is_closed:
            import asyncio

            asyncio.create_task(self._client.aclose())
