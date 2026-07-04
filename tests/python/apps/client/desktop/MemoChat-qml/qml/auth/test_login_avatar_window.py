import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
ICON_PATH_UTILS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/utils/IconPathUtils.h"
SESSION_AUTH_LOGIN_RESPONSE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/session/SessionAuthCoordinatorLoginResponse.cpp"
)
APP_CONTROLLER_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppController.cpp"
APP_SESSION_AUTH_PORT_BINDER = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/composition/AppSessionAuthPortBinder.cpp"
)
MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/app/Main.qml"
LOGIN_PAGE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view/LoginPage.qml"
LINUX_LOGIN_PAGE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/linux/LoginPage.qml"
LOGIN_TOP_BAR_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view/components/LoginTopBar.qml"
LOGIN_CREDENTIAL_RUNTIME = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/runtime/LoginCredentialRuntime.js"
)
QML_SHELL_QRC = REPO_ROOT / "apps/client/desktop/MemoChat-qml/resources/qrc/qml-shell.qrc"
AUTH_QRC = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/resources/auth.qrc"


def extract_cpp_function(source: str, signature: str) -> str:
    start = source.index(signature)
    open_brace = source.index("{", start)
    depth = 0
    for index in range(open_brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"Function body not found for {signature}")


def extract_qml_function(source: str, name: str) -> str:
    return extract_cpp_function(source, f"function {name}(")


class LoginAvatarWindowTests(unittest.TestCase):
    def test_gate_relative_media_download_url_is_not_treated_as_local_file(self):
        source = ICON_PATH_UTILS.read_text(encoding="utf-8")

        self.assertIn("isGateRelativeMediaDownloadUrl", source)
        self.assertIn("withGateMediaUrlPrefix", source)
        # normalizeRelativeMediaDownloadUrl now checks prefix is non-empty before
        # passing to attachMediaDownloadAuth (avatar-bug fix: return {} when unset).
        self.assertIn("return attachMediaDownloadAuth(full);", source)
        self.assertLess(
            source.index("isGateRelativeMediaDownloadUrl(icon)"),
            source.index("if (QDir::isAbsolutePath(icon))"),
        )

    def test_http_login_seeds_user_profile_icon_before_chat_login(self):
        source = SESSION_AUTH_LOGIN_RESPONSE.read_text(encoding="utf-8")
        app_controller = APP_CONTROLLER_CPP.read_text(encoding="utf-8")
        port_binder = APP_SESSION_AUTH_PORT_BINDER.read_text(encoding="utf-8")
        body = extract_cpp_function(source, "void SessionAuthCoordinator::onLoginHttpFinished")

        self.assertIn("_port.applyLoginSuccess(server_info, obj);", body)
        self.assertNotIn("_app.", body)
        self.assertIn(
            'applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);', port_binder
        )
        self.assertNotIn(
            'applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);', app_controller
        )
        self.assertLess(
            port_binder.index('applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);'),
            port_binder.index("_gateway.chatTransport()->connectToServer(serverInfo);"),
        )
        self.assertIn("const AppPendingLoginState& pending = _session_coordinator->pendingLoginState();", port_binder)
        self.assertIn("setIconDownloadAuthContext(pending.uid, pending.token);", port_binder)

    def test_login_and_chat_use_mutually_exclusive_windows_and_center_on_show(self):
        source = MAIN_QML.read_text(encoding="utf-8")
        sync_windows = extract_qml_function(source, "syncWindowsByPage")
        configure_login = extract_qml_function(source, "configureLoginWindow")
        configure_chat = extract_qml_function(source, "configureChatWindow")

        self.assertIn("visible: false", source)
        self.assertIn(
            "centerWindowForSize(win, Qt.size(win.width, win.height))", extract_qml_function(source, "centerWindow")
        )
        self.assertIn("property var loginWindowRef: null", source)
        self.assertIn("property var chatWindowRef: null", source)
        self.assertIn("function ensureLoginWindow()", source)
        self.assertIn("function ensureChatWindow()", source)
        self.assertIn("function destroyLoginWindow()", source)
        self.assertIn("function destroyChatWindow()", source)
        self.assertIn("sourceComponent: chatShellPageComponent", source)
        self.assertIn("sourceComponent: loginPageComponent", source)
        self.assertIn("destroyLoginWindow()", sync_windows)
        self.assertIn("destroyChatWindow()", sync_windows)
        self.assertIn("scheduleWindowHandoff", sync_windows)
        self.assertNotIn("showChatWindow()", sync_windows)
        self.assertNotIn("showLoginWindow()", sync_windows)
        self.assertIn("centerWindowForSize(win, root.loginWindowSize)", configure_login)
        self.assertIn("requestWindowCenter(win)", configure_login)
        self.assertIn("centerWindowForSize(win, root.chatWindowSize)", configure_chat)
        self.assertIn("requestWindowCenter(win)", configure_chat)
        self.assertNotIn("centerWindowWithRetry", source)
        self.assertNotIn("retryCenterTimer", source)
        self.assertNotIn("startupShowRetryTimer", source)
        self.assertNotIn("DragHandler {\n            target: null", source)

    def test_login_top_bar_owns_long_press_window_move(self):
        login_page = LOGIN_PAGE_QML.read_text(encoding="utf-8")
        top_bar = LOGIN_TOP_BAR_QML.read_text(encoding="utf-8")

        self.assertIn("import QtQuick.Window 2.15", login_page)
        self.assertIn("signal dragMoveRequested()", top_bar)
        self.assertIn("onPressed:", top_bar)
        self.assertIn("onDragMoveRequested:", login_page)
        self.assertIn("Window.window.startSystemMove()", login_page)

    def test_login_credential_runtime_owns_cache_helpers(self):
        qrc = QML_SHELL_QRC.read_text(encoding="utf-8") + "\n" + AUTH_QRC.read_text(encoding="utf-8")
        login_page = LOGIN_PAGE_QML.read_text(encoding="utf-8")
        linux_login_page = LINUX_LOGIN_PAGE_QML.read_text(encoding="utf-8")
        runtime = LOGIN_CREDENTIAL_RUNTIME.read_text(encoding="utf-8")

        self.assertTrue(LOGIN_CREDENTIAL_RUNTIME.is_file())
        self.assertIn("features/auth/runtime/LoginCredentialRuntime.js", qrc)
        self.assertIn('import "../runtime/LoginCredentialRuntime.js" as LoginCredentialRuntime', login_page)
        self.assertIn(
            'import "qrc:/features/auth/runtime/LoginCredentialRuntime.js" as LoginCredentialRuntime',
            linux_login_page,
        )

        for page in (login_page, linux_login_page):
            with self.subTest(page=page[:32]):
                self.assertIn("LoginCredentialRuntime.parseCredentialCache", page)
                self.assertIn("LoginCredentialRuntime.buildSavedCredentials", page)
                self.assertIn("LoginCredentialRuntime.credentialAt", page)
                self.assertNotIn("JSON.parse", page)
                self.assertNotIn(".filter(function(item)", page)
                self.assertNotIn("records.unshift", page)
                self.assertNotIn("records.slice(0, maxCachedCredentials)", page)

        self.assertNotIn("password", runtime)
        for page in (login_page, linux_login_page):
            with self.subTest(cache_contract_page=page[:32]):
                replace_model = extract_qml_function(page, "replaceCredentialModel")
                load_last = extract_qml_function(page, "loadLastCredential")
                save_credential = extract_qml_function(page, "saveCredential")
                apply_credential = extract_qml_function(page, "applyCredential")
                cache_helpers = "\n".join((replace_model, load_last, save_credential, apply_credential))

                self.assertIn('credentialModel.append({ "email": records[i].email })', replace_model)
                self.assertIn("LoginCredentialRuntime.normalizeCredential(email)", save_credential)
                self.assertIn("credentialProvider.saveLoginCredential(normalized.email", save_credential)
                for token in (
                    '"password"',
                    "record.password",
                    "normalized.password",
                    "pwdField.text = record",
                    "buildSavedCredentials(credentialCacheJson(), normalized.email, normalized.password",
                ):
                    self.assertNotIn(token, cache_helpers)


if __name__ == "__main__":
    unittest.main()
