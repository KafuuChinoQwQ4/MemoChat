"""
Claude LLM 适配器 — 支持 Claude 3.5 Sonnet 等
"""
import httpx
import json
from typing import AsyncIterator

from .base import BaseLLM, LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage


class ClaudeLLM(BaseLLM):
    _instance: "ClaudeLLM | None" = None

    def __init__(self, api_key: str, base_url: str = "https://api.anthropic.com/v1", model_name: str = "claude-3-5-sonnet-20241022"):
        super().__init__(model_name)
        self.api_key = api_key
        self.base_url = base_url.rstrip("/")
        self._client: httpx.AsyncClient | None = None
        ClaudeLLM._instance = self

    async def _get_client(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(
                timeout=httpx.Timeout(120.0),
                headers={
                    "x-api-key": self.api_key,
                    "anthropic-version": "2023-06-01",
                    "content-type": "application/json",
                },
            )
        return self._client

    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        client = await self._get_client()

        system_msg = ""
        formatted_messages = []
        for msg in messages:
            if msg.role == "system":
                system_msg = msg.content
            else:
                formatted_messages.append({"role": msg.role, "content": msg.content})

        payload = {
            "model": self.model_name,
            "messages": formatted_messages,
            "max_tokens": kwargs.get("max_tokens", 2048),
            "temperature": kwargs.get("temperature", 0.7),
            "stream": False,
        }
        if system_msg:
            payload["system"] = system_msg

        try:
            resp = await client.post(f"{self.base_url}/messages", json=payload)
            resp.raise_for_status()
            data = resp.json()

            content_blocks = data.get("content", [])
            content = ""
            for block in content_blocks:
                if block.get("type") == "text":
                    content += block.get("text", "")

            usage_data = data.get("usage", {})

            return LLMResponse(
                content=content,
                usage=LLMUsage(
                    prompt_tokens=usage_data.get("input_tokens", 0),
                    completion_tokens=usage_data.get("output_tokens", 0),
                    total_tokens=usage_data.get("input_tokens", 0) + usage_data.get("output_tokens", 0),
                ),
                model=self.model_name,
                finish_reason=data.get("stop_reason", ""),
            )
        except httpx.HTTPStatusError as e:
            raise RuntimeError(f"Claude HTTP error: {e.response.status_code}")
        except Exception as e:
            raise RuntimeError(f"Claude request failed: {e}")

    async def chat_stream(self, messages: list[LLMMessage], **kwargs) -> AsyncIterator[LLMStreamChunk]:
        client = await self._get_client()

        system_msg = ""
        formatted_messages = []
        for msg in messages:
            if msg.role == "system":
                system_msg = msg.content
            else:
                formatted_messages.append({"role": msg.role, "content": msg.content})

        payload = {
            "model": self.model_name,
            "messages": formatted_messages,
            "max_tokens": kwargs.get("max_tokens", 2048),
            "temperature": kwargs.get("temperature", 0.7),
            "stream": True,
        }
        if system_msg:
            payload["system"] = system_msg

        async with client.stream("POST", f"{self.base_url}/messages", json=payload) as resp:
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

                try:
                    data = json.loads(data_str)
                except json.JSONDecodeError:
                    continue

                event_type = data.get("type", "")
                if event_type == "content_block_delta":
                    delta = data.get("delta", {}).get("text", "")
                    accumulated += delta
                    yield LLMStreamChunk(content=delta, is_final=False, model=self.model_name)
                elif event_type == "message_stop":
                    usage_data = data.get("message", {}).get("usage", {})
                    if usage_data:
                        total_usage = LLMUsage(
                            prompt_tokens=usage_data.get("input_tokens", 0),
                            completion_tokens=usage_data.get("output_tokens", 0),
                            total_tokens=usage_data.get("input_tokens", 0) + usage_data.get("output_tokens", 0),
                        )
                    yield LLMStreamChunk(content="", is_final=True, usage=total_usage, model=self.model_name)
                    break

    async def list_models(self) -> list[str]:
        return [self.model_name]

    def close(self):
        if self._client and not self._client.is_closed:
            import asyncio
            asyncio.create_task(self._client.aclose())
