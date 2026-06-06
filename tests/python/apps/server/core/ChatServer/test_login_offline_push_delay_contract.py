import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[6]
CHAT_SERVER_DIR = REPO_ROOT / "apps/server/core/ChatServer"
RUNTIME_SERVICES_DIR = REPO_ROOT / "infra/Memo_ops/runtime/services"
RESUME = REPO_ROOT / "docs/resume/memochat_resume.typ"


class LoginOfflinePushDelayContractTest(unittest.TestCase):
    def test_code_default_and_configs_use_immediate_post_login_offline_push(self):
        source = (CHAT_SERVER_DIR / "config/ChatSessionConfig.cpp").read_text(encoding="utf-8")
        self.assertRegex(
            source,
            r'ConfigInt\("LogicSystem",\s*"LoginOfflinePushDelayMs",\s*0,\s*0,\s*60000\)',
        )

        config_paths = [CHAT_SERVER_DIR / "config.ini"]
        config_paths.extend(CHAT_SERVER_DIR / f"chatserver{i}.ini" for i in range(1, 7))
        config_paths.extend(RUNTIME_SERVICES_DIR / f"chatserver{i}/config.ini" for i in range(1, 7))

        for path in config_paths:
            with self.subTest(path=str(path.relative_to(REPO_ROOT))):
                text = path.read_text(encoding="utf-8")
                self.assertIn("LoginOfflinePushDelayMs=0", text)
                self.assertNotIn("LoginOfflinePushDelayMs=1000", text)

    def test_resume_matches_client_buffer_offline_push_design(self):
        resume = RESUME.read_text(encoding="utf-8")
        self.assertIn("客户端首屏消息缓冲区承接离线补推", resume)
        self.assertNotIn("LoginOfflinePushDelayMs=1000", resume)


if __name__ == "__main__":
    unittest.main()
