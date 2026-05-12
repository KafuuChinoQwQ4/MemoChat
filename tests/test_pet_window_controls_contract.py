import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
PET_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetWindow.qml"
PET_SCENE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetScene.qml"

WINDOW_CONTROL_PROPERTIES = (
    "alwaysOnTop",
    "clickThrough",
    "decorativeMode",
    "debugPanelVisible",
    "scaleFactor",
    "micMuted",
    "cameraEnabled",
    "cloudVisionEnabled",
    "localOnlyMode",
    "debugRetentionEnabled",
    "providerAvailable",
)

SCENE_CONTROL_SIGNALS = (
    "resetPositionRequested",
    "alwaysOnTopToggled",
    "clickThroughToggled",
    "debugToggled",
    "scaleRequested",
    "micMuteToggled",
    "cameraToggled",
    "cloudVisionToggled",
    "localOnlyModeToggled",
    "debugRetentionToggled",
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


class PetWindowControlsContractTests(unittest.TestCase):
    def test_pet_window_exposes_control_state_and_helpers(self):
        window = read_qml(PET_WINDOW_QML)

        for prop in WINDOW_CONTROL_PROPERTIES:
            self.assertRegex(window, r"\bproperty\s+\w+\s+" + re.escape(prop) + r"\b")

        for helper in ("openPet", "resetPosition", "applyWindowFlags"):
            self.assertRegex(window, r"\bfunction\s+" + re.escape(helper) + r"\s*\(")

    def test_pet_window_apply_flags_uses_additive_control_flags(self):
        window = read_qml(PET_WINDOW_QML)
        body = function_body(window, "applyWindowFlags")
        self.assertTrue(body, "PetWindow.qml must define applyWindowFlags()")

        for token in (
            "Qt.Tool",
            "Qt.FramelessWindowHint",
            "Qt.WindowStaysOnTopHint",
            "Qt.WindowTransparentForInput",
        ):
            self.assertIn(token, body)

        self.assertRegex(body, r"\balwaysOnTop\b")
        self.assertRegex(body, r"\bclickThrough\b")

    def test_pet_window_recovers_interactive_mode_when_reopened(self):
        window = read_qml(PET_WINDOW_QML)
        body = function_body(window, "openPet")
        self.assertTrue(body, "PetWindow.qml must define openPet()")

        self.assertRegex(body, r"\bclickThrough\s*=\s*false\b")
        self.assertRegex(body, r"\bdecorativeMode\s*=\s*false\b")
        self.assertRegex(window, r"\bdecorativeMode\s*=\s*clickThrough\b")

    def test_pet_window_wires_control_state_into_scene(self):
        window = read_qml(PET_WINDOW_QML)

        self.assertIn("PetScene", window)
        for prop in WINDOW_CONTROL_PROPERTIES:
            self.assertRegex(window, re.escape(prop) + r"\s*:\s*root\." + re.escape(prop))

        for signal_name in SCENE_CONTROL_SIGNALS:
            handler = "on" + signal_name[0].upper() + signal_name[1:]
            self.assertIn(handler, window)

    def test_pet_scene_declares_control_inputs_and_signals(self):
        scene = read_qml(PET_SCENE_QML)

        for prop in WINDOW_CONTROL_PROPERTIES:
            self.assertRegex(scene, r"\bproperty\s+\w+\s+" + re.escape(prop) + r"\b")

        expected_signatures = {
            "resetPositionRequested": r"\bsignal\s+resetPositionRequested\s*\(\s*\)",
            "alwaysOnTopToggled": r"\bsignal\s+alwaysOnTopToggled\s*\(\s*bool\b",
            "clickThroughToggled": r"\bsignal\s+clickThroughToggled\s*\(\s*bool\b",
            "debugToggled": r"\bsignal\s+debugToggled\s*\(\s*bool\b",
            "scaleRequested": r"\bsignal\s+scaleRequested\s*\(\s*real\b",
            "micMuteToggled": r"\bsignal\s+micMuteToggled\s*\(\s*bool\b",
            "cameraToggled": r"\bsignal\s+cameraToggled\s*\(\s*bool\b",
            "cloudVisionToggled": r"\bsignal\s+cloudVisionToggled\s*\(\s*bool\b",
        }
        for name, pattern in expected_signatures.items():
            self.assertRegex(scene, pattern, name)

    def test_pet_scene_contains_compact_controls_and_privacy_debug_ui(self):
        scene = read_qml(PET_SCENE_QML)

        self.assertIn("Slider", scene)
        self.assertRegex(scene, r"\bfrom\s*:\s*[0-9.]+")
        self.assertRegex(scene, r"\bto\s*:\s*[0-9.]+")
        self.assertIn("scaleRequested", scene)

        self.assertIn("debugPanelVisible", scene)
        self.assertRegex(scene, r"\bvisible\s*:\s*.*debugPanelVisible")

        for token in ("micMuted", "cameraEnabled", "cloudVisionEnabled"):
            self.assertIn(token, scene)

        self.assertRegex(scene, r"\bcomposer\b")
        self.assertRegex(scene, r"\bvisible\s*:\s*[^\n]*(decorativeMode|clickThrough)")
        self.assertRegex(scene, r"\benabled\s*:\s*[^\n]*(decorativeMode|clickThrough)")


if __name__ == "__main__":
    unittest.main()
