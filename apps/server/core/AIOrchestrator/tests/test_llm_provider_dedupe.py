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

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


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

                with patch.object(service._OpenAICompatibleClient, "list_models", new=AsyncMock(return_value=discovered)):
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
                    endpoint
                    for endpoint in registry.list_endpoints()
                    if endpoint.provider_id == first.provider_id
                ]
                self.assertEqual(len(endpoints), 1)
                self.assertEqual(len(endpoints[0].models), 2)
            finally:
                os.environ.pop("MEMOCHAT_API_PROVIDERS_FILE", None)
                _clear_service_stubs()
                sys.modules.pop("harness.llm.service", None)


if __name__ == "__main__":
    unittest.main()
