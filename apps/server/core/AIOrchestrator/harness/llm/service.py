from __future__ import annotations

import asyncio
import hashlib
import ipaddress
import json
import os
import re
import socket
import ssl
from dataclasses import dataclass
from pathlib import Path
from typing import AsyncIterator, Iterable
from urllib.parse import urlsplit

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


@dataclass(frozen=True)
class _ValidatedProviderBaseUrl:
    normalized_url: str
    host: str
    port: int | None
    pinned_addresses: tuple[str, ...]


def _wrap_thinking(reasoning: str, content: str) -> str:
    reasoning = (reasoning or "").strip()
    if not reasoning:
        return content or ""
    return f"<think>{reasoning}</think>{content or ''}"


def _is_moonshot_base_url(base_url: str) -> bool:
    normalized = (base_url or "").lower()
    return "moonshot" in normalized or "platform.kimi" in normalized


def _messages_to_openai_payload(messages: list[LLMMessage]) -> list[dict]:
    return [{"role": message.role, "content": message.content} for message in messages]


def _build_openai_compatible_chat_payload(
    base_url: str,
    model_name: str,
    messages: list[LLMMessage],
    stream: bool,
    **kwargs,
) -> dict:
    selected_model = str(kwargs.get("model_name") or model_name or "").strip()
    payload = {
        "model": selected_model,
        "messages": _messages_to_openai_payload(messages),
        "stream": stream,
    }
    max_tokens = kwargs.get("max_tokens", 2048)
    if _is_moonshot_base_url(base_url):
        payload["max_completion_tokens"] = max_tokens
    else:
        payload["temperature"] = kwargs.get("temperature", 0.7)
        payload["max_tokens"] = max_tokens
    return payload


def _safe_positive_int(value) -> int:
    try:
        parsed = int(value)
    except (TypeError, ValueError):
        return 0
    return parsed if parsed > 0 else 0


def _guess_context_window(model_name: str, base_url: str = "") -> int:
    normalized_model = str(model_name or "").strip().lower()
    normalized_provider = f"{base_url} {normalized_model}".lower()
    if "moonshot" not in normalized_provider and "kimi" not in normalized_provider:
        return 0
    match = re.search(r"(\d+)\s*k", normalized_model)
    if match:
        return int(match.group(1)) * 1024
    if "k2" in normalized_model:
        return 128000
    return 0


def _model_context_window(endpoint: ProviderEndpoint, model_name: str) -> int:
    selected_model = str(model_name or "").strip()
    fallback_model = str(endpoint.default_model or "").strip()
    for model in endpoint.models:
        if str(model.get("name") or "").strip() == selected_model:
            context_window = _safe_positive_int(model.get("context_window"))
            return context_window or _guess_context_window(selected_model, endpoint.base_url)
    for model in endpoint.models:
        if fallback_model and str(model.get("name") or "").strip() == fallback_model:
            context_window = _safe_positive_int(model.get("context_window"))
            return context_window or _guess_context_window(fallback_model, endpoint.base_url)
    return _guess_context_window(selected_model or fallback_model, endpoint.base_url)


def _estimate_prompt_tokens(messages: list[LLMMessage]) -> int:
    total = 16
    for message in messages:
        content = getattr(message, "content", "")
        if not isinstance(content, str):
            content = json.dumps(content, ensure_ascii=False)
        cjk_chars = sum(1 for char in content if "\u4e00" <= char <= "\u9fff")
        other_chars = max(0, len(content) - cjk_chars)
        total += cjk_chars + max(1, (other_chars + 3) // 4) + 8
    return max(1, total)


def _clamp_completion_tokens_for_context(
    endpoint: ProviderEndpoint,
    model_name: str,
    messages: list[LLMMessage],
    kwargs: dict,
) -> None:
    context_window = _model_context_window(endpoint, model_name)
    if context_window <= 0:
        return

    requested = _safe_positive_int(kwargs.get("max_tokens", 2048))
    if requested <= 0:
        return

    prompt_estimate = _estimate_prompt_tokens(messages)
    reserve = max(512, min(2048, context_window // 4))
    max_by_context = context_window - prompt_estimate - reserve
    if max_by_context < 256:
        max_by_context = max(64, min(256, context_window // 4))
    kwargs["max_tokens"] = min(requested, max_by_context)


def _normalize_model_selection(
    prefer_backend: str,
    model_name: str,
    endpoints: list[ProviderEndpoint],
) -> tuple[str, str]:
    backend = (prefer_backend or "").strip()
    model = (model_name or "").strip()
    if not model:
        return backend, model

    if any(model == str(item.get("name") or "").strip() for endpoint in endpoints for item in endpoint.models):
        return backend, model

    if ":" not in model:
        return backend, model

    prefix, raw_model = model.split(":", 1)
    prefix = prefix.strip()
    raw_model = raw_model.strip()
    if not prefix or not raw_model:
        return backend, model

    for endpoint in endpoints:
        if endpoint.provider_id != prefix:
            continue
        if backend and backend != prefix:
            return backend, model
        if any(raw_model == str(item.get("name") or "").strip() for item in endpoint.models):
            return prefix, raw_model
    return backend, model


class _OpenAICompatibleClient:
    def __init__(
        self,
        base_url: str,
        api_key: str,
        model_name: str,
        timeout_sec: int = 120,
        thinking_parameter: str = "",
        pinned_addresses: Iterable[str] | None = None,
    ):
        self.base_url = base_url.rstrip("/")
        self.api_key = api_key
        self.model_name = model_name
        self.timeout_sec = timeout_sec
        self.thinking_parameter = thinking_parameter
        self.pinned_addresses = _public_provider_addresses(pinned_addresses or []) if pinned_addresses else []
        self._client: httpx.AsyncClient | None = None

    async def _get_client(self) -> httpx.AsyncClient:
        if self._client is None or self._client.is_closed:
            headers = {"Authorization": f"Bearer {self.api_key}"} if self.api_key else {}
            transport = None
            if self.pinned_addresses:
                parsed = urlsplit(self.base_url)
                transport = _PinnedProviderAsyncTransport(parsed.hostname or "", self.pinned_addresses)
            self._client = httpx.AsyncClient(
                timeout=httpx.Timeout(float(self.timeout_sec)),
                headers=headers,
                follow_redirects=False,
                trust_env=False,
                transport=transport,
            )
        return self._client

    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        client = await self._get_client()
        payload = _build_openai_compatible_chat_payload(self.base_url, self.model_name, messages, False, **kwargs)
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
        payload = _build_openai_compatible_chat_payload(self.base_url, self.model_name, messages, True, **kwargs)
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
                    yield LLMStreamChunk(
                        content=_wrap_thinking(reasoning, ""),
                        reasoning_content=reasoning,
                        is_final=False,
                        model=payload["model"],
                    )
                if delta:
                    yield LLMStreamChunk(content=delta, is_final=False, model=payload["model"])

    async def list_models(self) -> list[dict]:
        client = await self._get_client()
        response = await client.get(f"{self.base_url}/models")
        if 300 <= response.status_code < 400:
            raise RuntimeError("provider redirects are not allowed")
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
                    "context_window": _guess_context_window(model_id, self.base_url),
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
            content=data.get("message", {}).get("content", "")
            if kwargs.get("think", False)
            else self._strip_think_blocks(data.get("message", {}).get("content", "")),
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
        self._runtime_provider_api_keys: dict[str, str] = {}

    def list_endpoints(self) -> list[ProviderEndpoint]:
        endpoints: list[ProviderEndpoint] = []
        endpoint_identities: list[tuple[str, str, str] | None] = []
        llm_cfg = settings.llm

        def append_endpoint(endpoint: ProviderEndpoint, api_key: str = "") -> None:
            _append_deduped_endpoint(
                endpoints,
                endpoint_identities,
                endpoint,
                _provider_api_identity(endpoint.adapter, endpoint.base_url, api_key),
            )

        if llm_cfg.ollama.enabled:
            ollama_models = self._list_ollama_configured_models(llm_cfg.ollama.base_url, llm_cfg.ollama.models)
            append_endpoint(
                ProviderEndpoint(
                    provider_id="ollama",
                    adapter="ollama",
                    deployment="local_api",
                    base_url=llm_cfg.ollama.base_url,
                    default_model=llm_cfg.default_model
                    if llm_cfg.default_backend == "ollama"
                    and any(model.get("name") == llm_cfg.default_model for model in ollama_models)
                    else (ollama_models[0].get("name", "") if ollama_models else ""),
                    enabled=True,
                    thinking_parameter="think",
                    models=ollama_models,
                )
            )
        if llm_cfg.openai.enabled:
            append_endpoint(
                ProviderEndpoint(
                    provider_id="openai",
                    adapter="openai_compatible",
                    deployment="external_api",
                    base_url=llm_cfg.openai.base_url,
                    default_model=llm_cfg.default_model
                    if llm_cfg.default_backend == "openai"
                    else (llm_cfg.openai.models[0].name if llm_cfg.openai.models else ""),
                    enabled=True,
                    thinking_parameter="",
                    models=[model.model_dump() for model in llm_cfg.openai.models],
                ),
                llm_cfg.openai.api_key,
            )
        if llm_cfg.anthropic.enabled:
            append_endpoint(
                ProviderEndpoint(
                    provider_id="claude",
                    adapter="anthropic",
                    deployment="external_api",
                    base_url=llm_cfg.anthropic.base_url,
                    default_model=llm_cfg.default_model
                    if llm_cfg.default_backend == "claude"
                    else (llm_cfg.anthropic.models[0].name if llm_cfg.anthropic.models else ""),
                    enabled=True,
                    thinking_parameter="",
                    models=[model.model_dump() for model in llm_cfg.anthropic.models],
                ),
                llm_cfg.anthropic.api_key,
            )
        if llm_cfg.kimi.enabled:
            append_endpoint(
                ProviderEndpoint(
                    provider_id="kimi",
                    adapter="openai_compatible",
                    deployment="external_api",
                    base_url=llm_cfg.kimi.base_url,
                    default_model=llm_cfg.default_model
                    if llm_cfg.default_backend == "kimi"
                    else (llm_cfg.kimi.models[0].name if llm_cfg.kimi.models else ""),
                    enabled=True,
                    thinking_parameter="",
                    models=[model.model_dump() for model in llm_cfg.kimi.models],
                ),
                llm_cfg.kimi.api_key,
            )

        for endpoint_cfg in settings.harness.providers.endpoints:
            if not endpoint_cfg.enabled:
                continue
            api_key = endpoint_cfg.api_key
            if endpoint_cfg.api_key_env:
                api_key = os.getenv(endpoint_cfg.api_key_env, api_key)
            if endpoint_cfg.api_key_env and not api_key:
                continue
            append_endpoint(
                ProviderEndpoint(
                    provider_id=endpoint_cfg.name,
                    adapter=endpoint_cfg.adapter,
                    deployment=endpoint_cfg.deployment,
                    base_url=endpoint_cfg.base_url,
                    default_model=endpoint_cfg.default_model,
                    enabled=endpoint_cfg.enabled,
                    thinking_parameter=endpoint_cfg.thinking_parameter,
                    models=[model.model_dump() for model in endpoint_cfg.models],
                ),
                api_key,
            )

        for endpoint_cfg in self._load_runtime_providers():
            if not endpoint_cfg.get("enabled", True):
                continue
            api_key = self._runtime_provider_api_key(endpoint_cfg)
            if not api_key:
                continue
            append_endpoint(
                ProviderEndpoint(
                    provider_id=endpoint_cfg.get("name", ""),
                    adapter=endpoint_cfg.get("adapter", "openai_compatible"),
                    deployment=endpoint_cfg.get("deployment", "external_api"),
                    base_url=endpoint_cfg.get("base_url", ""),
                    default_model=endpoint_cfg.get("default_model", ""),
                    enabled=True,
                    thinking_parameter=endpoint_cfg.get("thinking_parameter", ""),
                    models=endpoint_cfg.get("models", []),
                ),
                api_key,
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
        validated = _validate_public_provider_base_url(base_url)
        normalized_url = validated.normalized_url
        if not api_key:
            raise ValueError("api_key is required")
        if adapter != "openai_compatible":
            raise ValueError("only openai_compatible adapter is supported by UI registration")

        client = _OpenAICompatibleClient(
            normalized_url,
            api_key,
            "",
            timeout_sec=30,
            pinned_addresses=validated.pinned_addresses,
        )
        try:
            models = await client.list_models()
        finally:
            await client.close()
        if not models:
            raise RuntimeError("no models returned from provider")

        providers = self._load_runtime_providers()
        existing_provider = self._find_runtime_provider(provider_id)
        matching_provider = self._find_runtime_provider_by_api(adapter, normalized_url, api_key)
        target_provider = existing_provider or matching_provider
        target_provider_id = str(target_provider.get("name") or provider_id) if target_provider else provider_id
        if target_provider:
            models = _merge_model_lists(target_provider.get("models", []), models)
        target_identity = _provider_api_identity(adapter, normalized_url, api_key)
        providers = [
            provider
            for provider in providers
            if provider.get("name") not in {provider_id, target_provider_id}
            and _runtime_provider_identity(provider) != target_identity
        ]
        api_key_env = _runtime_provider_api_key_env_name(target_provider_id)
        provider_config = {
            "name": target_provider_id,
            "adapter": adapter,
            "deployment": "external_api",
            "base_url": normalized_url,
            "api_key_env": api_key_env,
            "api_key_fingerprint": _provider_api_fingerprint(api_key),
            "default_model": models[0]["name"],
            "enabled": True,
            "timeout_sec": 120,
            "thinking_parameter": _guess_thinking_parameter(provider_id, normalized_url),
            "pinned_addresses": list(validated.pinned_addresses),
            "models": models,
        }
        self._runtime_provider_api_keys[target_provider_id] = api_key
        providers.append(provider_config)
        self._save_runtime_providers(providers)
        for cache_key in list(self._custom_clients.keys()):
            if cache_key.startswith(f"{provider_id}:") or cache_key.startswith(f"{target_provider_id}:"):
                self._custom_clients.pop(cache_key, None)
        return ProviderEndpoint(
            provider_id=target_provider_id,
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
            provider for provider in providers if provider.get("name") not in {provider_id, normalized_provider_id}
        ]
        if len(next_providers) == len(providers):
            return False
        self._save_runtime_providers(next_providers)
        self._runtime_provider_api_keys.pop(provider_id, None)
        self._runtime_provider_api_keys.pop(normalized_provider_id, None)
        for cache_key in list(self._custom_clients.keys()):
            if cache_key.startswith(f"{provider_id}:") or cache_key.startswith(f"{normalized_provider_id}:"):
                self._custom_clients.pop(cache_key, None)
        return True

    def _resolve_with_model(
        self,
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
    ) -> tuple[ProviderEndpoint, str]:
        endpoints = [endpoint for endpoint in self.list_endpoints() if endpoint.enabled]
        prefer_backend, model_name = _normalize_model_selection(prefer_backend, model_name, endpoints)
        if prefer_backend:
            for endpoint in endpoints:
                if endpoint.provider_id == prefer_backend:
                    return endpoint, model_name

        if deployment_preference in {"local_api", "external_api"}:
            for endpoint in endpoints:
                if endpoint.deployment == deployment_preference:
                    return endpoint, model_name

        if model_name:
            for endpoint in endpoints:
                if any(model.get("name") == model_name for model in endpoint.models):
                    return endpoint, model_name

        for endpoint in endpoints:
            if endpoint.provider_id == settings.llm.default_backend:
                return endpoint, model_name

        if not endpoints:
            raise RuntimeError("No enabled LLM endpoint configured")
        return endpoints[0], model_name

    def resolve(
        self,
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
    ) -> ProviderEndpoint:
        endpoint, _ = self._resolve_with_model(prefer_backend, model_name, deployment_preference)
        return endpoint

    async def complete(
        self,
        messages: list[LLMMessage],
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
        **kwargs,
    ) -> LLMResponse:
        endpoint, selected_model = self._resolve_with_model(prefer_backend, model_name, deployment_preference)
        selected_model = selected_model or endpoint.default_model
        request_kwargs = dict(kwargs)
        _clamp_completion_tokens_for_context(endpoint, selected_model, messages, request_kwargs)
        if endpoint.provider_id in {"ollama", "openai", "claude", "kimi"}:
            return await self._manager.achat(
                messages,
                prefer_backend=endpoint.provider_id,
                model_name=selected_model,
                **request_kwargs,
            )
        client = self._get_custom_client(endpoint, selected_model)
        return await client.chat(messages, model_name=selected_model, **request_kwargs)

    async def stream(
        self,
        messages: list[LLMMessage],
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
        **kwargs,
    ) -> AsyncIterator[LLMStreamChunk]:
        endpoint, selected_model = self._resolve_with_model(prefer_backend, model_name, deployment_preference)
        selected_model = selected_model or endpoint.default_model
        request_kwargs = dict(kwargs)
        _clamp_completion_tokens_for_context(endpoint, selected_model, messages, request_kwargs)
        if endpoint.provider_id in {"ollama", "openai", "claude", "kimi"}:
            async for chunk in self._manager.astream(
                messages,
                prefer_backend=endpoint.provider_id,
                model_name=selected_model,
                **request_kwargs,
            ):
                yield chunk
            return
        client = self._get_custom_client(endpoint, selected_model)
        async for chunk in client.chat_stream(messages, model_name=selected_model, **request_kwargs):
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

        endpoint_cfg = next(
            (item for item in settings.harness.providers.endpoints if item.name == endpoint.provider_id), None
        )
        runtime_cfg = self._find_runtime_provider(endpoint.provider_id)
        api_key = ""
        timeout_sec = 120
        pinned_addresses: list[str] = []
        if endpoint_cfg is not None:
            api_key = endpoint_cfg.api_key
            if endpoint_cfg.api_key_env:
                api_key = os.getenv(endpoint_cfg.api_key_env, api_key)
            timeout_sec = endpoint_cfg.timeout_sec
            if endpoint_cfg.adapter == "openai_compatible" and endpoint_cfg.deployment == "external_api":
                pinned_addresses = list(_validate_public_provider_base_url(endpoint.base_url).pinned_addresses)
        elif runtime_cfg is not None:
            api_key = self._runtime_provider_api_key(runtime_cfg)
            timeout_sec = int(runtime_cfg.get("timeout_sec", timeout_sec) or timeout_sec)
            pinned_addresses = _runtime_provider_pinned_addresses(runtime_cfg)
            if not pinned_addresses:
                pinned_addresses = list(_validate_public_provider_base_url(endpoint.base_url).pinned_addresses)

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
                pinned_addresses=pinned_addresses,
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
            return _dedupe_runtime_providers([provider for provider in providers if isinstance(provider, dict)])
        except Exception:
            return []

    def _save_runtime_providers(self, providers: list[dict]) -> None:
        providers = _dedupe_runtime_providers(providers)
        _RUNTIME_PROVIDER_FILE.parent.mkdir(parents=True, exist_ok=True)
        tmp_path = _RUNTIME_PROVIDER_FILE.with_suffix(".tmp")
        with tmp_path.open("w", encoding="utf-8") as file:
            json.dump({"providers": providers}, file, ensure_ascii=False, indent=2)
        tmp_path.replace(_RUNTIME_PROVIDER_FILE)

    def _find_runtime_provider(self, provider_id: str) -> dict | None:
        return next(
            (provider for provider in self._load_runtime_providers() if provider.get("name") == provider_id), None
        )

    def _find_runtime_provider_by_api(self, adapter: str, base_url: str, api_key: str) -> dict | None:
        identity = _provider_api_identity(adapter, base_url, api_key)
        if identity is None:
            return None
        return next(
            (
                provider
                for provider in self._load_runtime_providers()
                if _runtime_provider_identity(provider) == identity
            ),
            None,
        )

    def _runtime_provider_api_key(self, provider_cfg: dict) -> str:
        provider_name = str(provider_cfg.get("name") or "").strip()
        if provider_name and provider_name in self._runtime_provider_api_keys:
            return self._runtime_provider_api_keys[provider_name]
        env_name = str(provider_cfg.get("api_key_env") or "").strip()
        if not env_name and provider_name:
            env_name = _runtime_provider_api_key_env_name(provider_name)
        return os.getenv(env_name, "").strip() if env_name else ""

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


def _runtime_provider_api_key_env_name(provider_id: str) -> str:
    suffix = re.sub(r"[^A-Z0-9]+", "_", (provider_id or "").strip().upper()).strip("_")
    if not suffix:
        suffix = "CUSTOM_API"
    return f"MEMOCHAT_AI_PROVIDER_{suffix}_API_KEY"


def _provider_api_fingerprint(api_key: str) -> str:
    return hashlib.sha256((api_key or "").strip().encode("utf-8")).hexdigest()


def _provider_api_identity(adapter: str, base_url: str, api_key: str) -> tuple[str, str, str] | None:
    normalized_url = _normalize_base_url(base_url)
    normalized_adapter = (adapter or "openai_compatible").strip().lower()
    key = (api_key or "").strip()
    if not normalized_url or not key:
        return None
    return normalized_adapter, normalized_url, _provider_api_fingerprint(key)


def _runtime_provider_identity(provider: dict) -> tuple[str, str, str] | None:
    normalized_url = _normalize_base_url(str(provider.get("base_url") or ""))
    normalized_adapter = str(provider.get("adapter") or "openai_compatible").strip().lower()
    fingerprint = str(provider.get("api_key_fingerprint") or "").strip()
    if not fingerprint:
        legacy_key = str(provider.get("api_key") or "").strip()
        if legacy_key:
            fingerprint = _provider_api_fingerprint(legacy_key)
    if not normalized_url or not fingerprint:
        return None
    return normalized_adapter, normalized_url, fingerprint


def _provider_identities_match(left: tuple[str, str, str] | None, right: tuple[str, str, str] | None) -> bool:
    return left is not None and right is not None and left == right


def _append_deduped_endpoint(
    endpoints: list[ProviderEndpoint],
    identities: list[tuple[str, str, str] | None],
    endpoint: ProviderEndpoint,
    identity: tuple[str, str, str] | None,
) -> None:
    for index, existing in enumerate(endpoints):
        if existing.provider_id == endpoint.provider_id or _provider_identities_match(identities[index], identity):
            if identities[index] is None and identity is not None:
                identities[index] = identity
            existing.models = _merge_model_lists(existing.models, endpoint.models)
            if not existing.default_model and endpoint.default_model:
                existing.default_model = endpoint.default_model
            existing.enabled = existing.enabled or endpoint.enabled
            if not existing.thinking_parameter and endpoint.thinking_parameter:
                existing.thinking_parameter = endpoint.thinking_parameter
            return
    endpoints.append(endpoint)
    identities.append(identity)


def _dedupe_runtime_providers(providers: list[dict]) -> list[dict]:
    result: list[dict] = []
    by_name: dict[str, int] = {}
    by_identity: dict[tuple[str, str, str], int] = {}

    for raw_provider in providers:
        if not isinstance(raw_provider, dict):
            continue
        provider = dict(raw_provider)
        legacy_api_key = str(provider.pop("api_key", "") or "").strip()
        if not provider.get("api_key_fingerprint") and legacy_api_key:
            provider["api_key_fingerprint"] = _provider_api_fingerprint(legacy_api_key)
        name = str(provider.get("name") or "").strip()
        identity = _runtime_provider_identity(provider)
        if name and not provider.get("api_key_env"):
            provider["api_key_env"] = _runtime_provider_api_key_env_name(name)

        index = by_name.get(name) if name else None
        if index is None and identity is not None:
            index = by_identity.get(identity)

        if index is None:
            provider["models"] = _merge_model_lists([], provider.get("models", []))
            if identity is not None:
                by_identity[identity] = len(result)
            if name:
                by_name[name] = len(result)
            result.append(provider)
            continue

        current = result[index]
        current["models"] = _merge_model_lists(current.get("models", []), provider.get("models", []))
        if not current.get("default_model") and provider.get("default_model"):
            current["default_model"] = provider.get("default_model")
        if provider.get("enabled", True):
            current["enabled"] = True
        if not current.get("thinking_parameter") and provider.get("thinking_parameter"):
            current["thinking_parameter"] = provider.get("thinking_parameter")
        if not current.get("timeout_sec") and provider.get("timeout_sec"):
            current["timeout_sec"] = provider.get("timeout_sec")
        if not current.get("api_key_env") and provider.get("api_key_env"):
            current["api_key_env"] = provider.get("api_key_env")
        if not current.get("api_key_fingerprint") and provider.get("api_key_fingerprint"):
            current["api_key_fingerprint"] = provider.get("api_key_fingerprint")
        if not current.get("pinned_addresses") and provider.get("pinned_addresses"):
            current["pinned_addresses"] = provider.get("pinned_addresses")

    return result


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


class _PinnedProviderNetworkBackend:
    def __init__(self, host: str, pinned_addresses: Iterable[str], delegate=None):
        self._host = _normalize_provider_host(host)
        self._pinned_addresses = _public_provider_addresses(pinned_addresses)
        if delegate is None:
            from httpcore._backends.auto import AutoBackend

            delegate = AutoBackend()
        self._delegate = delegate

    async def connect_tcp(
        self,
        host: str,
        port: int,
        timeout: float | None = None,
        local_address: str | None = None,
        socket_options=None,
    ):
        requested_host = _normalize_provider_host(host)
        if requested_host != self._host:
            return await self._delegate.connect_tcp(
                host,
                port,
                timeout=timeout,
                local_address=local_address,
                socket_options=socket_options,
            )

        last_exc: Exception | None = None
        for address in self._pinned_addresses:
            try:
                return await self._delegate.connect_tcp(
                    address,
                    port,
                    timeout=timeout,
                    local_address=local_address,
                    socket_options=socket_options,
                )
            except Exception as exc:
                last_exc = exc
        if last_exc is not None:
            raise last_exc
        raise OSError("provider pinned address list is empty")

    async def connect_unix_socket(self, path: str, timeout: float | None = None, socket_options=None):
        return await self._delegate.connect_unix_socket(path, timeout=timeout, socket_options=socket_options)

    async def sleep(self, seconds: float) -> None:
        await self._delegate.sleep(seconds)


class _PinnedProviderAsyncTransport:
    def __init__(self, host: str, pinned_addresses: Iterable[str]):
        self._host = _normalize_provider_host(host)
        self._pinned_addresses = _public_provider_addresses(pinned_addresses)
        self._pool = None

    def _ensure_pool(self):
        if self._pool is not None:
            return self._pool
        import httpcore

        self._pool = httpcore.AsyncConnectionPool(
            ssl_context=ssl.create_default_context(),
            max_connections=100,
            max_keepalive_connections=20,
            keepalive_expiry=5.0,
            http1=True,
            http2=False,
            retries=0,
            network_backend=_PinnedProviderNetworkBackend(self._host, self._pinned_addresses),
        )
        return self._pool

    async def handle_async_request(self, request):
        import httpcore
        from httpx._transports.default import AsyncResponseStream, map_httpcore_exceptions

        req = httpcore.Request(
            method=request.method,
            url=httpcore.URL(
                scheme=request.url.raw_scheme,
                host=request.url.raw_host,
                port=request.url.port,
                target=request.url.raw_path,
            ),
            headers=request.headers.raw,
            content=request.stream,
            extensions=request.extensions,
        )
        with map_httpcore_exceptions():
            resp = await self._ensure_pool().handle_async_request(req)
        return httpx.Response(
            status_code=resp.status,
            headers=resp.headers,
            stream=AsyncResponseStream(resp.stream),
            extensions=resp.extensions,
        )

    async def aclose(self) -> None:
        if self._pool is not None:
            await self._pool.aclose()


def _normalize_provider_host(host: str) -> str:
    if isinstance(host, bytes):
        host = host.decode("ascii", "ignore")
    normalized = str(host or "").strip().rstrip(".").lower()
    try:
        return normalized.encode("idna").decode("ascii")
    except UnicodeError:
        return normalized


def _public_provider_addresses(addresses: Iterable[str]) -> list[str]:
    result: list[str] = []
    for address in addresses:
        text = str(address or "").strip()
        if not text:
            continue
        if _is_disallowed_provider_ip(text):
            raise ValueError("provider pinned addresses must be public IP addresses")
        if text not in result:
            result.append(text)
    if not result:
        raise ValueError("provider pinned addresses are required")
    return result


def _runtime_provider_pinned_addresses(provider_cfg: dict) -> list[str]:
    raw_addresses = provider_cfg.get("pinned_addresses", [])
    if not isinstance(raw_addresses, list):
        return []
    return _public_provider_addresses(raw_addresses) if raw_addresses else []


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


def _is_disallowed_provider_ip(address: str) -> bool:
    try:
        parsed = ipaddress.ip_address(address)
    except ValueError:
        return True
    return not parsed.is_global


def _resolved_provider_addresses(host: str, port: int | None) -> set[str]:
    try:
        infos = socket.getaddrinfo(host, port, type=socket.SOCK_STREAM)
    except socket.gaierror as exc:
        raise ValueError("base_url host cannot be resolved") from exc

    addresses: set[str] = set()
    for info in infos:
        sockaddr = info[4]
        if not sockaddr:
            continue
        address = str(sockaddr[0])
        if address:
            addresses.add(address)
    if not addresses:
        raise ValueError("base_url host cannot be resolved")
    return addresses


def _validate_public_provider_base_url(base_url: str) -> _ValidatedProviderBaseUrl:
    raw_url = (base_url or "").strip()
    if "://" in raw_url:
        raw_scheme = urlsplit(raw_url).scheme
        if raw_scheme not in {"http", "https"}:
            raise ValueError("base_url must use http or https")

    normalized_url = _normalize_base_url(base_url)
    if not normalized_url:
        raise ValueError("base_url is required")

    try:
        parsed = urlsplit(normalized_url)
        port = parsed.port
    except ValueError as exc:
        raise ValueError("base_url is invalid") from exc

    if parsed.scheme not in {"http", "https"}:
        raise ValueError("base_url must use http or https")
    if parsed.username or parsed.password:
        raise ValueError("base_url must not include user info")

    host = (parsed.hostname or "").strip().rstrip(".")
    if not host:
        raise ValueError("base_url host is required")

    lowered_host = host.lower()
    if lowered_host == "localhost" or lowered_host.endswith(".localhost"):
        raise ValueError("base_url host must be public")

    addresses = _resolved_provider_addresses(host, port)
    for address in addresses:
        if _is_disallowed_provider_ip(address):
            raise ValueError("base_url host must resolve to public addresses")

    return _ValidatedProviderBaseUrl(
        normalized_url=normalized_url,
        host=host,
        port=port,
        pinned_addresses=tuple(sorted(addresses)),
    )


def _normalize_and_validate_public_provider_base_url(base_url: str) -> str:
    return _validate_public_provider_base_url(base_url).normalized_url


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
