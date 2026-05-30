import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"

LIVE2D_ASSET_H = CLIENT_DIR / "live2d/Live2DAsset.h"
LIVE2D_ASSET_CPP = CLIENT_DIR / "live2d/Live2DAsset.cpp"
CLIENT_CMAKE = CLIENT_DIR / "CMakeLists.txt"
MAIN_QML_TYPE_REGISTRY_CPP = CLIENT_DIR / "app/MainQmlTypeRegistry.cpp"
CHARACTER_PANE_QML = CLIENT_DIR / "qml/pet/Live2DCharacterPane.qml"
QML_QRC = CLIENT_DIR / "qml.qrc"
LIVE2D_FIXTURE_DIR = REPO_ROOT / "tests/fixtures/live2d/minimal_model"
LIVE2D_FIXTURE_MODEL = LIVE2D_FIXTURE_DIR / "minimal.model3.json"
LIVE2D_POLICY_DOC = CLIENT_DIR / "docs/live2d-desktop-pet-assets.md"

REQUIRED_PROPERTIES = (
    "modelRoot",
    "modelJson",
    "motionDirectory",
    "expressionDirectory",
    "voiceDirectory",
    "physicsFile",
    "poseFile",
    "userDataFile",
    "vtubeMappingFile",
    "packageChecksum",
    "valid",
    "status",
    "statusText",
    "errors",
    "warnings",
    "motionCount",
    "expressionCount",
    "textureCount",
    "voiceCount",
    "referencedFileCount",
)

MINIMAL_MODEL_FILES = (
    "minimal.model3.json",
    "minimal.moc3",
    "textures/minimal_texture_00.png",
    "motions/minimal_idle.motion3.json",
    "expressions/minimal_smile.exp3.json",
    "minimal.physics3.json",
    "minimal.pose3.json",
    "minimal.userdata3.json",
    "minimal.vtube.json",
    "voice/minimal_voice.wav",
)

MINIMAL_MODEL_JSON_FILES = (
    "minimal.model3.json",
    "motions/minimal_idle.motion3.json",
    "expressions/minimal_smile.exp3.json",
    "minimal.physics3.json",
    "minimal.pose3.json",
    "minimal.userdata3.json",
    "minimal.vtube.json",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class Live2DAssetContractTests(unittest.TestCase):
    def assertFileExists(self, path: Path) -> None:
        self.assertTrue(path.exists(), f"{path.relative_to(REPO_ROOT)} should exist")

    def assertContains(self, text: str, needle: str, message: str | None = None) -> None:
        if needle not in text:
            self.fail(message or f"Expected source to contain {needle!r}")

    def test_live2d_asset_files_exist_and_are_added_to_client_cmake(self):
        self.assertFileExists(LIVE2D_ASSET_H)
        self.assertFileExists(LIVE2D_ASSET_CPP)

        cmake = read(CLIENT_CMAKE)
        self.assertRegex(cmake, r"\bLive2DAsset\.cpp\b")
        self.assertRegex(cmake, r"\bLive2DAsset\.h\b")

    def test_live2d_asset_qobject_contract_is_exposed(self):
        self.assertFileExists(LIVE2D_ASSET_H)
        header = read(LIVE2D_ASSET_H)

        self.assertContains(header, "class Live2DAsset")
        self.assertContains(header, "Q_OBJECT")
        for prop in REQUIRED_PROPERTIES:
            self.assertRegex(
                header,
                rf"Q_PROPERTY\s*\([^)]*\b{re.escape(prop)}\b[^)]*\)",
                f"Live2DAsset should expose Q_PROPERTY {prop}",
            )

        self.assertRegex(header, r"Q_INVOKABLE\s+[^\n;]*\bvalidate\s*\(")
        self.assertRegex(header, r"Q_INVOKABLE\s+[^\n;]*\bclear\s*\(")

    def test_live2d_asset_uses_qt_json_model3_validation_apis(self):
        self.assertFileExists(LIVE2D_ASSET_CPP)
        source = read(LIVE2D_ASSET_CPP)

        for token in ("QJsonDocument", "QJsonObject", "QJsonArray"):
            self.assertContains(source, token)

        for token in (
            'QStringLiteral("FileReferences")',
            'QStringLiteral("Moc")',
            'QStringLiteral("Textures")',
            'QStringLiteral("Motions")',
            'QStringLiteral("Expressions")',
            'QStringLiteral("Physics")',
            'QStringLiteral("Pose")',
            'QStringLiteral("UserData")',
        ):
            self.assertContains(source, token)

        self.assertContains(source, ".model3.json")
        self.assertContains(source, ".vtube.json")
        self.assertRegex(source, r"QJsonDocument::fromJson\s*\(")

    def test_live2d_asset_computes_deterministic_sha256_package_checksum(self):
        self.assertFileExists(LIVE2D_ASSET_CPP)
        source = read(LIVE2D_ASSET_CPP)

        self.assertContains(source, "QCryptographicHash")
        self.assertRegex(source, r"QCryptographicHash\b[^\n;]*\bSha256\b")
        self.assertContains(source, "packageChecksum")
        self.assertContains(source, "referencedFileCount")
        self.assertRegex(source, r"\.toHex\s*\(")

    def test_main_registers_live2d_asset_in_memo_chat_module(self):
        qml_registry = read(MAIN_QML_TYPE_REGISTRY_CPP)

        self.assertContains(qml_registry, '#include "Live2DAsset.h"')
        self.assertRegex(
            qml_registry,
            r"qmlRegisterType\s*<\s*Live2DAsset\s*>\s*\(\s*\"MemoChat\"\s*,\s*1\s*,\s*0\s*,\s*\"Live2DAsset\"\s*\)",
        )

    def test_character_pane_binds_asset_validator_and_surfaces_feedback(self):
        qml = read(CHARACTER_PANE_QML)

        self.assertContains(qml, "import MemoChat 1.0")
        self.assertRegex(qml, r"\bLive2DAsset\s*\{")

        for prop in (
            "modelRoot",
            "modelJson",
            "motionDirectory",
            "expressionDirectory",
            "voiceDirectory",
        ):
            self.assertRegex(
                qml,
                rf"\b{prop}\s*:\s*root\.{prop}\b",
                f"Live2DAsset should bind {prop} to the editable root field",
            )

        self.assertRegex(qml, r"\.validate\s*\(")
        for feedback_token in (
            "status",
            "statusText",
            "motionCount",
            "expressionCount",
            "textureCount",
            "voiceCount",
            "packageChecksum",
            "referencedFileCount",
            "physicsFile",
            "poseFile",
            "userDataFile",
            "vtubeMappingFile",
            "errors",
            "warnings",
        ):
            self.assertContains(qml, feedback_token)

    def test_minimal_live2d_fixture_package_is_repo_owned_and_parseable(self):
        self.assertFileExists(LIVE2D_FIXTURE_MODEL)

        for relative_path in MINIMAL_MODEL_FILES:
            self.assertFileExists(LIVE2D_FIXTURE_DIR / relative_path)

        import json

        for relative_path in MINIMAL_MODEL_JSON_FILES:
            try:
                json.loads(read(LIVE2D_FIXTURE_DIR / relative_path))
            except json.JSONDecodeError as exc:
                self.fail(f"{relative_path} should contain valid JSON: {exc}")

        model = json.loads(read(LIVE2D_FIXTURE_MODEL))
        refs = model.get("FileReferences", {})
        self.assertEqual(refs.get("Moc"), "minimal.moc3")
        self.assertEqual(refs.get("Textures"), ["textures/minimal_texture_00.png"])
        self.assertEqual(refs.get("Physics"), "minimal.physics3.json")
        self.assertEqual(refs.get("Pose"), "minimal.pose3.json")
        self.assertEqual(refs.get("UserData"), "minimal.userdata3.json")
        self.assertEqual(
            refs.get("Motions", {}).get("Idle", [{}])[0].get("File"),
            "motions/minimal_idle.motion3.json",
        )
        self.assertEqual(
            refs.get("Expressions", [{}])[0].get("File"),
            "expressions/minimal_smile.exp3.json",
        )

    def test_qrc_keeps_pet_qml_and_does_not_bundle_live2d_model_assets(self):
        qrc = read(QML_QRC)

        for token in (
            "qml/pet/PetWindow.qml",
            "qml/pet/PetScene.qml",
            "qml/pet/Live2DCharacterPane.qml",
        ):
            self.assertContains(qrc, token)

        forbidden_asset_patterns = (
            r"\.model3\.json\b",
            r"\.moc3\b",
            r"\.motion3\.json\b",
            r"\.exp3\.json\b",
            r"\.physics3\.json\b",
            r"\.pose3\.json\b",
            r"\.userdata3\.json\b",
            r"\.vtube\.json\b",
            r"\.wav\b",
            r"(?i)KafuuChino",
            "香风智乃live2D",
        )
        for pattern in forbidden_asset_patterns:
            self.assertNotRegex(qrc, pattern)

    def test_live2d_policy_keeps_licensed_assets_out_of_repo_defaults(self):
        doc = read(LIVE2D_POLICY_DOC)
        cmake = read(CLIENT_CMAKE)

        self.assertIn("Repo-owned test fixtures", doc)
        self.assertIn("tests/fixtures/live2d", doc)
        self.assertIn("Do not use `D:` except for legacy Windows checks", doc)
        self.assertIn("MEMOCHAT_ENABLE_LIVE2D_NATIVE", cmake)
        self.assertNotIn("src/KafuuChino", cmake)
        self.assertNotIn("香风智乃live2D", cmake)


if __name__ == "__main__":
    unittest.main()
