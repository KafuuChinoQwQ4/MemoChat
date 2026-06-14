import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
FEATURE_VIEW = CLIENT / "features/pet/view"
FEATURE_RUNTIME = CLIENT / "features/pet/runtime"
LEGACY_VIEW = CLIENT / "qml/pet"
FEATURE_QRC = CLIENT / "features/pet/resources/pet.qrc"
LEGACY_QRC = CLIENT / "resources/qrc/qml-pet.qrc"
SOURCES = CLIENT / "features/pet/sources.cmake"

FEATURE_FILES = (
    "Live2DAssetSummaryPanel.qml",
    "Live2DAvatarPreviewImage.qml",
    "Live2DBehaviorMemoryPanel.qml",
    "Live2DCharacterAssetColumn.qml",
    "Live2DCharacterBehaviorColumn.qml",
    "Live2DCharacterPane.qml",
    "Live2DCharacterPreviewPanel.qml",
    "Live2DCharacterPromptColumn.qml",
    "Live2DComboBlock.qml",
    "Live2DFormFieldBlock.qml",
    "Live2DPersonaPanel.qml",
    "Live2DPromptPreviewPanel.qml",
    "Live2DResourceVoicePanel.qml",
    "Live2DSectionPanel.qml",
    "Live2DSliderBlock.qml",
    "Live2DSpeechStylePanel.qml",
    "Live2DStatusChip.qml",
    "Live2DTextAreaBlock.qml",
    "Live2DToggleRow.qml",
    "PetAudioPlayer.qml",
    "PetCameraCapture.qml",
    "PetChatComposer.qml",
    "PetChatHeader.qml",
    "PetChatMessageList.qml",
    "PetChatWindow.qml",
    "PetControlApiProviderPanel.qml",
    "PetControlHeader.qml",
    "PetControlLive2DActionPanel.qml",
    "PetControlWindow.qml",
    "PetScene.qml",
    "PetVisionPrivacyCard.qml",
    "PetWindow.qml",
)

# Pure JavaScript runtime helpers live in a dedicated runtime/ folder, not view/.
RUNTIME_FILES = (
    "Live2DCharacterRuntime.js",
    "PetChatRuntime.js",
    "PetControlRuntime.js",
    "PetWindowRuntime.js",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class PetQmlPhysicalLayoutContractTests(unittest.TestCase):
    def test_pet_qml_lives_under_feature_view(self):
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertTrue((FEATURE_VIEW / file_name).is_file())
                self.assertFalse((LEGACY_VIEW / file_name).exists())

    def test_pet_runtime_js_lives_under_feature_runtime(self):
        for file_name in RUNTIME_FILES:
            with self.subTest(file=file_name):
                self.assertTrue((FEATURE_RUNTIME / file_name).is_file())
                self.assertFalse((FEATURE_VIEW / file_name).exists())

    def test_pet_qrc_uses_feature_aliases_only(self):
        self.assertFalse(LEGACY_QRC.exists())
        feature_qrc = read(FEATURE_QRC)
        legacy_qrc = read(LEGACY_QRC)
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertIn(f'alias="features/pet/view/{file_name}">../view/{file_name}</file>', feature_qrc)
                self.assertNotIn(f'alias="qml/pet/{file_name}"', legacy_qrc)
        for file_name in RUNTIME_FILES:
            with self.subTest(file=file_name):
                self.assertIn(f'alias="features/pet/runtime/{file_name}">../runtime/{file_name}</file>', feature_qrc)

    def test_pet_qrc_is_registered_by_feature_manifest(self):
        self.assertIn("features/pet/resources/pet.qrc", read(SOURCES))


if __name__ == "__main__":
    unittest.main()
