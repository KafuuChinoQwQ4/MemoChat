import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
AUTH = CLIENT / "features/auth"
AUTH_VIEW = AUTH / "view"
LEGACY_AUTH = CLIENT / "qml/auth"
AUTH_QML_FILES = ("LoginPage.qml", "RegisterPage.qml", "ResetPage.qml")


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class AuthFeatureContractTests(unittest.TestCase):
    def test_auth_feature_has_viewmodel_model_service_and_resources(self):
        expected = (
            AUTH / "model/AuthState.h",
            AUTH / "service/AuthService.h",
            AUTH / "service/AuthService.cpp",
            AUTH / "viewmodel/AuthViewModel.h",
            AUTH / "viewmodel/AuthViewModel.cpp",
            AUTH / "resources/auth.qrc",
            *(AUTH_VIEW / file_name for file_name in AUTH_QML_FILES),
        )
        for path in expected:
            with self.subTest(path=path):
                self.assertTrue(path.is_file())

    def test_legacy_physical_auth_pages_are_removed(self):
        for file_name in AUTH_QML_FILES:
            with self.subTest(file_name=file_name):
                self.assertFalse((LEGACY_AUTH / file_name).exists())

    def test_auth_viewmodel_exposes_legacy_auth_qml_surface(self):
        header = read(AUTH / "viewmodel/AuthViewModel.h")
        expected_tokens = (
            "Q_PROPERTY(QString tipText READ tipText NOTIFY",
            "Q_PROPERTY(bool tipError READ tipError NOTIFY",
            "Q_PROPERTY(bool busy READ busy NOTIFY",
            "Q_PROPERTY(bool registerSuccessPage READ registerSuccessPage NOTIFY",
            "Q_PROPERTY(int registerCountdown READ registerCountdown NOTIFY",
            "Q_PROPERTY(int registerCodeCooldownSeconds READ registerCodeCooldownSeconds NOTIFY",
            "Q_PROPERTY(bool registerCodeRequestPending READ registerCodeRequestPending NOTIFY",
            "Q_PROPERTY(QString loginCredentialCacheJson READ loginCredentialCacheJson NOTIFY",
            "Q_INVOKABLE void clearTip()",
            "Q_INVOKABLE void saveLoginCredential(const QString& email, const QString& password)",
            "Q_INVOKABLE void login(const QString& email, const QString& password)",
            "Q_INVOKABLE void requestRegisterCode(const QString& email)",
            "Q_INVOKABLE void registerUser(const QString& user,",
            "Q_INVOKABLE void requestResetCode(const QString& email)",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, header)

        normalized_header = " ".join(header.split())
        self.assertIn(
            "Q_INVOKABLE void resetPassword(const QString& user, const QString& email, "
            "const QString& password, const QString& verifyCode);",
            normalized_header,
        )

    def test_feature_manifest_lists_auth_viewmodel_service_and_state(self):
        manifest = read(AUTH / "sources.cmake")
        for token in (
            "features/auth/viewmodel/AuthViewModel.cpp",
            "features/auth/service/AuthService.cpp",
            "features/auth/viewmodel/AuthViewModel.h",
            "features/auth/service/AuthService.h",
            "features/auth/model/AuthState.h",
            "features/auth/resources/auth.qrc",
        ):
            with self.subTest(token=token):
                self.assertIn(token, manifest)


if __name__ == "__main__":
    unittest.main()
