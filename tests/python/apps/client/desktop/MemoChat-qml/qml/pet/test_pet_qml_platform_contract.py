import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
QRC_ROOT = CLIENT_DIR / "resources/qrc"
QML_QRCS = (
    QRC_ROOT / "qml-shell.qrc",
    CLIENT_DIR / "features/pet/resources/pet.qrc",
    QRC_ROOT / "icons.qrc",
)
PET_WINDOW_QML = CLIENT_DIR / "features/pet/view/PetWindow.qml"
PET_WINDOW_RUNTIME_JS = CLIENT_DIR / "features/pet/runtime/PetWindowRuntime.js"
PET_SCENE_QML = CLIENT_DIR / "features/pet/view/PetScene.qml"
SHARED_MAIN_QML = CLIENT_DIR / "qml/app/Main.qml"
LINUX_MAIN_QML = CLIENT_DIR / "qml/linux/Main.qml"
LINUX_GLASS_SURFACE_QML = CLIENT_DIR / "qml/linux/components/GlassSurface.qml"
SHARED_GLASS_SURFACE_QML = CLIENT_DIR / "qml/components/GlassSurface.qml"


def read(path):
    return path.read_text(encoding="utf-8")


def qrc_text():
    return "\n".join(read(path) for path in QML_QRCS)


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
    return source[match.end() : index - 1]


class PetQmlPlatformContractTests(unittest.TestCase):
    def test_qrc_registers_linux_and_pet_platform_resources(self):
        qrc = qrc_text()

        for token in (
            "qml/linux/Main.qml",
            "qml/linux/components/WindowGlassShell.qml",
            "qml/linux/components/GlassSurface.qml",
            "features/pet/view/PetWindow.qml",
            "features/pet/view/PetScene.qml",
            "features/pet/view/Live2DCharacterPane.qml",
            'alias="icons/modelive2d.png"',
        ):
            self.assertIn(token, qrc)

    def test_pet_window_keeps_transparent_click_through_opt_in_and_recoverable(self):
        window = read(PET_WINDOW_QML)
        runtime = read(PET_WINDOW_RUNTIME_JS)
        open_body = function_body(window, "openPet")
        flags_body = function_body(window, "applyWindowFlags") + "\n" + function_body(runtime, "petWindowFlags")

        self.assertIn('color: "transparent"', window)
        self.assertRegex(flags_body, r"\b(?:Qt|qt)\.WindowTransparentForInput\b")
        self.assertRegex(flags_body, r"\bif\s*\(\s*clickThrough\s*\)")
        self.assertRegex(open_body, r"\bclickThrough\s*=\s*false\b")
        self.assertRegex(open_body, r"\bdecorativeMode\s*=\s*false\b")

    def test_pet_qml_avoids_unstable_transparent_window_effects(self):
        combined = read(PET_WINDOW_QML) + "\n" + read(PET_SCENE_QML)

        for forbidden in (
            "ShaderEffectSource",
            "MultiEffect",
            "layer.enabled",
            "QRegion",
            "clip: true",
        ):
            self.assertNotIn(forbidden, combined)

    def test_shared_and_linux_entry_points_create_the_same_pet_window_contract(self):
        for path in (SHARED_MAIN_QML, LINUX_MAIN_QML):
            source = read(path)
            self.assertIn('import "qrc:/features/pet/view"', source)
            self.assertIn("function ensurePetWindow(petAssetSettings)", source)
            self.assertIn("function openPetWindow(petAssetSettings)", source)
            self.assertIn("petWindowComponent.createObject", source)
            self.assertIn('"petController": pet', source)
            self.assertIn('"petAssetSettings": settings', source)
            self.assertIn("PetWindow { }", source)

    def test_linux_glass_compatibility_path_stays_additive(self):
        linux_surface = read(LINUX_GLASS_SURFACE_QML)
        shared_surface = read(SHARED_GLASS_SURFACE_QML)

        self.assertNotIn("ShaderEffectSource", linux_surface)
        self.assertNotIn("MultiEffect", linux_surface)
        self.assertIn("ShaderEffectSource", shared_surface)
        self.assertIn("MultiEffect", shared_surface)


if __name__ == "__main__":
    unittest.main()
