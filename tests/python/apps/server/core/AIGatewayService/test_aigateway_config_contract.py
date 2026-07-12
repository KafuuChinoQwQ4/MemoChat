"""AIGateway runtime configuration contracts."""

import configparser
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
AIGATEWAY_DIR = REPO_ROOT / "apps" / "server" / "core" / "AIGatewayService"
AIGATEWAY_CONFIG = AIGATEWAY_DIR / "aigateway.ini"
AI_SERVICE = AIGATEWAY_DIR / "domain" / "services" / "ai" / "AIService.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def load_config(path: Path) -> configparser.ConfigParser:
    parser = configparser.ConfigParser()
    parser.optionxform = str
    with path.open(encoding="utf-8-sig") as handle:
        parser.read_file(handle)
    return parser


class AIGatewayConfigContractTests(unittest.TestCase):
    def test_user_auth_token_validation_has_redis_config(self):
        source = read(AI_SERVICE)
        self.assertIn(
            "memochat::auth::ResolveBearerAccessUserId",
            source,
            "AIGateway AI routes validate Bearer access tokens through the shared Redis-backed validator",
        )

        config = load_config(AIGATEWAY_CONFIG)
        self.assertIn(
            "Redis",
            config,
            "AIGateway config must include Redis because AI user auth initializes RedisMgr",
        )

        redis = config["Redis"]
        for key in ("Host", "Port", "Passwd", "PoolSize"):
            with self.subTest(key=key):
                self.assertIn(key, redis)

        for key in ("Host", "Port", "PoolSize"):
            with self.subTest(key=key):
                self.assertTrue(redis[key].strip(), f"Redis.{key} must not be empty")

        self.assertEqual(redis["Host"], "127.0.0.1")
        self.assertEqual(redis["Port"], "6379")
        self.assertEqual(redis["Passwd"], "", "Redis credentials must not be committed in service config")

    def test_ai_orchestrator_and_provider_admin_auth_config_is_preserved(self):
        config = load_config(AIGATEWAY_CONFIG)

        orchestrator = config["AIOrchestrator"]
        self.assertEqual(orchestrator["Host"], "127.0.0.1")
        self.assertEqual(orchestrator["Port"], "8096")
        self.assertEqual(orchestrator["InternalAuthHeader"], "X-MemoChat-AI-Internal-Key")
        self.assertEqual(orchestrator["InternalApiKeyEnv"], "MEMOCHAT_AI_INTERNAL_API_KEY")

        ai_server = config["AIServer"]
        self.assertEqual(ai_server["Host"], "127.0.0.1")
        self.assertEqual(ai_server["InternalAuthHeader"], "X-MemoChat-AI-Internal-Key")
        self.assertEqual(ai_server["InternalApiKeyEnv"], "MEMOCHAT_AI_INTERNAL_API_KEY")

        provider_admin = config["AIProviderAdmin"]
        self.assertEqual(provider_admin["AuthHeader"], "X-MemoChat-AI-Provider-Admin-Key")
        self.assertEqual(provider_admin["AdminKeyEnv"], "MEMOCHAT_AI_PROVIDER_ADMIN_KEY")


if __name__ == "__main__":
    unittest.main()
