import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
AUTH = CLIENT / "features/auth"
AUTH_VIEW = AUTH / "view"
LEGACY_AUTH = CLIENT / "qml/auth"
GLOBAL_HEADER = CLIENT / "core/common/global.h"
PENDING_LOGIN_STATE = CLIENT / "app/connection/AppControllerConnectionState.h"
SESSION_LOGIN_RESPONSE = CLIENT / "app/session/SessionAuthCoordinatorLoginResponse.cpp"
SESSION_CONNECTION_STATE = CLIENT / "app/session/AppSessionCoordinatorConnectionState.cpp"
AUTH_QML_FILES = ("LoginPage.qml", "RegisterPage.qml", "ResetPage.qml")
AUTH_SOURCE_SUFFIXES = {".cpp", ".h", ".qml", ".js"}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class AuthFeatureContractTests(unittest.TestCase):
    def test_auth_feature_has_viewmodel_model_service_and_resources(self):
        expected = (
            AUTH / "model/AuthState.h",
            AUTH / "service/AuthCredentialStore.h",
            AUTH / "service/AuthCredentialStore.cpp",
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
            "struct AuthCommandPort",
            "void setCommandPort(AuthCommandPort port)",
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
            "features/auth/service/AuthCredentialStore.cpp",
            "features/auth/service/AuthService.cpp",
            "features/auth/viewmodel/AuthViewModel.h",
            "features/auth/service/AuthCredentialStore.h",
            "features/auth/service/AuthService.h",
            "features/auth/model/AuthState.h",
            "features/auth/resources/auth.qrc",
        ):
            with self.subTest(token=token):
                self.assertIn(token, manifest)

    def test_auth_feature_does_not_depend_on_app_controller_or_countdown_coordinator(self):
        for path in sorted(AUTH.rglob("*")):
            if path.suffix not in AUTH_SOURCE_SUFFIXES:
                continue

            source = read(path)
            with self.subTest(path=path.relative_to(CLIENT)):
                self.assertNotIn("AppController", source)
                self.assertNotIn("RegisterCountdownController", source)
                self.assertNotIn("RegisterCountdownPort", source)

    def test_auth_viewmodel_owns_command_loop_through_command_port(self):
        header = read(AUTH / "viewmodel/AuthViewModel.h")
        source = read(AUTH / "viewmodel/AuthViewModel.cpp")
        store_header = read(AUTH / "service/AuthCredentialStore.h")
        store_source = read(AUTH / "service/AuthCredentialStore.cpp")

        self.assertIn("struct AuthCommandPort", header)
        self.assertIn("AuthCommandPort _command_port;", header)
        self.assertIn("AuthCredentialStore _credentialStore;", header)
        self.assertIn("void AuthViewModel::setCommandPort(AuthCommandPort port)", source)
        for token in (
            "_command_port.clearTip();",
            "_command_port.login(email, password);",
            "_command_port.requestRegisterCode(email);",
            "_command_port.registerUser(user, email, password, confirm, verifyCode);",
            "_command_port.requestResetCode(email);",
            "_command_port.resetPassword(user, email, password, verifyCode);",
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

        self.assertNotIn("saveLoginCredential;", header)
        self.assertIn("Q_UNUSED(password)", source)
        self.assertIn("_credentialStore.saveLoginCredential(email, QString());", source)
        self.assertIn("syncLoginCredentialCacheJson(_credentialStore.credentialCacheJson());", source)

        for legacy_signal in (
            "clearTipRequested",
            "saveLoginCredentialRequested",
            "loginRequested",
            "registerCodeRequested",
            "registerUserRequested",
            "resetCodeRequested",
            "resetPasswordRequested",
        ):
            with self.subTest(signal=legacy_signal):
                self.assertNotIn(legacy_signal, header)
                self.assertNotIn(legacy_signal, source)

        self.assertIn("class AuthCredentialStore", store_header)
        self.assertIn("QString credentialCacheJson() const;", store_header)
        self.assertIn("void saveLoginCredential(const QString& email, const QString& password) const;", store_header)
        store_implementation = store_header + "\n" + store_source
        self.assertIn("LoginCredentialCache", store_implementation)
        self.assertIn("kMaxLoginCredentialCache", store_implementation)
        for token in (
            'QStringLiteral("password")',
            'obj.value(QStringLiteral("password"))',
            "password.isEmpty()",
            '{QStringLiteral("email"), normalizedEmail}, {QStringLiteral("password")',
        ):
            with self.subTest(forbidden_cache_password_token=token):
                self.assertNotIn(token, store_implementation)
        self.assertIn('sanitized.append(QJsonObject{{QStringLiteral("email"), itemEmail}})', store_implementation)
        self.assertIn('next.append(QJsonObject{{QStringLiteral("email"), normalizedEmail}})', store_implementation)

    def test_auth_credential_store_never_persists_tokens_or_passwords(self):
        store = read(AUTH / "service/AuthCredentialStore.h")
        forbidden_tokens = (
            "refresh_token",
            "RefreshToken",
            "access_token",
            "login_ticket",
            "LoginTicket",
            "token_hash",
            "SetToken",
            'QStringLiteral("password")',
            'QStringLiteral("token")',
        )
        for token in forbidden_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, store)

        self.assertIn('QJsonObject{{QStringLiteral("email"), itemEmail}}', store)
        self.assertIn('QJsonObject{{QStringLiteral("email"), normalizedEmail}}', store)

    def test_refresh_token_is_login_response_memory_only_state(self):
        global_header = read(GLOBAL_HEADER)
        pending_header = read(PENDING_LOGIN_STATE)
        login_response = read(SESSION_LOGIN_RESPONSE)
        connection_state = read(SESSION_CONNECTION_STATE)

        self.assertIn("QString RefreshToken;", global_header)
        self.assertIn("QString refreshToken;", pending_header)
        self.assertIn('obj.value("refresh_token").toString()', login_response)
        self.assertIn("server_info.RefreshToken.trimmed().isEmpty()", login_response)
        self.assertIn("_pending_login_state.refreshToken = serverInfo.RefreshToken;", connection_state)
        self.assertGreaterEqual(connection_state.count("_pending_login_state.refreshToken.clear();"), 2)

        for source in (login_response, connection_state, pending_header):
            with self.subTest(source_hash=hash(source)):
                self.assertNotIn("QSettings", source)
                self.assertNotIn("AuthCredentialStore", source)
        self.assertNotIn('qWarning() << "Invalid chat server response:" << obj', login_response)
        self.assertIn("hasRefreshToken", login_response)


if __name__ == "__main__":
    unittest.main()
