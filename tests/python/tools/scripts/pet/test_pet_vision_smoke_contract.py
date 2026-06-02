import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
PET_VISION_SMOKE = REPO_ROOT / "tools/scripts/pet/smoke_pet_vision_wsl.sh"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class PetVisionSmokeContractTests(unittest.TestCase):
    def test_wsl_smoke_exposes_full_visual_layer_test_user_flow(self):
        script = read(PET_VISION_SMOKE)

        for token in (
            "--full-visual-layer",
            "909007",
            "vision-smoke-capture",
            "vision-smoke-agent",
            "PYTHON_BIN",
            "/diagnostics/vision",
            "/sessions",
            "/capture",
            "/observation",
            "/input",
            "/interrupt",
            "/sessions?uid=",
            "generate_synthetic_png",
            "synthetic-test-user.png",
        ):
            self.assertIn(token, script)

    def test_wsl_smoke_checks_visual_privacy_and_agent_boundaries(self):
        script = read(PET_VISION_SMOKE)

        for token in (
            '"include_frame": False',
            "raw_frame_sent",
            "retention",
            "uploaded_frame",
            "scene.summary",
            "visual_summary",
            "visual_react",
            "first_user_seen",
            "我看到你啦",
            "心情还不错",
            "low_confidence",
            "no_face",
            "cooldown",
            "agent input after visual summary OK",
        ):
            self.assertIn(token, script)


if __name__ == "__main__":
    unittest.main()
