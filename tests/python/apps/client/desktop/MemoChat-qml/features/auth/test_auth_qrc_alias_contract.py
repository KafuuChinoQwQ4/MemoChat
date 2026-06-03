import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
AUTH_QRC = CLIENT / "features/auth/resources/auth.qrc"
RESOURCES = CLIENT / "resources/resources.cmake"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class AuthQrcAliasContractTests(unittest.TestCase):
    def test_auth_qrc_uses_feature_aliases_only(self):
        qrc = read(AUTH_QRC)
        feature_entries = (
            'alias="features/auth/view/LoginPage.qml">../view/LoginPage.qml</file>',
            'alias="features/auth/view/RegisterPage.qml">../view/RegisterPage.qml</file>',
            'alias="features/auth/view/ResetPage.qml">../view/ResetPage.qml</file>',
        )
        for entry in feature_entries:
            with self.subTest(entry=entry):
                self.assertIn(entry, qrc)

        legacy_aliases = (
            'alias="qml/LoginPage.qml"',
            'alias="qml/auth/LoginPage.qml"',
            'alias="qml/RegisterPage.qml"',
            'alias="qml/auth/RegisterPage.qml"',
            'alias="qml/ResetPage.qml"',
            'alias="qml/auth/ResetPage.qml"',
        )
        for alias in legacy_aliases:
            with self.subTest(alias=alias):
                self.assertNotIn(alias, qrc)

    def test_auth_qrc_is_in_resource_manifest(self):
        resources = read(RESOURCES)
        manifest = read(CLIENT / "features/auth/sources.cmake")
        self.assertIn("features/auth/resources/auth.qrc", manifest)
        self.assertIn("${MEMOCHAT_QML_FEATURE_RESOURCES}", resources)


if __name__ == "__main__":
    unittest.main()
