import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"

PET_ASSET_SETTINGS_H = CLIENT_DIR / "PetAssetSettings.h"
PET_ASSET_SETTINGS_CPP = CLIENT_DIR / "PetAssetSettings.cpp"
CLIENT_CMAKE = CLIENT_DIR / "CMakeLists.txt"
MAIN_CPP = CLIENT_DIR / "main.cpp"
CHARACTER_PANE_QML = CLIENT_DIR / "qml/pet/Live2DCharacterPane.qml"
QML_QRC = CLIENT_DIR / "qml.qrc"

REQUIRED_PROPERTIES = (
    "characterName",
    "roleIdentity",
    "modelRoot",
    "modelJson",
    "motionDirectory",
    "expressionDirectory",
    "voiceDirectory",
    "defaultVoice",
    "idleMotion",
    "speakingMotion",
    "fallbackExpression",
    "personalityTags",
    "relationshipStyle",
    "worldSetting",
    "speechRules",
    "catchphrases",
    "forbiddenRules",
    "toneIndex",
    "responseLengthIndex",
    "languageIndex",
    "emotionLevel",
    "creativityLevel",
    "voiceSpeed",
    "lipSyncSensitivity",
    "voiceLipSyncEnabled",
    "emotionSoundEnabled",
    "idleMotionEnabled",
    "gazeFollowEnabled",
    "memoryEnabled",
    "interruptEnabled",
    "cameraEnabled",
    "cloudVisionEnabled",
    "storagePath",
    "dirty",
    "statusText",
)

PERSISTED_DRAFT_FIELDS = tuple(
    prop
    for prop in REQUIRED_PROPERTIES
    if prop not in {"storagePath", "dirty", "statusText"}
)

REQUIRED_INVOKABLES = (
    "load",
    "save",
    "resetToDefaults",
    "toVariantMap",
)

QML_EDITABLE_FIELDS = (
    "characterName",
    "roleIdentity",
    "modelRoot",
    "modelJson",
    "motionDirectory",
    "expressionDirectory",
    "voiceDirectory",
    "defaultVoice",
    "idleMotion",
    "speakingMotion",
    "fallbackExpression",
    "personalityTags",
    "relationshipStyle",
    "worldSetting",
    "speechRules",
    "catchphrases",
    "forbiddenRules",
    "emotionLevel",
    "creativityLevel",
    "voiceSpeed",
    "lipSyncSensitivity",
)

FORBIDDEN_LICENSED_ASSET_TOKENS = (
    "src/KafuuChino",
    "KafuuChino",
    "香风智乃live2D",
    "香风智乃.model3.json",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def function_body(source: str, name: str) -> str:
    match = re.search(rf"\bfunction\s+{re.escape(name)}\s*\([^)]*\)\s*\{{", source)
    if not match:
        raise AssertionError(f"Expected QML function {name}() to exist")

    start = match.end() - 1
    depth = 0
    for index in range(start, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start + 1:index]
    raise AssertionError(f"Expected QML function {name}() to have a closed body")


class PetAssetSettingsContractTests(unittest.TestCase):
    def assertFileExists(self, path: Path) -> None:
        self.assertTrue(path.exists(), f"{path.relative_to(REPO_ROOT)} should exist")

    def assertContains(self, text: str, needle: str, message: str | None = None) -> None:
        if needle not in text:
            self.fail(message or f"Expected source to contain {needle!r}")

    def test_pet_asset_settings_files_exist_and_are_added_to_client_cmake(self):
        self.assertFileExists(PET_ASSET_SETTINGS_H)
        self.assertFileExists(PET_ASSET_SETTINGS_CPP)

        cmake = read(CLIENT_CMAKE)
        self.assertRegex(cmake, r"\bPetAssetSettings\.cpp\b")
        self.assertRegex(cmake, r"\bPetAssetSettings\.h\b")

    def test_pet_asset_settings_qobject_contract_is_exposed(self):
        self.assertFileExists(PET_ASSET_SETTINGS_H)
        header = read(PET_ASSET_SETTINGS_H)

        self.assertContains(header, "class PetAssetSettings")
        self.assertContains(header, "Q_OBJECT")
        for prop in REQUIRED_PROPERTIES:
            self.assertRegex(
                header,
                rf"Q_PROPERTY\s*\([^)]*\b{re.escape(prop)}\b[^)]*\)",
                f"PetAssetSettings should expose Q_PROPERTY {prop}",
            )

        for invokable in REQUIRED_INVOKABLES:
            self.assertRegex(
                header,
                rf"Q_INVOKABLE\s+[^\n;]*\b{re.escape(invokable)}\s*\(",
                f"PetAssetSettings should expose Q_INVOKABLE {invokable}()",
            )

    def test_main_registers_pet_asset_settings_in_memo_chat_module(self):
        main = read(MAIN_CPP)

        self.assertContains(main, '#include "PetAssetSettings.h"')
        self.assertRegex(
            main,
            r"qmlRegisterType\s*<\s*PetAssetSettings\s*>\s*\(\s*\"MemoChat\"\s*,\s*1\s*,\s*0\s*,\s*\"PetAssetSettings\"\s*\)",
        )

    def test_pet_asset_settings_uses_app_data_json_storage(self):
        self.assertFileExists(PET_ASSET_SETTINGS_CPP)
        source = read(PET_ASSET_SETTINGS_CPP)

        for token in (
            "QStandardPaths",
            "QStandardPaths::AppDataLocation",
            "QJsonDocument",
            "QJsonObject",
            "QFile",
            "QDir",
            'QStringLiteral("schema_version")',
            "live2d-character-draft.json",
        ):
            self.assertContains(source, token)

        self.assertRegex(source, r"pet[/\\]live2d-character-draft\.json")
        self.assertRegex(source, r"QJsonDocument::fromJson\s*\(")
        self.assertRegex(source, r"\.toJson\s*\(")
        self.assertRegex(source, r"\bQIODevice::WriteOnly\b")
        self.assertRegex(source, r"\bQIODevice::ReadOnly\b")

    def test_pet_asset_settings_serializes_declared_draft_fields(self):
        self.assertFileExists(PET_ASSET_SETTINGS_CPP)
        source = read(PET_ASSET_SETTINGS_CPP)

        for field in PERSISTED_DRAFT_FIELDS:
            self.assertContains(
                source,
                field,
                f"PetAssetSettings persistence should mention draft field {field}",
            )

        self.assertContains(source, "toVariantMap")
        self.assertContains(source, "schema_version")

    def test_pet_asset_settings_clamps_invalid_json_to_safe_defaults(self):
        self.assertFileExists(PET_ASSET_SETTINGS_CPP)
        source = read(PET_ASSET_SETTINGS_CPP)

        self.assertRegex(
            source,
            r"\b(qBound|std::clamp|clamp)\s*(?:<[^>]+>)?\s*\(",
            "numeric values loaded from JSON should be clamped",
        )
        for token in (
            "resetToDefaults",
            "toneIndex",
            "responseLengthIndex",
            "languageIndex",
            "emotionLevel",
            "creativityLevel",
            "voiceSpeed",
            "lipSyncSensitivity",
        ):
            self.assertContains(source, token)

    def test_pet_asset_settings_does_not_embed_licensed_live2d_resources(self):
        checked_sources = {
            "PetAssetSettings.h": read(PET_ASSET_SETTINGS_H) if PET_ASSET_SETTINGS_H.exists() else "",
            "PetAssetSettings.cpp": read(PET_ASSET_SETTINGS_CPP) if PET_ASSET_SETTINGS_CPP.exists() else "",
            "Live2DCharacterPane.qml": read(CHARACTER_PANE_QML),
            "qml.qrc": read(QML_QRC),
        }

        for label, text in checked_sources.items():
            for token in FORBIDDEN_LICENSED_ASSET_TOKENS:
                self.assertNotIn(token, text, f"{label} should not embed {token!r}")

    def test_character_pane_owns_settings_store_and_loads_on_startup(self):
        qml = read(CHARACTER_PANE_QML)

        self.assertContains(qml, "import MemoChat 1.0")
        self.assertRegex(qml, r"\bPetAssetSettings\s*\{")
        self.assertRegex(qml, r"\.load\s*\(")
        self.assertContains(qml, "Component.onCompleted")

        for field in QML_EDITABLE_FIELDS:
            self.assertContains(
                qml,
                field,
                f"Live2DCharacterPane should expose editable field {field}",
            )

    def test_character_pane_persists_after_asset_validation(self):
        qml = read(CHARACTER_PANE_QML)
        save_body = function_body(qml, "saveDraft")

        save_index = save_body.find(".save(")
        self.assertNotEqual(save_index, -1, "saveDraft() should call PetAssetSettings.save()")

        validation_indexes = (
            save_body.find("runAssetValidation"),
            save_body.find("assetValidator.validate"),
        )
        self.assertTrue(
            any(0 <= index < save_index for index in validation_indexes),
            "saveDraft() should validate Live2D assets before calling save()",
        )

    def test_character_pane_reset_uses_settings_defaults_and_reapplies_to_ui(self):
        qml = read(CHARACTER_PANE_QML)
        reset_body = function_body(qml, "resetDraft")

        self.assertRegex(reset_body, r"\.resetToDefaults\s*\(")
        self.assertRegex(
            reset_body,
            r"(apply|root\.characterName\s*=)",
            "resetDraft() should re-apply PetAssetSettings defaults to editable UI fields",
        )


if __name__ == "__main__":
    unittest.main()
