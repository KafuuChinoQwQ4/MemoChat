from __future__ import annotations

import asyncio
import json
import os
import re
from pathlib import Path
from typing import AsyncIterator

import httpx

from config import settings
from harness.contracts import ProviderEndpoint
from llm import LLMManager
from llm.base import LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage
from llm.claude_llm import ClaudeLLM


_RUNTIME_PROVIDER_FILE = Path(
    os.getenv(
        "MEMOCHAT_API_PROVIDERS_FILE",
        str(Path(__file__).resolve().parents[2] / ".data" / "api_providers.json"),
    )
)


def _wrap_thinking(reasoning: str, content: str) -> str:
    reasoning = (reasoning or "").strip()
    if not reasoning:
        return content or ""
    return f"<think>{reasoning}</think>{content or ''}"


class _OpenAICompatibleClient:
    def __init__(
        self,
        base_url: str,
        api_key: str,
        model_name: str,
        timeout_sec: int = 120,
        thinking_parameter: str = "",
    ):
        self.base_url = base_url.rstrip("/")
        self.api_key = api_key
        self.model_name = model_name
        self.timeout_sec = timeout_sec
        self.thinking_parameter = thinking_parameter
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
        self._apply_thinking_payload(payload, kwargs.get("think", False))
        response = await client.post(f"{self.base_url}/chat/completions", json=payload)
        response.raise_for_status()
        data = response.json()
        choice = data.get("choices", [{}])[0]
        message = choice.get("message", {})
        usage = data.get("usage", {})
        reasoning = message.get("reasoning_content") or message.get("reasoning") or ""
        content = message.get("content", "")
        return LLMResponse(
            content=_wrap_thinking(reasoning, content) if kwargs.get("think", False) else content,
            reasoning_content=reasoning if kwargs.get("think", False) else "",
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
        self._apply_thinking_payload(payload, kwargs.get("think", False))
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
                delta_obj = data.get("choices", [{}])[0].get("delta", {})
                reasoning = delta_obj.get("reasoning_content") or delta_obj.get("reasoning") or ""
                delta = delta_obj.get("content", "")
                if reasoning and kwargs.get("think", False):
                    yield LLMStreamChunk(content=_wrap_thinking(reasoning, ""), reasoning_content=reasoning, is_final=False, model=payload["model"])
                if delta:
                    yield LLMStreamChunk(content=delta, is_final=False, model=payload["model"])

    async def list_models(self) -> list[dict]:
        client = await self._get_client()
        response = await client.get(f"{self.base_url}/models")
        response.raise_for_status()
        data = response.json()
        raw_models = data.get("data", data.get("models", []))
        models: list[dict] = []
        for item in raw_models:
            if isinstance(item, str):
                model_id = item
            elif isinstance(item, dict):
                model_id = item.get("id") or item.get("name") or item.get("model") or ""
            else:
                model_id = ""
            model_id = str(model_id).strip()
            if not model_id:
                continue
            models.append(
                {
                    "name": model_id,
                    "display": model_id,
                    "context_window": 0,
                    "supports_thinking": _guess_supports_thinking(model_id),
                }
            )
        return models

    def _apply_thinking_payload(self, payload: dict, enabled: bool) -> None:
        if not enabled or not self.thinking_parameter:
            return
        if self.thinking_parameter == "enable_thinking":
            payload["enable_thinking"] = True
        elif self.thinking_parameter == "reasoning":
            payload["reasoning"] = True
        elif self.thinking_parameter == "include_reasoning":
            payload["include_reasoning"] = True
        elif self.thinking_parameter == "gemini_thinking":
            payload["extra_body"] = {"google": {"thinking_config": {"include_thoughts": True}}}
        elif self.thinking_parameter == "glm_thinking":
            payload["extra_body"] = {"thinking": {"type": "enabled"}}

    async def close(self) -> None:
        if self._client and not self._client.is_closed:
            await self._client.aclose()


class _OllamaCompatibleClient:
    def __init__(self, base_url: str, model_name: str, timeout_sec: int = 300):
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
            "think": bool(kwargs.get("think", False)),
            "options": {
                "temperature": kwargs.get("temperature", 0.7),
                "num_predict": kwargs.get("max_tokens", 2048),
            },
        }
        response = await client.post(f"{self.base_url}/api/chat", json=payload)
        response.raise_for_status()
        data = response.json()
        return LLMResponse(
            content=data.get("message", {}).get("content", "") if kwargs.get("think", False) else self._strip_think_blocks(data.get("message", {}).get("content", "")),
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
            "think": bool(kwargs.get("think", False)),
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
                if not kwargs.get("think", False):
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
            ollama_models = self._list_ollama_configured_models(llm_cfg.ollama.base_url, llm_cfg.ollama.models)
            endpoints.append(
                ProviderEndpoint(
                    provider_id="ollama",
                    adapter="ollama",
                    deployment="local_api",
                    base_url=llm_cfg.ollama.base_url,
                    default_model=llm_cfg.default_model if llm_cfg.default_backend == "ollama" and any(model.get("name") == llm_cfg.default_model for model in ollama_models) else (ollama_models[0].get("name", "") if ollama_models else ""),
                    enabled=True,
                    thinking_parameter="think",
                    models=ollama_models,
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
                    thinking_parameter="",
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
                    thinking_parameter="",
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
                    thinking_parameter="",
                    models=[model.model_dump() for model in llm_cfg.kimi.models],
                )
            )

        for endpoint_cfg in settings.harness.providers.endpoints:
            if not endpoint_cfg.enabled:
                continue
            api_key = endpoint_cfg.api_key
            if endpoint_cfg.api_key_env:
                api_key = os.getenv(endpoint_cfg.api_key_env, api_key)
            if endpoint_cfg.api_key_env and not api_key:
                continue
            endpoints.append(
                ProviderEndpoint(
                    provider_id=endpoint_cfg.name,
                    adapter=endpoint_cfg.adapter,
                    deployment=endpoint_cfg.deployment,
                    base_url=endpoint_cfg.base_url,
                    default_model=endpoint_cfg.default_model,
                    enabled=endpoint_cfg.enabled,
                    thinking_parameter=endpoint_cfg.thinking_parameter,
                    models=[model.model_dump() for model in endpoint_cfg.models],
                )
            )

        for endpoint_cfg in self._load_runtime_providers():
            if not endpoint_cfg.get("enabled", True):
                continue
            if not endpoint_cfg.get("api_key", ""):
                continue
            endpoints.append(
                ProviderEndpoint(
                    provider_id=endpoint_cfg.get("name", ""),
                    adapter=endpoint_cfg.get("adapter", "openai_compatible"),
                    deployment=endpoint_cfg.get("deployment", "external_api"),
                    base_url=endpoint_cfg.get("base_url", ""),
                    default_model=endpoint_cfg.get("default_model", ""),
                    enabled=True,
                    thinking_parameter=endpoint_cfg.get("thinking_parameter", ""),
                    models=endpoint_cfg.get("models", []),
                )
            )

        return endpoints

    async def register_api_provider(
        self,
        provider_name: str,
        base_url: str,
        api_key: str,
        adapter: str = "openai_compatible",
    ) -> ProviderEndpoint:
        provider_id = _normalize_provider_id(provider_name or "custom-api")
        normalized_url = _normalize_base_url(base_url)
        if not normalized_url:
            raise ValueError("base_url is required")
        if not api_key:
            raise ValueError("api_key is required")
        if adapter != "openai_compatible":
            raise ValueError("only openai_compatible adapter is supported by UI registration")

        client = _OpenAICompatibleClient(normalized_url, api_key, "", timeout_sec=30)
        try:
            models = await client.list_models()
        finally:
            await client.close()
        if not models:
            raise RuntimeError("no models returned from provider")

        providers = [provider for provider in self._load_runtime_providers() if provider.get("name") != provider_id]
        existing_provider = self._find_runtime_provider(provider_id)
        if existing_provider:
            models = _merge_model_lists(existing_provider.get("models", []), models)
        provider_config = {
            "name": provider_id,
            "adapter": adapter,
            "deployment": "external_api",
            "base_url": normalized_url,
            "api_key": api_key,
            "default_model": models[0]["name"],
            "enabled": True,
            "timeout_sec": 120,
            "thinking_parameter": _guess_thinking_parameter(provider_id, normalized_url),
            "models": models,
        }
        providers.append(provider_config)
        self._save_runtime_providers(providers)
        for cache_key in list(self._custom_clients.keys()):
            if cache_key.startswith(f"{provider_id}:"):
                self._custom_clients.pop(cache_key, None)
        return ProviderEndpoint(
            provider_id=provider_id,
            adapter=adapter,
            deployment="external_api",
            base_url=normalized_url,
            default_model=models[0]["name"],
            enabled=True,
            thinking_parameter=provider_config["thinking_parameter"],
            models=models,
        )

    def delete_api_provider(self, provider_id: str) -> bool:
        normalized_provider_id = _normalize_provider_id(provider_id or "")
        providers = self._load_runtime_providers()
        next_providers = [
            provider
            for provider in providers
            if provider.get("name") not in {provider_id, normalized_provider_id}
        ]
        if len(next_providers) == len(providers):
            return False
        self._save_runtime_providers(next_providers)
        for cache_key in list(self._custom_clients.keys()):
            if cache_key.startswith(f"{provider_id}:") or cache_key.startswith(f"{normalized_provider_id}:"):
                self._custom_clients.pop(cache_key, None)
        return True

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
        runtime_cfg = self._find_runtime_provider(endpoint.provider_id)
        api_key = ""
        timeout_sec = 120
        if endpoint_cfg is not None:
            api_key = endpoint_cfg.api_key
            if endpoint_cfg.api_key_env:
                api_key = os.getenv(endpoint_cfg.api_key_env, api_key)
            timeout_sec = endpoint_cfg.timeout_sec
        elif runtime_cfg is not None:
            api_key = runtime_cfg.get("api_key", "")
            timeout_sec = int(runtime_cfg.get("timeout_sec", timeout_sec) or timeout_sec)

        if endpoint.adapter == "ollama":
            client = _OllamaCompatibleClient(endpoint.base_url, model_name, timeout_sec=timeout_sec)
        elif endpoint.adapter == "anthropic":
            client = ClaudeLLM(api_key=api_key, base_url=endpoint.base_url, model_name=model_name)
        else:
            client = _OpenAICompatibleClient(
                endpoint.base_url,
                api_key,
                model_name,
                timeout_sec=timeout_sec,
                thinking_parameter=endpoint.thinking_parameter,
            )

        self._custom_clients[cache_key] = client
        return client

    def _load_runtime_providers(self) -> list[dict]:
        try:
            if not _RUNTIME_PROVIDER_FILE.exists():
                return []
            with _RUNTIME_PROVIDER_FILE.open("r", encoding="utf-8") as file:
                data = json.load(file)
            providers = data.get("providers", []) if isinstance(data, dict) else []
            return [provider for provider in providers if isinstance(provider, dict)]
        except Exception:
            return []

    def _save_runtime_providers(self, providers: list[dict]) -> None:
        _RUNTIME_PROVIDER_FILE.parent.mkdir(parents=True, exist_ok=True)
        tmp_path = _RUNTIME_PROVIDER_FILE.with_suffix(".tmp")
        with tmp_path.open("w", encoding="utf-8") as file:
            json.dump({"providers": providers}, file, ensure_ascii=False, indent=2)
        tmp_path.replace(_RUNTIME_PROVIDER_FILE)

    def _find_runtime_provider(self, provider_id: str) -> dict | None:
        return next((provider for provider in self._load_runtime_providers() if provider.get("name") == provider_id), None)

    def _list_ollama_configured_models(self, base_url: str, configured_models: list) -> list[dict]:
        configured_by_name = {model.name: model.model_dump() for model in configured_models}
        try:
            response = httpx.get(f"{base_url.rstrip('/')}/api/tags", timeout=3.0)
            response.raise_for_status()
            data = response.json()
        except Exception:
            return []

        result: list[dict] = []
        for item in data.get("models", []):
            if not isinstance(item, dict):
                continue
            model_name = str(item.get("name") or item.get("model") or "").strip()
            if not model_name:
                continue
            model_info = configured_by_name.get(
                model_name,
                {
                    "name": model_name,
                    "display": model_name,
                    "context_window": 0,
                    "supports_thinking": _guess_supports_thinking(model_name),
                },
            )
            result.append(model_info)
        return result


def _normalize_provider_id(name: str) -> str:
    cleaned = re.sub(r"[^a-zA-Z0-9_-]+", "-", name.strip().lower()).strip("-")
    if not cleaned:
        cleaned = "custom-api"
    if not cleaned.startswith("api-"):
        cleaned = f"api-{cleaned}"
    return cleaned[:48]


def _merge_model_lists(existing_models: list[dict], discovered_models: list[dict]) -> list[dict]:
    merged: dict[str, dict] = {}
    for model in existing_models:
        if not isinstance(model, dict):
            continue
        name = str(model.get("name") or "").strip()
        if name:
            merged[name] = dict(model)
    for model in discovered_models:
        if not isinstance(model, dict):
            continue
        name = str(model.get("name") or "").strip()
        if name:
            current = merged.get(name, {})
            current.update(model)
            merged[name] = current
    return list(merged.values())


def _normalize_base_url(base_url: str) -> str:
    url = (base_url or "").strip().rstrip("/")
    if not url:
        return ""
    if not url.startswith(("http://", "https://")):
        url = "https://" + url
    lower = url.lower()
    if "api.openai.com" in lower and not lower.endswith("/v1"):
        url += "/v1"
    if "api.deepseek.com" in lower and lower.endswith("/v1"):
        url = url[:-3]
    return url.rstrip("/")


def _guess_supports_thinking(model_id: str) -> bool:
    normalized = model_id.lower()
    return "reasoner" in normalized or "thinking" in normalized or normalized.startswith("qwen3")


def _guess_thinking_parameter(provider_id: str, base_url: str) -> str:
    text = f"{provider_id} {base_url}".lower()
    if "dashscope" in text or "qwen" in text:
        return "enable_thinking"
    if "googleapis" in text or "gemini" in text:
        return "gemini_thinking"
    return ""
