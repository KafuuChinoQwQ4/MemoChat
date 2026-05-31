import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"

PET_ASSET_SETTINGS_H = CLIENT_DIR / "features/pet/assets/PetAssetSettings.h"
PET_ASSET_SETTINGS_CPP = CLIENT_DIR / "features/pet/assets/PetAssetSettings.cpp"
PET_ASSET_SETTINGS_PRIVATE_H = CLIENT_DIR / "features/pet/assets/PetAssetSettingsPrivate.h"
PET_ASSET_SETTINGS_AVATAR_CPP = CLIENT_DIR / "features/pet/assets/PetAssetSettingsAvatar.cpp"
PET_AVATAR_RESOLVER_H = CLIENT_DIR / "features/pet/assets/PetAvatarResolver.h"
PET_AVATAR_RESOLVER_CPP = CLIENT_DIR / "features/pet/assets/PetAvatarResolver.cpp"
PET_ASSET_SETTINGS_PERSISTENCE_CPP = CLIENT_DIR / "features/pet/assets/PetAssetSettingsPersistence.cpp"
PET_ASSET_SETTINGS_STATE_CPP = CLIENT_DIR / "features/pet/assets/PetAssetSettingsState.cpp"
PET_ASSET_SETTINGS_VOICE_TRAINING_CPP = CLIENT_DIR / "features/pet/assets/PetAssetSettingsVoiceTraining.cpp"
CLIENT_CMAKE = CLIENT_DIR / "CMakeLists.txt"
CLIENT_CMAKE_MANIFESTS = (
    CLIENT_DIR / "app/sources.cmake",
    CLIENT_DIR / "features/sources.cmake",
    CLIENT_DIR / "features/agent/sources.cmake",
    CLIENT_DIR / "features/auth/sources.cmake",
    CLIENT_DIR / "features/call/sources.cmake",
    CLIENT_DIR / "features/chat/sources.cmake",
    CLIENT_DIR / "features/contact/sources.cmake",
    CLIENT_DIR / "features/moments/sources.cmake",
    CLIENT_DIR / "features/pet/sources.cmake",
    CLIENT_DIR / "features/profile/sources.cmake",
    CLIENT_DIR / "features/r18/sources.cmake",
    CLIENT_DIR / "shared/sources.cmake",
    CLIENT_DIR / "live2d/sources.cmake",
    CLIENT_DIR / "resources/resources.cmake",
)
MAIN_CPP = CLIENT_DIR / "app/bootstrap/main.cpp"
MAIN_QML_TYPE_REGISTRY_CPP = CLIENT_DIR / "app/bootstrap/MainQmlTypeRegistry.cpp"
CHARACTER_PANE_QML = CLIENT_DIR / "qml/pet/Live2DCharacterPane.qml"
RESOURCE_VOICE_PANEL_QML = CLIENT_DIR / "qml/pet/Live2DResourceVoicePanel.qml"
QRC_ROOT = CLIENT_DIR / "resources/qrc"
QML_QRCS = tuple(sorted(QRC_ROOT.glob("*.qrc")))

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
    "autoStartPetOnClientStart",
    "voiceTrainingConsent",
    "voiceTrainingConsentScope",
    "voiceTrainingJobId",
    "voiceTrainingStatus",
    "voiceTrainingStage",
    "voiceTrainingProgress",
    "voiceTrainingArtifactPath",
    "voiceTrainingMessage",
    "storagePath",
    "dirty",
    "statusText",
)

PERSISTED_DRAFT_FIELDS = tuple(
    prop for prop in REQUIRED_PROPERTIES if prop not in {"storagePath", "dirty", "statusText"}
)

REQUIRED_INVOKABLES = (
    "load",
    "save",
    "resetToDefaults",
    "toVariantMap",
    "pickLocalFilePath",
    "pickLocalDirectoryPath",
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
    "voiceTrainingConsent",
    "voiceTrainingConsentScope",
    "voiceTrainingJobId",
    "voiceTrainingStatus",
    "voiceTrainingStage",
    "voiceTrainingProgress",
    "voiceTrainingArtifactPath",
    "voiceTrainingMessage",
    "autoStartPetOnClientStart",
)

FORBIDDEN_LICENSED_ASSET_TOKENS = (
    "qrc:/src/KafuuChino",
    "<file>src/KafuuChino",
)

DEFAULT_LIVE2D_RESOURCE_ROOT = "resources/live2d/KafuuChino"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def qrc_text() -> str:
    return "\n".join(read(path) for path in QML_QRCS)


def read_texts(*paths: Path) -> str:
    return "\n".join(read(path) for path in paths)


def client_cmake_text() -> str:
    return read_texts(CLIENT_CMAKE, *(manifest for manifest in CLIENT_CMAKE_MANIFESTS if manifest.exists()))


def pet_asset_settings_source() -> str:
    return read_texts(
        PET_ASSET_SETTINGS_CPP,
        PET_ASSET_SETTINGS_PRIVATE_H,
        PET_ASSET_SETTINGS_AVATAR_CPP,
        PET_AVATAR_RESOLVER_H,
        PET_AVATAR_RESOLVER_CPP,
        PET_ASSET_SETTINGS_PERSISTENCE_CPP,
        PET_ASSET_SETTINGS_STATE_CPP,
        PET_ASSET_SETTINGS_VOICE_TRAINING_CPP,
    )


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
                return source[start + 1 : index]
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
        self.assertFileExists(PET_ASSET_SETTINGS_PRIVATE_H)
        self.assertFileExists(PET_ASSET_SETTINGS_AVATAR_CPP)
        self.assertFileExists(PET_AVATAR_RESOLVER_H)
        self.assertFileExists(PET_AVATAR_RESOLVER_CPP)
        self.assertFileExists(PET_ASSET_SETTINGS_PERSISTENCE_CPP)
        self.assertFileExists(PET_ASSET_SETTINGS_STATE_CPP)
        self.assertFileExists(PET_ASSET_SETTINGS_VOICE_TRAINING_CPP)

        cmake = client_cmake_text()
        self.assertRegex(cmake, r"\bPetAssetSettings\.cpp\b")
        self.assertRegex(cmake, r"\bPetAssetSettingsAvatar\.cpp\b")
        self.assertRegex(cmake, r"\bPetAvatarResolver\.cpp\b")
        self.assertRegex(cmake, r"\bPetAssetSettingsPersistence\.cpp\b")
        self.assertRegex(cmake, r"\bPetAssetSettingsState\.cpp\b")
        self.assertRegex(cmake, r"\bPetAssetSettingsVoiceTraining\.cpp\b")
        self.assertRegex(cmake, r"\bPetAssetSettings\.h\b")
        self.assertRegex(cmake, r"\bPetAssetSettingsPrivate\.h\b")
        self.assertRegex(cmake, r"\bPetAvatarResolver\.h\b")

    def test_pet_asset_settings_large_concerns_are_split(self):
        main = read(PET_ASSET_SETTINGS_CPP)
        avatar = read(PET_ASSET_SETTINGS_AVATAR_CPP)
        avatar_resolver = read(PET_AVATAR_RESOLVER_CPP)
        persistence = read(PET_ASSET_SETTINGS_PERSISTENCE_CPP)
        state = read(PET_ASSET_SETTINGS_STATE_CPP)
        voice_training = read(PET_ASSET_SETTINGS_VOICE_TRAINING_CPP)

        self.assertIn("QString PetAssetSettings::resolveLive2DAvatarUrl", avatar)
        self.assertIn("QString resolveLive2DAvatarCacheUrl", avatar_resolver)
        self.assertIn("bool PetAssetSettings::load()", persistence)
        self.assertIn("QVariantMap PetAssetSettings::toVariantMap() const", persistence)
        self.assertIn("void PetAssetSettings::applyDefaults", state)
        self.assertIn("void PetAssetSettings::setCharacterName", state)
        self.assertIn("void PetAssetSettings::setVoiceTrainingConsent", voice_training)
        self.assertIn("void PetAssetSettings::normalizeVoiceTrainingState", voice_training)
        self.assertNotIn("resolveLive2DAvatarUrl", main)
        self.assertNotIn("modelTexturePaths", avatar)
        self.assertNotIn("packageImageCandidates", avatar)
        self.assertNotIn("QVariantMap PetAssetSettings::toVariantMap", main)
        self.assertNotIn("normalizeVoiceTrainingState", state)
        self.assertLess(len(main.splitlines()), 120)

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
        main = read(MAIN_CPP) + "\n" + read(MAIN_QML_TYPE_REGISTRY_CPP)

        self.assertContains(main, '#include "PetAssetSettings.h"')
        self.assertRegex(
            main,
            r"qmlRegisterType\s*<\s*PetAssetSettings\s*>\s*\(\s*\"MemoChat\"\s*,\s*1\s*,\s*0\s*,\s*\"PetAssetSettings\"\s*\)",
        )

    def test_pet_asset_settings_uses_app_data_json_storage(self):
        self.assertFileExists(PET_ASSET_SETTINGS_CPP)
        source = pet_asset_settings_source()

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
        source = pet_asset_settings_source()

        for field in PERSISTED_DRAFT_FIELDS:
            self.assertContains(
                source,
                field,
                f"PetAssetSettings persistence should mention draft field {field}",
            )

        self.assertContains(source, "toVariantMap")
        self.assertContains(source, "schema_version")

    def test_pet_asset_settings_defaults_client_pet_autostart_off_and_exposes_pickers(self):
        header = read(PET_ASSET_SETTINGS_H)
        source = pet_asset_settings_source()

        self.assertRegex(source, r"_auto_start_pet_on_client_start\s*=\s*false")
        self.assertRegex(source, r"boolValue\s*\(\s*values\s*,\s*QStringLiteral\(\"autoStartPetOnClientStart\"\)")
        self.assertIn('values[QStringLiteral("autoStartPetOnClientStart")]', source)

        for token in (
            "QFileDialog",
            "QFileInfo",
            "pickLocalFilePath",
            "pickLocalDirectoryPath",
            "QFileDialog::getOpenFileName",
            "QFileDialog::getExistingDirectory",
        ):
            self.assertContains(source, token)

        self.assertRegex(header, r"Q_INVOKABLE\s+QString\s+pickLocalFilePath\s*\(")
        self.assertRegex(header, r"Q_INVOKABLE\s+QString\s+pickLocalDirectoryPath\s*\(")

    def test_pet_asset_settings_clamps_invalid_json_to_safe_defaults(self):
        self.assertFileExists(PET_ASSET_SETTINGS_CPP)
        source = pet_asset_settings_source()

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

    def test_pet_asset_settings_discards_stale_blocked_voice_training_jobs_on_load(self):
        source = pet_asset_settings_source()

        for token in (
            "reference_not_visible",
            "reference_unreadable",
            "reference_too_short",
            "_voice_training_job_id.clear()",
            '_voice_training_status = QStringLiteral("idle")',
            "_voice_training_stage.clear()",
            "_voice_training_progress = 0",
            "_voice_training_artifact_path.clear()",
            '_voice_training_message = QStringLiteral("等待确认参考音频权限")',
        ):
            self.assertContains(source, token)

    def test_pet_asset_settings_defaults_to_user_requested_src_character_assets(self):
        source = pet_asset_settings_source()
        qml = read(CHARACTER_PANE_QML)
        cmake = client_cmake_text()

        for token in (
            "MEMOCHAT_QML_SOURCE_DIR",
            f"{DEFAULT_LIVE2D_RESOURCE_ROOT}/香风智乃live2D",
            f"{DEFAULT_LIVE2D_RESOURCE_ROOT}/香风智乃live2D/香风智乃.model3.json",
            f"{DEFAULT_LIVE2D_RESOURCE_ROOT}/香风智乃voice",
            "Kafuuchino-voice.mp3",
        ):
            self.assertContains(source, token)

        for token in (
            f"{DEFAULT_LIVE2D_RESOURCE_ROOT}/香风智乃live2D",
            f"{DEFAULT_LIVE2D_RESOURCE_ROOT}/香风智乃voice",
            "Kafuuchino-voice.mp3",
        ):
            self.assertContains(qml, token)

        self.assertNotIn("src/KafuuChino", source)
        self.assertNotIn("src/KafuuChino", qml)

        self.assertContains(cmake, "MEMOCHAT_QML_SOURCE_DIR")

    def test_pet_asset_settings_migrates_legacy_bundled_live2d_paths_on_load(self):
        source = pet_asset_settings_source()
        persistence = read(PET_ASSET_SETTINGS_PERSISTENCE_CPP)

        for token in (
            "migrateLegacyBundledLive2DPaths",
            "migrateLegacyBundledLive2DPath",
            "resources/live2d/KafuuChino",
            "modelRoot",
            "modelJson",
            "motionDirectory",
            "expressionDirectory",
            "voiceDirectory",
        ):
            self.assertContains(source, token)

        self.assertRegex(source, r"QDir::cleanPath\s*\(")
        self.assertRegex(source, r"\.startsWith\s*\(")
        self.assertRegex(
            source,
            r"applyObject\s*\([^;]+;\s*const\s+bool\s+migrated\s*=\s*migrateLegacyBundledLive2DPaths\s*\(",
            "Loaded drafts should be migrated immediately after JSON values are applied",
        )
        self.assertRegex(
            persistence,
            r"if\s*\(\s*migrated\s*\)\s*\{[\s\S]*\bsave\s*\(",
            "Migrated drafts should be persisted so stale deleted paths do not return next launch",
        )

    def test_pet_asset_settings_does_not_bundle_licensed_live2d_resources(self):
        checked_sources = {
            "resources/qrc/*.qrc": qrc_text(),
            "CMakeLists.txt": client_cmake_text(),
        }

        for label, text in checked_sources.items():
            for token in FORBIDDEN_LICENSED_ASSET_TOKENS:
                self.assertNotIn(token, text, f"{label} should not embed {token!r}")

    def test_character_pane_language_options_are_single_language_only(self):
        qml = read(CHARACTER_PANE_QML)
        source = pet_asset_settings_source()

        self.assertIn('model: ["中文", "日语", "英语", "韩语", "法语", "西班牙语"]', qml)
        self.assertIn("kLanguageOptionCount - 1", source)
        for mixed in ("中文优先", "中日混合", "中英双语"):
            self.assertNotIn(mixed, qml)

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

    def test_character_pane_exposes_safe_voice_training_controls(self):
        qml = read(CHARACTER_PANE_QML) + "\n" + read(RESOURCE_VOICE_PANEL_QML)

        for token in (
            "voiceTrainingConsent",
            "voiceTrainingConsentScope",
            "voiceTrainingJobId",
            "voiceTrainingStatus",
            "voiceTrainingMessage",
            "function defaultVoicePath",
            "function startVoiceTraining",
            "consent_confirmed",
            "reference_audio_path",
            "reference_audio_directory",
            "reference_audio_file",
            "startVoiceTraining({",
            "refreshVoiceTraining",
            "voiceTrainingProgress",
            "voiceTrainingArtifactPath",
            "开始声音训练",
            "允许使用参考音频",
            "准备包 ",
            "src-default",
            "gpt-sovits",
        ):
            self.assertContains(qml, token)

        self.assertContains(qml, f"{DEFAULT_LIVE2D_RESOURCE_ROOT}/香风智乃voice")
        self.assertContains(qml, "Kafuuchino-voice.mp3")
        self.assertNotIn("?.", qml, "QML should avoid optional chaining for Qt 6.8 compatibility")


if __name__ == "__main__":
    unittest.main()
