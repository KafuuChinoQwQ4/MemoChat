from __future__ import annotations

import asyncio
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
            os.environ.pop("MEMOCHAT_AI_PROVIDER_API_DEEPSEEK_FLASH_API_KEY", None)
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

                with (
                    patch.object(
                        service.socket,
                        "getaddrinfo",
                        return_value=[(None, None, None, "", ("93.184.216.34", 443))],
                    ),
                    patch.object(
                        service._OpenAICompatibleClient, "list_models", new=AsyncMock(return_value=discovered)
                    ),
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
                self.assertNotIn("api_key", providers[0])
                self.assertEqual(providers[0]["api_key_env"], "MEMOCHAT_AI_PROVIDER_API_DEEPSEEK_FLASH_API_KEY")
                self.assertRegex(providers[0]["api_key_fingerprint"], r"^[0-9a-f]{64}$")
                self.assertEqual(providers[0]["pinned_addresses"], ["93.184.216.34"])
                self.assertEqual(
                    sorted(model["name"] for model in providers[0]["models"]),
                    ["deepseek-v4-flash", "deepseek-v4-pro"],
                )

                endpoints = [
                    endpoint for endpoint in registry.list_endpoints() if endpoint.provider_id == first.provider_id
                ]
                self.assertEqual(len(endpoints), 1)
                self.assertEqual(len(endpoints[0].models), 2)

                restarted_registry = service.LLMEndpointRegistry()
                self.assertFalse(
                    [
                        endpoint
                        for endpoint in restarted_registry.list_endpoints()
                        if endpoint.provider_id == first.provider_id
                    ]
                )
                with patch.dict(
                    os.environ,
                    {"MEMOCHAT_AI_PROVIDER_API_DEEPSEEK_FLASH_API_KEY": "same-key"},
                    clear=False,
                ):
                    restarted_endpoints = [
                        endpoint
                        for endpoint in restarted_registry.list_endpoints()
                        if endpoint.provider_id == first.provider_id
                    ]
                self.assertEqual(len(restarted_endpoints), 1)
            finally:
                os.environ.pop("MEMOCHAT_API_PROVIDERS_FILE", None)
                _clear_service_stubs()
                sys.modules.pop("harness.llm.service", None)


class AIOrchestratorSecurityConfigTests(unittest.TestCase):
    def test_internal_auth_config_helpers_allow_health_and_loopback_only(self):
        _clear_service_stubs()
        sys.modules.pop("config", None)

        import config as config_module

        self.assertTrue(config_module.is_unauthenticated_path("/health", ["/health", "/ready"]))
        self.assertTrue(config_module.is_unauthenticated_path("/health/", ["/health"]))
        self.assertFalse(config_module.is_unauthenticated_path("/chat", ["/health", "/ready"]))
        self.assertTrue(config_module.is_trusted_client_host("127.0.0.1", ["127.0.0.1", "::1"]))
        self.assertFalse(config_module.is_trusted_client_host("10.0.0.8", ["127.0.0.1", "::1"]))

        security = config_module.SecurityConfig(
            internal_api_key="config-key",
            internal_api_key_env="MEMOCHAT_TEST_AI_INTERNAL_KEY",
            provider_admin_key="provider-config-key",
            provider_admin_key_env="MEMOCHAT_TEST_AI_PROVIDER_ADMIN_KEY",
        )
        with patch.dict(os.environ, {"MEMOCHAT_TEST_AI_INTERNAL_KEY": "env-key"}, clear=False):
            self.assertEqual(config_module.resolve_internal_api_key(security), "env-key")
        with patch.dict(os.environ, {}, clear=True):
            self.assertEqual(config_module.resolve_internal_api_key(security), "config-key")
        with patch.dict(os.environ, {"MEMOCHAT_TEST_AI_PROVIDER_ADMIN_KEY": "provider-env-key"}, clear=False):
            self.assertEqual(config_module.resolve_provider_admin_key(security), "provider-env-key")
        with patch.dict(os.environ, {}, clear=True):
            self.assertEqual(config_module.resolve_provider_admin_key(security), "provider-config-key")


class LLMProviderSsrfGuardTests(unittest.TestCase):
    def setUp(self):
        _install_service_stubs()
        import harness.llm.service as service

        self.service = importlib.reload(service)

    def tearDown(self):
        _clear_service_stubs()
        sys.modules.pop("harness.llm.service", None)

    def test_provider_registration_rejects_local_and_private_hosts(self):
        with self.assertRaisesRegex(ValueError, "public"):
            self.service._normalize_and_validate_public_provider_base_url("http://localhost:11434")

        with patch.object(
            self.service.socket,
            "getaddrinfo",
            return_value=[(None, None, None, "", ("10.0.0.12", 443))],
        ):
            with self.assertRaisesRegex(ValueError, "public addresses"):
                self.service._normalize_and_validate_public_provider_base_url("https://api.example.test/v1")

        with self.assertRaisesRegex(ValueError, "http or https"):
            self.service._normalize_and_validate_public_provider_base_url("ftp://api.example.test")

    def test_provider_registration_accepts_public_http_hosts(self):
        with patch.object(
            self.service.socket,
            "getaddrinfo",
            return_value=[(None, None, None, "", ("93.184.216.34", 443))],
        ):
            self.assertEqual(
                self.service._normalize_and_validate_public_provider_base_url("https://api.deepseek.com/v1"),
                "https://api.deepseek.com",
            )

    def test_openai_compatible_client_disables_redirects(self):
        calls = {}

        class FakeAsyncClient:
            is_closed = False

            def __init__(self, **kwargs):
                calls.update(kwargs)

        self.service.httpx.AsyncClient = FakeAsyncClient
        self.service.httpx.Timeout = lambda value: ("timeout", value)

        client = self.service._OpenAICompatibleClient("https://api.example.test/v1", "key", "")
        asyncio.run(client._get_client())

        self.assertIn("follow_redirects", calls)
        self.assertFalse(calls["follow_redirects"])
        self.assertFalse(calls["trust_env"])
        self.assertIsNone(calls["transport"])

    def test_openai_compatible_client_uses_pinned_transport_for_runtime_provider(self):
        calls = {}

        class FakeAsyncClient:
            is_closed = False

            def __init__(self, **kwargs):
                calls.update(kwargs)

        self.service.httpx.AsyncClient = FakeAsyncClient
        self.service.httpx.Timeout = lambda value: ("timeout", value)

        client = self.service._OpenAICompatibleClient(
            "https://api.example.test/v1",
            "key",
            "",
            pinned_addresses=["93.184.216.34"],
        )
        asyncio.run(client._get_client())

        self.assertFalse(calls["trust_env"])
        self.assertFalse(calls["follow_redirects"])
        self.assertIsNotNone(calls["transport"])

    def test_static_external_provider_uses_pinned_transport(self):
        calls = {}

        class FakeClient:
            def __init__(
                self, base_url, api_key, model_name, timeout_sec=120, thinking_parameter="", pinned_addresses=None
            ):
                calls["base_url"] = base_url
                calls["api_key"] = api_key
                calls["model_name"] = model_name
                calls["timeout_sec"] = timeout_sec
                calls["thinking_parameter"] = thinking_parameter
                calls["pinned_addresses"] = pinned_addresses

        endpoint_cfg = types.SimpleNamespace(
            name="static-provider",
            adapter="openai_compatible",
            deployment="external_api",
            base_url="https://api.example.test/v1",
            api_key="static-key",
            api_key_env="",
            timeout_sec=45,
            thinking_parameter="",
        )
        self.service.settings.harness.providers.endpoints = [endpoint_cfg]
        endpoint = _ProviderEndpoint(
            provider_id="static-provider",
            adapter="openai_compatible",
            deployment="external_api",
            base_url="https://api.example.test/v1",
            default_model="demo",
            enabled=True,
        )

        with (
            patch.object(self.service, "_OpenAICompatibleClient", FakeClient),
            patch.object(
                self.service.socket,
                "getaddrinfo",
                return_value=[(None, None, None, "", ("93.184.216.34", 443))],
            ),
        ):
            self.service.LLMEndpointRegistry()._get_custom_client(endpoint, "demo")

        self.assertEqual(calls["base_url"], "https://api.example.test/v1")
        self.assertEqual(calls["api_key"], "static-key")
        self.assertEqual(calls["model_name"], "demo")
        self.assertEqual(calls["timeout_sec"], 45)
        self.assertEqual(calls["pinned_addresses"], ["93.184.216.34"])

    def test_static_external_provider_rejects_private_dns_before_client_creation(self):
        class FakeClient:
            def __init__(self, *args, **kwargs):
                raise AssertionError("client should not be created for private provider addresses")

        endpoint_cfg = types.SimpleNamespace(
            name="static-provider",
            adapter="openai_compatible",
            deployment="external_api",
            base_url="https://api.example.test/v1",
            api_key="static-key",
            api_key_env="",
            timeout_sec=45,
            thinking_parameter="",
        )
        self.service.settings.harness.providers.endpoints = [endpoint_cfg]
        endpoint = _ProviderEndpoint(
            provider_id="static-provider",
            adapter="openai_compatible",
            deployment="external_api",
            base_url="https://api.example.test/v1",
            default_model="demo",
            enabled=True,
        )

        with (
            patch.object(self.service, "_OpenAICompatibleClient", FakeClient),
            patch.object(
                self.service.socket,
                "getaddrinfo",
                return_value=[(None, None, None, "", ("10.0.0.8", 443))],
            ),
            self.assertRaisesRegex(ValueError, "public addresses"),
        ):
            self.service.LLMEndpointRegistry()._get_custom_client(endpoint, "demo")

    def test_pinned_provider_network_backend_connects_to_validated_ip_only(self):
        class FakeDelegate:
            def __init__(self):
                self.calls = []

            async def connect_tcp(self, host, port, timeout=None, local_address=None, socket_options=None):
                self.calls.append((host, port))
                return {"host": host, "port": port}

            async def connect_unix_socket(self, path, timeout=None, socket_options=None):
                return {"path": path}

            async def sleep(self, seconds):
                return None

        delegate = FakeDelegate()
        backend = self.service._PinnedProviderNetworkBackend(
            "api.example.test",
            ["93.184.216.34"],
            delegate=delegate,
        )

        pinned_result = asyncio.run(backend.connect_tcp("api.example.test", 443))
        passthrough_result = asyncio.run(backend.connect_tcp("other.example.test", 443))

        self.assertEqual(pinned_result["host"], "93.184.216.34")
        self.assertEqual(passthrough_result["host"], "other.example.test")
        self.assertEqual(delegate.calls, [("93.184.216.34", 443), ("other.example.test", 443)])

        with self.assertRaisesRegex(ValueError, "public IP"):
            self.service._PinnedProviderNetworkBackend(
                "api.example.test",
                ["10.0.0.8"],
                delegate=FakeDelegate(),
            )


if __name__ == "__main__":
    unittest.main()
