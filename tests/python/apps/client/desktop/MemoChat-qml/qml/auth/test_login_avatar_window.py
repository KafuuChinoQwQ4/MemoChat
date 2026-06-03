import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
ICON_PATH_UTILS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/utils/IconPathUtils.h"
SESSION_AUTH_LOGIN_RESPONSE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/session/SessionAuthCoordinatorLoginResponse.cpp"
)
MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/app/Main.qml"
LOGIN_PAGE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view/LoginPage.qml"
LINUX_LOGIN_PAGE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/linux/LoginPage.qml"
LOGIN_TOP_BAR_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view/components/LoginTopBar.qml"
LOGIN_CREDENTIAL_RUNTIME = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view/components/LoginCredentialRuntime.js"
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
        self.assertIn("return attachMediaDownloadAuth(withGateMediaUrlPrefix(icon));", source)
        self.assertLess(
            source.index("isGateRelativeMediaDownloadUrl(icon)"),
            source.index("if (QDir::isAbsolutePath(icon))"),
        )

    def test_http_login_seeds_user_profile_icon_before_chat_login(self):
        source = SESSION_AUTH_LOGIN_RESPONSE.read_text(encoding="utf-8")
        body = extract_cpp_function(source, "void SessionAuthCoordinator::onLoginHttpFinished")

        self.assertIn(
            '_app.applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);', body
        )
        self.assertLess(
            body.index('_app.applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);'),
            body.index("_gateway.chatTransport()->connectToServer(server_info);"),
        )
        self.assertIn(
            "setIconDownloadAuthContext(_app._pending_login_state.uid, _app._pending_login_state.token);", body
        )

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

        self.assertTrue(LOGIN_CREDENTIAL_RUNTIME.is_file())
        self.assertIn("features/auth/view/components/LoginCredentialRuntime.js", qrc)
        self.assertIn('import "components/LoginCredentialRuntime.js" as LoginCredentialRuntime', login_page)
        self.assertIn(
            'import "qrc:/features/auth/view/components/LoginCredentialRuntime.js" as LoginCredentialRuntime',
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


if __name__ == "__main__":
    unittest.main()
