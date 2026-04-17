"""
Ollama LLM 适配器 — 支持本地大模型
"""
import httpx
import json
from typing import AsyncIterator

from .base import BaseLLM, LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage


class OllamaLLM(BaseLLM):
    _instance: "OllamaLLM | None" = None

    def __init__(self, base_url: str = "http://127.0.0.1:11434", model_name: str = "qwen2.5:7b"):
        super().__init__(model_name)
        self.base_url = base_url.rstrip("/")
        self._client: httpx.AsyncClient | None = None
        OllamaLLM._instance = self

    async def _get_client(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=httpx.Timeout(120.0))
        return self._client

    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        client = await self._get_client()
        payload = {
            "model": self.model_name,
            "messages": self._messages_to_dict(messages),
            "stream": False,
            "options": {
                "temperature": kwargs.get("temperature", 0.7),
                "num_predict": kwargs.get("max_tokens", 2048),
            },
        }

        try:
            resp = await client.post(f"{self.base_url}/api/chat", json=payload)
            resp.raise_for_status()
            data = resp.json()

            content = data.get("message", {}).get("content", "")
            usage_data = data.get("eval_count", 0)
            prompt_eval_count = data.get("prompt_eval_count", 0)

            return LLMResponse(
                content=content,
                usage=LLMUsage(
                    prompt_tokens=prompt_eval_count,
                    completion_tokens=usage_data,
                    total_tokens=prompt_eval_count + usage_data,
                ),
                model=self.model_name,
                finish_reason=data.get("done_reason", ""),
            )
        except httpx.HTTPStatusError as e:
            raise RuntimeError(f"Ollama HTTP error: {e.response.status_code} — {e.response.text}")
        except Exception as e:
            raise RuntimeError(f"Ollama request failed: {e}")

    async def chat_stream(self, messages: list[LLMMessage], **kwargs) -> AsyncIterator[LLMStreamChunk]:
        client = await self._get_client()
        payload = {
            "model": self.model_name,
            "messages": self._messages_to_dict(messages),
            "stream": True,
            "options": {
                "temperature": kwargs.get("temperature", 0.7),
                "num_predict": kwargs.get("max_tokens", 2048),
            },
        }

        async with client.stream("POST", f"{self.base_url}/api/chat", json=payload) as resp:
            resp.raise_for_status()
            accumulated = ""
            total_tokens = 0

            async for line in resp.aiter_lines():
                if not line.strip():
                    continue
                try:
                    data = json.loads(line)
                except json.JSONDecodeError:
                    continue

                delta = data.get("message", {}).get("content", "")
                accumulated += delta

                eval_count = data.get("eval_count", 0)
                prompt_eval_count = data.get("prompt_eval_count", 0)
                total_tokens = eval_count + prompt_eval_count
                done = data.get("done", False)

                yield LLMStreamChunk(
                    content=delta,
                    is_final=done,
                    usage=LLMUsage(
                        prompt_tokens=prompt_eval_count,
                        completion_tokens=eval_count,
                        total_tokens=total_tokens,
                    ) if done else None,
                    model=self.model_name,
                )

                if done:
                    break

    async def list_models(self) -> list[str]:
        client = await self._get_client()
        try:
            resp = await client.get(f"{self.base_url}/api/tags")
            resp.raise_for_status()
            data = resp.json()
            return [m["name"] for m in data.get("models", [])]
        except Exception:
            return [self.model_name]

    def close(self):
        if self._client and not self._client.is_closed:
            import asyncio
            asyncio.create_task(self._client.aclose())
