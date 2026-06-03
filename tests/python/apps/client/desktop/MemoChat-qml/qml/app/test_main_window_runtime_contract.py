import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
QML_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
SHARED_MAIN = QML_DIR / "qml/app/Main.qml"
LINUX_MAIN = QML_DIR / "qml/linux/Main.qml"
RUNTIME_JS = QML_DIR / "qml/app/AppWindowRuntime.js"
QML_QRC = QML_DIR / "resources/qrc/qml-shell.qrc"


class MainWindowRuntimeContractTests(unittest.TestCase):
    def test_runtime_helper_exists_and_is_registered(self):
        self.assertTrue(RUNTIME_JS.is_file())

        runtime_text = RUNTIME_JS.read_text(encoding="utf-8")
        for function_name in (
            "availableScreenGeometry",
            "centerPointForSize",
            "clampedWindowPosition",
        ):
            with self.subTest(function=function_name):
                self.assertRegex(runtime_text, rf"function\s+{function_name}\s*\(")

        qrc_text = QML_QRC.read_text(encoding="utf-8")
        self.assertIn('alias="qml/AppWindowRuntime.js"', qrc_text)
        self.assertIn('alias="qml/app/AppWindowRuntime.js"', qrc_text)

    def test_main_files_import_and_delegate_pure_runtime_logic(self):
        for qml_path in (SHARED_MAIN, LINUX_MAIN):
            text = qml_path.read_text(encoding="utf-8")
            with self.subTest(file=qml_path.name):
                self.assertRegex(text, r'import\s+"(?:(?:\.\./)?app/)?AppWindowRuntime\.js"\s+as AppWindowRuntime')
                self.assertIn("AppWindowRuntime.clampedWindowPosition", text)
                self.assertIn("function configureLoginWindow", text)
                self.assertIn("function configureChatWindow", text)

    def test_main_files_import_auth_from_feature_view(self):
        shared_text = SHARED_MAIN.read_text(encoding="utf-8")
        linux_text = LINUX_MAIN.read_text(encoding="utf-8")

        self.assertIn('import "qrc:/features/auth/view"', shared_text)
        self.assertIn('import "qrc:/features/auth/view" as SharedAuth', linux_text)
        self.assertNotIn('import "../auth"', shared_text)
        self.assertNotIn('import "../auth" as SharedAuth', linux_text)

    def test_centering_retry_and_platform_shell_are_preserved(self):
        for qml_path in (SHARED_MAIN, LINUX_MAIN):
            text = qml_path.read_text(encoding="utf-8")
            with self.subTest(file=qml_path.name):
                self.assertIn("pendingCenterPasses = 4", text)

        linux_text = LINUX_MAIN.read_text(encoding="utf-8")
        self.assertIn("WindowGlassShell", linux_text)

    def test_top_level_geometry_is_not_page_bound(self):
        page_bound_patterns = (
            r"\b(?:x|y|width|height)\s*:\s*root\.chatPageActive",
            r"\b(?:x|y|width|height)\s*:\s*root\.targetWindowSize\s*\(",
            r"\b(?:x|y|width|height)\s*:\s*shell\.page",
        )
        for qml_path in (SHARED_MAIN, LINUX_MAIN):
            direct_window_geometry = "\n".join(
                line.strip()
                for line in qml_path.read_text(encoding="utf-8").splitlines()
                if re.match(r"^\s{12}(?:x|y|width|height)\s*:", line)
            )
            for pattern in page_bound_patterns:
                with self.subTest(file=qml_path.name, pattern=pattern):
                    self.assertIsNone(re.search(pattern, direct_window_geometry))

    def test_login_chat_handoff_uses_mutually_exclusive_top_level_windows(self):
        for qml_path in (SHARED_MAIN, LINUX_MAIN):
            text = qml_path.read_text(encoding="utf-8")
            with self.subTest(file=qml_path.name):
                self.assertIn("property var loginWindowRef: null", text)
                self.assertIn("property var chatWindowRef: null", text)
                self.assertIn("function retireWindow", text)
                self.assertIn("function destroyLoginWindow", text)
                self.assertIn("function destroyChatWindow", text)
                sync_start = text.index("function syncWindowsByPage()")
                sync_body = text[sync_start : text.index("Component.onCompleted", sync_start)]
                self.assertIn("const token = ++windowSwitchToken", sync_body)
                self.assertIn("destroyLoginWindow()", sync_body)
                self.assertIn("destroyChatWindow()", sync_body)
                self.assertIn("scheduleWindowHandoff", sync_body)
                self.assertNotIn("Qt.callLater(function() {", sync_body)
                self.assertNotIn("showChatWindow()", sync_body)
                self.assertNotIn("showLoginWindow()", sync_body)
                self.assertLess(sync_body.index("destroyLoginWindow()"), sync_body.index("scheduleWindowHandoff"))
                self.assertLess(sync_body.index("destroyChatWindow()"), sync_body.rindex("scheduleWindowHandoff"))

                for page in ("LoginPage", "RegisterPage", "ResetPage"):
                    self.assertIn(f"shell.page === AppController.{page}", text)

                self.assertNotIn("property int displayedPage", text)
                self.assertNotIn("readonly property int noDisplayedPage: -1", text)
                self.assertNotIn("active: root.chatPageActive", text)


if __name__ == "__main__":
    unittest.main()
