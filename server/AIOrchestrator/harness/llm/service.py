from __future__ import annotations

import asyncio
import json
import os
import re
from typing import AsyncIterator

import httpx

from config import settings
from harness.contracts import ProviderEndpoint
from llm import LLMManager
from llm.base import LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage


class _OpenAICompatibleClient:
    def __init__(self, base_url: str, api_key: str, model_name: str, timeout_sec: int = 120):
        self.base_url = base_url.rstrip("/")
        self.api_key = api_key
        self.model_name = model_name
        self.timeout_sec = timeout_sec
        self._client: httpx.AsyncClient | None = None

    async def _get_client(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            headers = {"Authorization": f"Bearer {self.api_key}"} if self.api_key else {}
            self._client = httpx.AsyncClient(timeout=httpx.Timeout(float(self.timeout_sec)), headers=headers)
        return self._client

    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        client = await self._get_client()
        payload = {
            "model": kwargs.get("model_name") or self.model_name,
            "messages": [{"role": message.role, "content": message.content} for message in messages],
            "stream": False,
            "temperature": kwargs.get("temperature", 0.7),
            "max_tokens": kwargs.get("max_tokens", 2048),
        }
        response = await client.post(f"{self.base_url}/chat/completions", json=payload)
        response.raise_for_status()
        data = response.json()
        choice = data.get("choices", [{}])[0]
        usage = data.get("usage", {})
        return LLMResponse(
            content=choice.get("message", {}).get("content", ""),
            usage=LLMUsage(
                prompt_tokens=usage.get("prompt_tokens", 0),
                completion_tokens=usage.get("completion_tokens", 0),
                total_tokens=usage.get("total_tokens", 0),
            ),
            model=payload["model"],
            finish_reason=choice.get("finish_reason", ""),
        )

    async def chat_stream(self, messages: list[LLMMessage], **kwargs) -> AsyncIterator[LLMStreamChunk]:
        client = await self._get_client()
        payload = {
            "model": kwargs.get("model_name") or self.model_name,
            "messages": [{"role": message.role, "content": message.content} for message in messages],
            "stream": True,
            "temperature": kwargs.get("temperature", 0.7),
            "max_tokens": kwargs.get("max_tokens", 2048),
        }
        async with client.stream("POST", f"{self.base_url}/chat/completions", json=payload) as response:
            response.raise_for_status()
            async for line in response.aiter_lines():
                if not line.startswith("data: "):
                    continue
                raw = line[6:].strip()
                if raw == "[DONE]":
                    yield LLMStreamChunk(content="", is_final=True, model=payload["model"])
                    return
                try:
                    data = json.loads(raw)
                except Exception:
                    continue
                delta = data.get("choices", [{}])[0].get("delta", {}).get("content", "")
                yield LLMStreamChunk(content=delta, is_final=False, model=payload["model"])

    async def close(self) -> None:
        if self._client and not self._client.is_closed:
            await self._client.aclose()


class _OllamaCompatibleClient:
    def __init__(self, base_url: str, model_name: str, timeout_sec: int = 120):
        self.base_url = base_url.rstrip("/")
        self.model_name = model_name
        self.timeout_sec = timeout_sec
        self._client: httpx.AsyncClient | None = None

    @staticmethod
    def _strip_think_blocks(content: str) -> str:
        if not content:
            return ""
        if "</think>" in content:
            content = content.rsplit("</think>", 1)[-1]
        content = re.sub(r"<think>.*?</think>\s*", "", content, flags=re.DOTALL)
        return content.strip()

    async def _get_client(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=httpx.Timeout(float(self.timeout_sec)))
        return self._client

    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        client = await self._get_client()
        payload = {
            "model": kwargs.get("model_name") or self.model_name,
            "messages": [{"role": message.role, "content": message.content} for message in messages],
            "stream": False,
            "think": False,
            "options": {
                "temperature": kwargs.get("temperature", 0.7),
                "num_predict": kwargs.get("max_tokens", 2048),
            },
        }
        response = await client.post(f"{self.base_url}/api/chat", json=payload)
        response.raise_for_status()
        data = response.json()
        return LLMResponse(
            content=self._strip_think_blocks(data.get("message", {}).get("content", "")),
            usage=LLMUsage(
                prompt_tokens=data.get("prompt_eval_count", 0),
                completion_tokens=data.get("eval_count", 0),
                total_tokens=data.get("prompt_eval_count", 0) + data.get("eval_count", 0),
            ),
            model=payload["model"],
            finish_reason=data.get("done_reason", ""),
        )

    async def chat_stream(self, messages: list[LLMMessage], **kwargs) -> AsyncIterator[LLMStreamChunk]:
        client = await self._get_client()
        payload = {
            "model": kwargs.get("model_name") or self.model_name,
            "messages": [{"role": message.role, "content": message.content} for message in messages],
            "stream": True,
            "think": False,
            "options": {
                "temperature": kwargs.get("temperature", 0.7),
                "num_predict": kwargs.get("max_tokens", 2048),
            },
        }
        async with client.stream("POST", f"{self.base_url}/api/chat", json=payload) as response:
            response.raise_for_status()
            in_think_block = False
            async for line in response.aiter_lines():
                if not line.strip():
                    continue
                try:
                    data = json.loads(line)
                except Exception:
                    continue
                done = data.get("done", False)
                delta = data.get("message", {}).get("content", "")
                if "<think>" in delta:
                    in_think_block = True
                    delta = delta.split("<think>", 1)[0]
                if in_think_block and "</think>" in delta:
                    delta = delta.split("</think>", 1)[1]
                    in_think_block = False
                elif in_think_block:
                    delta = ""
                delta = self._strip_think_blocks(delta)
                yield LLMStreamChunk(content=delta, is_final=done, model=payload["model"])
                if done:
                    return

    async def close(self) -> None:
        if self._client and not self._client.is_closed:
            await self._client.aclose()


class LLMEndpointRegistry:
    def __init__(self):
        self._manager = LLMManager.get_instance()
        self._custom_clients: dict[str, object] = {}

    def list_endpoints(self) -> list[ProviderEndpoint]:
        endpoints: list[ProviderEndpoint] = []
        llm_cfg = settings.llm

        if llm_cfg.ollama.enabled:
            endpoints.append(
                ProviderEndpoint(
                    provider_id="ollama",
                    adapter="ollama",
                    deployment="local_api",
                    base_url=llm_cfg.ollama.base_url,
                    default_model=llm_cfg.default_model if llm_cfg.default_backend == "ollama" else (llm_cfg.ollama.models[0].name if llm_cfg.ollama.models else ""),
                    enabled=True,
                    models=[model.model_dump() for model in llm_cfg.ollama.models],
                )
            )
        if llm_cfg.openai.enabled:
            endpoints.append(
                ProviderEndpoint(
                    provider_id="openai",
                    adapter="openai_compatible",
                    deployment="external_api",
                    base_url=llm_cfg.openai.base_url,
                    default_model=llm_cfg.default_model if llm_cfg.default_backend == "openai" else (llm_cfg.openai.models[0].name if llm_cfg.openai.models else ""),
                    enabled=True,
                    models=[model.model_dump() for model in llm_cfg.openai.models],
                )
            )
        if llm_cfg.anthropic.enabled:
            endpoints.append(
                ProviderEndpoint(
                    provider_id="claude",
                    adapter="anthropic",
                    deployment="external_api",
                    base_url=llm_cfg.anthropic.base_url,
                    default_model=llm_cfg.default_model if llm_cfg.default_backend == "claude" else (llm_cfg.anthropic.models[0].name if llm_cfg.anthropic.models else ""),
                    enabled=True,
                    models=[model.model_dump() for model in llm_cfg.anthropic.models],
                )
            )
        if llm_cfg.kimi.enabled:
            endpoints.append(
                ProviderEndpoint(
                    provider_id="kimi",
                    adapter="openai_compatible",
                    deployment="external_api",
                    base_url=llm_cfg.kimi.base_url,
                    default_model=llm_cfg.default_model if llm_cfg.default_backend == "kimi" else (llm_cfg.kimi.models[0].name if llm_cfg.kimi.models else ""),
                    enabled=True,
                    models=[model.model_dump() for model in llm_cfg.kimi.models],
                )
            )

        for endpoint_cfg in settings.harness.providers.endpoints:
            if not endpoint_cfg.enabled:
                continue
            endpoints.append(
                ProviderEndpoint(
                    provider_id=endpoint_cfg.name,
                    adapter=endpoint_cfg.adapter,
                    deployment=endpoint_cfg.deployment,
                    base_url=endpoint_cfg.base_url,
                    default_model=endpoint_cfg.default_model,
                    enabled=endpoint_cfg.enabled,
                    models=[model.model_dump() for model in endpoint_cfg.models],
                )
            )

        return endpoints

    def resolve(
        self,
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
    ) -> ProviderEndpoint:
        endpoints = [endpoint for endpoint in self.list_endpoints() if endpoint.enabled]
        if prefer_backend:
            for endpoint in endpoints:
                if endpoint.provider_id == prefer_backend:
                    return endpoint

        if deployment_preference in {"local_api", "external_api"}:
            for endpoint in endpoints:
                if endpoint.deployment == deployment_preference:
                    return endpoint

        if model_name:
            for endpoint in endpoints:
                if any(model.get("name") == model_name for model in endpoint.models):
                    return endpoint

        for endpoint in endpoints:
            if endpoint.provider_id == settings.llm.default_backend:
                return endpoint

        if not endpoints:
            raise RuntimeError("No enabled LLM endpoint configured")
        return endpoints[0]

    async def complete(
        self,
        messages: list[LLMMessage],
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
        **kwargs,
    ) -> LLMResponse:
        endpoint = self.resolve(prefer_backend, model_name, deployment_preference)
        if endpoint.provider_id in {"ollama", "openai", "claude", "kimi"}:
            return await self._manager.achat(
                messages,
                prefer_backend=endpoint.provider_id,
                model_name=model_name or endpoint.default_model,
                **kwargs,
            )
        client = self._get_custom_client(endpoint, model_name or endpoint.default_model)
        return await client.chat(messages, model_name=model_name or endpoint.default_model, **kwargs)

    async def stream(
        self,
        messages: list[LLMMessage],
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
        **kwargs,
    ) -> AsyncIterator[LLMStreamChunk]:
        endpoint = self.resolve(prefer_backend, model_name, deployment_preference)
        if endpoint.provider_id in {"ollama", "openai", "claude", "kimi"}:
            async for chunk in self._manager.astream(
                messages,
                prefer_backend=endpoint.provider_id,
                model_name=model_name or endpoint.default_model,
                **kwargs,
            ):
                yield chunk
            return
        client = self._get_custom_client(endpoint, model_name or endpoint.default_model)
        async for chunk in client.chat_stream(messages, model_name=model_name or endpoint.default_model, **kwargs):
            yield chunk

    async def close(self) -> None:
        for client in self._custom_clients.values():
            close_method = getattr(client, "close", None)
            if close_method is None:
                continue
            maybe_coro = close_method()
            if asyncio.iscoroutine(maybe_coro):
                await maybe_coro
        self._custom_clients.clear()

    def _get_custom_client(self, endpoint: ProviderEndpoint, model_name: str):
        cache_key = f"{endpoint.provider_id}:{model_name}"
        if cache_key in self._custom_clients:
            return self._custom_clients[cache_key]

        endpoint_cfg = next((item for item in settings.harness.providers.endpoints if item.name == endpoint.provider_id), None)
        api_key = ""
        timeout_sec = 120
        if endpoint_cfg is not None:
            api_key = endpoint_cfg.api_key
            if endpoint_cfg.api_key_env:
                api_key = os.getenv(endpoint_cfg.api_key_env, api_key)
            timeout_sec = endpoint_cfg.timeout_sec

        if endpoint.adapter == "ollama":
            client = _OllamaCompatibleClient(endpoint.base_url, model_name, timeout_sec=timeout_sec)
        else:
            client = _OpenAICompatibleClient(endpoint.base_url, api_key, model_name, timeout_sec=timeout_sec)

        self._custom_clients[cache_key] = client
        return client
