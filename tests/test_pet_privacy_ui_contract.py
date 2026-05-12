import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
PET_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetWindow.qml"
PET_SCENE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetScene.qml"

PRIVACY_PROPERTIES = (
    "localOnlyMode",
    "debugRetentionEnabled",
    "providerAvailable",
)

PRIVACY_HELPERS = (
    "micPrivacyText",
    "cameraPrivacyText",
    "cloudPrivacyText",
    "localModeText",
    "debugRetentionText",
    "privacyColor",
)


def read_qml(path):
    return path.read_text(encoding="utf-8")


def function_body(source, name):
    match = re.search(r"\bfunction\s+" + re.escape(name) + r"\s*\([^)]*\)\s*\{", source)
    if not match:
        return ""

    depth = 1
    index = match.end()
    while index < len(source) and depth > 0:
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        index += 1
    return source[match.end():index - 1]


class PetPrivacyUiContractTests(unittest.TestCase):
    def test_window_exposes_safe_privacy_defaults_and_forwards_to_scene(self):
        window = read_qml(PET_WINDOW_QML)

        self.assertRegex(window, r"\bproperty\s+bool\s+localOnlyMode\s*:\s*true\b")
        self.assertRegex(window, r"\bproperty\s+bool\s+debugRetentionEnabled\s*:\s*false\b")
        self.assertRegex(window, r"\bproperty\s+bool\s+providerAvailable\s*:\s*false\b")

        for prop in PRIVACY_PROPERTIES:
            self.assertRegex(window, re.escape(prop) + r"\s*:\s*root\." + re.escape(prop))

        self.assertIn("onLocalOnlyModeToggled", window)
        self.assertIn("onDebugRetentionToggled", window)

    def test_scene_declares_privacy_inputs_signals_and_helpers(self):
        scene = read_qml(PET_SCENE_QML)

        for prop in PRIVACY_PROPERTIES:
            self.assertRegex(scene, r"\bproperty\s+bool\s+" + re.escape(prop) + r"\b")

        self.assertRegex(scene, r"\bsignal\s+localOnlyModeToggled\s*\(\s*bool\b")
        self.assertRegex(scene, r"\bsignal\s+debugRetentionToggled\s*\(\s*bool\b")

        for helper in PRIVACY_HELPERS:
            self.assertRegex(scene, r"\bfunction\s+" + re.escape(helper) + r"\s*\(")

    def test_cloud_privacy_text_distinguishes_disabled_unavailable_and_active(self):
        scene = read_qml(PET_SCENE_QML)
        body = function_body(scene, "cloudPrivacyText")
        self.assertTrue(body, "PetScene.qml must define cloudPrivacyText()")

        self.assertIn("cloudVisionEnabled", body)
        self.assertIn("providerAvailable", body)
        self.assertIn("云视觉 关闭", body)
        self.assertIn("云视觉 无提供方", body)
        self.assertIn("云视觉 已授权", body)

    def test_local_and_debug_text_make_safe_states_explicit(self):
        scene = read_qml(PET_SCENE_QML)

        local_body = function_body(scene, "localModeText")
        debug_body = function_body(scene, "debugRetentionText")
        self.assertIn("本地优先", local_body)
        self.assertIn("允许云能力", local_body)
        self.assertIn("调试保留 关闭", debug_body)
        self.assertIn("调试保留 开启", debug_body)

    def test_debug_retention_control_is_debug_gated(self):
        scene = read_qml(PET_SCENE_QML)

        self.assertIn("debugRetentionEnabled", scene)
        self.assertRegex(
            scene,
            r"\bvisible\s*:\s*root\.debugPanelVisible\s*&&\s*!root\.decorativeMode",
        )
        self.assertIn("debugRetentionToggled(!root.debugRetentionEnabled)", scene)

    def test_custom_signals_do_not_collide_with_property_notifiers(self):
        scene = read_qml(PET_SCENE_QML)

        self.assertNotRegex(scene, r"\bsignal\s+localOnlyModeChanged\b")
        self.assertNotRegex(scene, r"\bsignal\s+debugRetentionEnabledChanged\b")
        self.assertNotRegex(scene, r"\bsignal\s+providerAvailableChanged\b")


if __name__ == "__main__":
    unittest.main()
