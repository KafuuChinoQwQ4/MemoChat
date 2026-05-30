import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
ICON_PATH_UTILS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/utils/IconPathUtils.h"
SESSION_AUTH_LOGIN_RESPONSE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/SessionAuthCoordinatorLoginResponse.cpp"
MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/Main.qml"
LOGIN_PAGE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/LoginPage.qml"
LOGIN_TOP_BAR_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/components/LoginTopBar.qml"


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
    return extract_cpp_function(source, f"function {name}")


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

    def test_login_and_chat_share_single_window_and_center_on_page_sync(self):
        source = MAIN_QML.read_text(encoding="utf-8")
        sync_windows = extract_qml_function(source, "syncWindowsByPage")
        configure_window = extract_qml_function(source, "configureAppWindowForPage")

        self.assertIn("visible: false", source)
        self.assertIn("return true", extract_qml_function(source, "centerWindow"))
        self.assertIn("property var appWindowRef: null", source)
        self.assertIn("function ensureAppWindow()", source)
        self.assertIn("sourceComponent: chatShellPageComponent", source)
        self.assertIn("sourceComponent: loginPageComponent", source)
        self.assertIn("configureAppWindowForPage(win)", sync_windows)
        self.assertIn("showWindowCentered(win)", sync_windows)
        self.assertIn("centerWindowForSize(win, size)", configure_window)
        self.assertIn("requestWindowCenter(win)", configure_window)
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


if __name__ == "__main__":
    unittest.main()
