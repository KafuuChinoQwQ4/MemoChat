from __future__ import annotations

import importlib
import json
import os
import sys
import tempfile
import types
import unittest
from dataclasses import dataclass, field
from pathlib import Path
from unittest.mock import AsyncMock, patch

from tests.python.support.paths import ai_orchestrator_root

sys.path.insert(0, str(ai_orchestrator_root()))


@dataclass
class _ProviderEndpoint:
    provider_id: str
    adapter: str
    deployment: str
    base_url: str
    default_model: str
    enabled: bool
    thinking_parameter: str = ""
    models: list[dict] = field(default_factory=list)


class _LLMManager:
    @classmethod
    def get_instance(cls):
        return cls()


def _install_service_stubs():
    httpx = types.ModuleType("httpx")
    httpx.AsyncClient = object
    httpx.Timeout = object
    httpx.get = lambda *args, **kwargs: None
    config = types.ModuleType("config")
    config.settings = types.SimpleNamespace(
        llm=types.SimpleNamespace(
            default_backend="",
            default_model="",
            ollama=types.SimpleNamespace(enabled=False, base_url="", models=[]),
            openai=types.SimpleNamespace(enabled=False, base_url="", api_key="", models=[]),
            anthropic=types.SimpleNamespace(enabled=False, base_url="", api_key="", models=[]),
            kimi=types.SimpleNamespace(enabled=False, base_url="", api_key="", models=[]),
        ),
        harness=types.SimpleNamespace(providers=types.SimpleNamespace(endpoints=[])),
    )
    contracts = types.ModuleType("harness.contracts")
    contracts.ProviderEndpoint = _ProviderEndpoint
    llm = types.ModuleType("llm")
    llm.LLMManager = _LLMManager
    base = types.ModuleType("llm.base")
    base.LLMMessage = type("LLMMessage", (), {})
    base.LLMResponse = type("LLMResponse", (), {})
    base.LLMStreamChunk = type("LLMStreamChunk", (), {})
    base.LLMUsage = type("LLMUsage", (), {})
    claude = types.ModuleType("llm.claude_llm")
    claude.ClaudeLLM = type("ClaudeLLM", (), {})
    sys.modules.update(
        {
            "config": config,
            "harness.contracts": contracts,
            "llm": llm,
            "llm.base": base,
            "llm.claude_llm": claude,
            "httpx": httpx,
        }
    )


def _clear_service_stubs():
    for name in ("config", "harness.contracts", "llm", "llm.base", "llm.claude_llm", "httpx"):
        sys.modules.pop(name, None)


class LLMProviderDedupeTests(unittest.IsolatedAsyncioTestCase):
    async def test_moonshot_chat_payload_uses_provider_safe_parameters(self):
        _install_service_stubs()
        try:
            import harness.llm.service as service

            service = importlib.reload(service)
            payload = service._build_openai_compatible_chat_payload(
                base_url="https://api.moonshot.cn/v1",
                model_name="kimi-k2.7-code",
                messages=[types.SimpleNamespace(role="user", content="hello")],
                stream=False,
                temperature=0.7,
                max_tokens=8192,
            )

            self.assertEqual(payload["model"], "kimi-k2.7-code")
            self.assertEqual(payload["messages"], [{"role": "user", "content": "hello"}])
            self.assertFalse(payload["stream"])
            self.assertEqual(payload["max_completion_tokens"], 8192)
            self.assertNotIn("max_tokens", payload)
            self.assertNotIn("temperature", payload)
        finally:
            _clear_service_stubs()
            sys.modules.pop("harness.llm.service", None)

    async def test_prefixed_model_selector_routes_provider_but_sends_raw_model_id(self):
        _install_service_stubs()
        try:
            import harness.llm.service as service

            service = importlib.reload(service)
            endpoints = [
                _ProviderEndpoint(
                    provider_id="api-kimi",
                    adapter="openai_compatible",
                    deployment="external_api",
                    base_url="https://api.moonshot.cn/v1",
                    default_model="kimi-k2.7-code",
                    enabled=True,
                    models=[{"name": "kimi-k2.7-code"}],
                ),
                _ProviderEndpoint(
                    provider_id="ollama",
                    adapter="ollama",
                    deployment="local_api",
                    base_url="http://127.0.0.1:11434",
                    default_model="qwen3:4b",
                    enabled=True,
                    models=[{"name": "qwen3:4b"}],
                ),
            ]

            backend, model_name = service._normalize_model_selection("", "api-kimi:kimi-k2.7-code", endpoints)
            self.assertEqual(backend, "api-kimi")
            self.assertEqual(model_name, "kimi-k2.7-code")

            backend, model_name = service._normalize_model_selection("ollama", "qwen3:4b", endpoints)
            self.assertEqual(backend, "ollama")
            self.assertEqual(model_name, "qwen3:4b")
        finally:
            _clear_service_stubs()
            sys.modules.pop("harness.llm.service", None)

    async def test_completion_budget_is_capped_below_kimi_context_window(self):
        _install_service_stubs()
        try:
            import harness.llm.service as service

            service = importlib.reload(service)
            endpoint = _ProviderEndpoint(
                provider_id="kimi",
                adapter="openai_compatible",
                deployment="external_api",
                base_url="https://api.moonshot.cn/v1",
                default_model="moonshot-v1-8k",
                enabled=True,
                models=[{"name": "moonshot-v1-8k", "context_window": 8192}],
            )
            kwargs = {"max_tokens": 8192}

            service._clamp_completion_tokens_for_context(
                endpoint,
                "moonshot-v1-8k",
                [
                    types.SimpleNamespace(role="system", content="你是 MemoChat 的 AI 助手。"),
                    types.SimpleNamespace(role="user", content="hello"),
                ],
                kwargs,
            )

            self.assertLess(kwargs["max_tokens"], 8192)
            self.assertGreaterEqual(kwargs["max_tokens"], 256)
        finally:
            _clear_service_stubs()
            sys.modules.pop("harness.llm.service", None)

    async def test_completion_budget_infers_kimi_context_when_model_metadata_is_missing(self):
        _install_service_stubs()
        try:
            import harness.llm.service as service

            service = importlib.reload(service)
            endpoint = _ProviderEndpoint(
                provider_id="api-kimi",
                adapter="openai_compatible",
                deployment="external_api",
                base_url="https://api.moonshot.cn/v1",
                default_model="moonshot-v1-8k",
                enabled=True,
                models=[{"name": "moonshot-v1-8k", "context_window": 0}],
            )
            kwargs = {"max_tokens": 8192}

            service._clamp_completion_tokens_for_context(
                endpoint,
                "moonshot-v1-8k",
                [types.SimpleNamespace(role="user", content="hello")],
                kwargs,
            )

            self.assertLess(kwargs["max_tokens"], 8192)
        finally:
            _clear_service_stubs()
            sys.modules.pop("harness.llm.service", None)

    async def test_runtime_provider_registration_is_idempotent_for_same_api(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            provider_file = Path(tmpdir) / "api_providers.json"
            os.environ["MEMOCHAT_API_PROVIDERS_FILE"] = str(provider_file)
            _install_service_stubs()

            try:
                import harness.llm.service as service

                service = importlib.reload(service)
                registry = service.LLMEndpointRegistry()
                discovered = [
                    {
                        "name": "deepseek-v4-flash",
                        "display": "DeepSeek Flash",
                        "context_window": 64000,
                        "supports_thinking": False,
                    },
                    {
                        "name": "deepseek-v4-pro",
                        "display": "DeepSeek Pro",
                        "context_window": 64000,
                        "supports_thinking": True,
                    },
                ]

                with patch.object(
                    service._OpenAICompatibleClient, "list_models", new=AsyncMock(return_value=discovered)
                ):
                    first = await registry.register_api_provider(
                        "deepseek flash",
                        "https://api.deepseek.com/v1",
                        "same-key",
                    )
                    second = await registry.register_api_provider(
                        "deepseek pro",
                        "https://api.deepseek.com",
                        "same-key",
                    )

                self.assertEqual(first.provider_id, second.provider_id)
                data = json.loads(provider_file.read_text(encoding="utf-8"))
                providers = data["providers"]
                self.assertEqual(len(providers), 1)
                self.assertEqual(providers[0]["base_url"], "https://api.deepseek.com")
                self.assertEqual(
                    sorted(model["name"] for model in providers[0]["models"]),
                    ["deepseek-v4-flash", "deepseek-v4-pro"],
                )

                endpoints = [
                    endpoint for endpoint in registry.list_endpoints() if endpoint.provider_id == first.provider_id
                ]
                self.assertEqual(len(endpoints), 1)
                self.assertEqual(len(endpoints[0].models), 2)
            finally:
                os.environ.pop("MEMOCHAT_API_PROVIDERS_FILE", None)
                _clear_service_stubs()
                sys.modules.pop("harness.llm.service", None)


if __name__ == "__main__":
    unittest.main()
