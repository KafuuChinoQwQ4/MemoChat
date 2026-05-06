import os
import sys
from pathlib import Path
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from config import LangSmithConfig, ObservabilityConfig


class LangSmithConfigTests(unittest.TestCase):
    def test_langsmith_defaults_are_safe_for_local_dev(self):
        cfg = LangSmithConfig()

        self.assertEqual(cfg.endpoint, "https://api.smith.langchain.com")
        self.assertEqual(cfg.api_key_env, "LANGSMITH_API_KEY")
        self.assertEqual(cfg.project, "memochat-ai-local")
        self.assertEqual(cfg.sampling_rate, 1.0)
        self.assertTrue(cfg.redact_inputs)
        self.assertFalse(cfg.upload_user_content)

    def test_observability_backend_defaults_to_langsmith(self):
        cfg = ObservabilityConfig(enabled=True)

        self.assertEqual(cfg.backend, "langsmith")
        self.assertTrue(cfg.prometheus_enabled)


class LangSmithAdapterTests(unittest.TestCase):
    def setUp(self):
        self._env_patch = mock.patch.dict(os.environ, {}, clear=True)
        self._env_patch.start()

    def tearDown(self):
        self._env_patch.stop()

    def test_init_sets_official_langsmith_environment_without_secret_leak(self):
        from observability.langsmith_instrument import init_langsmith

        obs = ObservabilityConfig(
            enabled=True,
            backend="langsmith",
            langsmith=LangSmithConfig(
                tracing=True,
                api_key_env="LANGSMITH_API_KEY",
                project="memo-test",
                endpoint="https://api.smith.langchain.com",
            ),
        )
        os.environ["LANGSMITH_API_KEY"] = "test-secret"

        status = init_langsmith(obs)

        self.assertTrue(status.enabled)
        self.assertEqual(status.project, "memo-test")
        self.assertEqual(os.environ["LANGSMITH_TRACING"], "true")
        self.assertEqual(os.environ["LANGSMITH_PROJECT"], "memo-test")
        self.assertEqual(os.environ["LANGSMITH_ENDPOINT"], "https://api.smith.langchain.com")
        self.assertNotIn("test-secret", repr(status))

    def test_init_respects_official_langsmith_project_environment(self):
        from observability.langsmith_instrument import init_langsmith

        obs = ObservabilityConfig(
            enabled=True,
            backend="langsmith",
            langsmith=LangSmithConfig(project="config-project"),
        )
        os.environ["LANGSMITH_API_KEY"] = "test-secret"
        os.environ["LANGSMITH_PROJECT"] = "env-project"

        status = init_langsmith(obs)

        self.assertTrue(status.enabled)
        self.assertEqual(status.project, "env-project")
        self.assertEqual(os.environ["LANGSMITH_PROJECT"], "env-project")

    def test_official_langsmith_tracing_env_can_disable_adapter(self):
        from observability.langsmith_instrument import init_langsmith, trace_context

        obs = ObservabilityConfig(enabled=True, backend="langsmith", langsmith=LangSmithConfig(tracing=True))
        os.environ["LANGSMITH_API_KEY"] = "test-secret"
        os.environ["LANGSMITH_TRACING"] = "false"

        status = init_langsmith(obs)

        self.assertFalse(status.enabled)
        self.assertEqual(status.reason, "disabled")
        with trace_context("env-disabled", obs, inputs={"uid": 1}) as run:
            self.assertIsNone(run)

    def test_redact_payload_hashes_uid_and_removes_content_by_default(self):
        from observability.langsmith_instrument import sanitize_payload

        payload = {
            "uid": 123,
            "content": "用户原始问题",
            "query": "知识库问题",
            "token": "secret-token",
            "nested": {"api_key": "secret-key", "safe": "ok"},
        }
        cfg = LangSmithConfig(upload_user_content=False, redact_inputs=True)

        redacted = sanitize_payload(payload, cfg)

        self.assertIn("uid_hash", redacted)
        self.assertNotIn("uid", redacted)
        self.assertEqual(redacted["content"], "[redacted]")
        self.assertEqual(redacted["query"], "[redacted]")
        self.assertEqual(redacted["token"], "[redacted]")
        self.assertEqual(redacted["nested"]["api_key"], "[redacted]")
        self.assertEqual(redacted["nested"]["safe"], "ok")

    def test_trace_context_is_noop_when_observability_disabled(self):
        from observability.langsmith_instrument import trace_context

        obs = ObservabilityConfig(enabled=False)

        with trace_context("disabled", obs, inputs={"uid": 1}) as run:
            self.assertIsNone(run)

    def test_trace_context_fails_open_when_langsmith_start_fails(self):
        from observability.langsmith_instrument import trace_context

        obs = ObservabilityConfig(
            enabled=True,
            backend="langsmith",
            langsmith=LangSmithConfig(tracing=True, api_key_env="LANGSMITH_API_KEY"),
        )
        os.environ["LANGSMITH_API_KEY"] = "test-secret"

        with mock.patch("langsmith.run_helpers.trace", side_effect=RuntimeError("network down")):
            with trace_context("start-failure", obs, inputs={"uid": 1}) as run:
                self.assertIsNone(run)


if __name__ == "__main__":
    unittest.main()
