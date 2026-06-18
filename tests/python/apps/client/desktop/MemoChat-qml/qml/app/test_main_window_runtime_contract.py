import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
QML_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
SHARED_MAIN = QML_DIR / "qml/app/Main.qml"
LINUX_MAIN = QML_DIR / "qml/linux/Main.qml"
RUNTIME_JS = QML_DIR / "qml/runtime/AppWindowRuntime.js"
QML_QRC = QML_DIR / "resources/qrc/qml-shell.qrc"


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
        self.assertIn('alias="qml/runtime/AppWindowRuntime.js"', qrc_text)
        self.assertIn("../../qml/runtime/AppWindowRuntime.js", qrc_text)

    def test_main_files_import_and_delegate_pure_runtime_logic(self):
        for qml_path in (SHARED_MAIN, LINUX_MAIN):
            text = qml_path.read_text(encoding="utf-8")
            with self.subTest(file=qml_path.name):
                self.assertRegex(text, r'import\s+"qrc:/qml/runtime/AppWindowRuntime\.js"\s+as AppWindowRuntime')
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
                    self.assertIn(f"shell.page === ShellViewModel.{page}", text)

                self.assertNotIn("property int displayedPage", text)
                self.assertNotIn("readonly property int noDisplayedPage: -1", text)
                self.assertNotIn("active: root.chatPageActive", text)

    def test_pet_window_lifecycle_is_bound_to_logged_in_chat_account(self):
        for qml_path in (SHARED_MAIN, LINUX_MAIN):
            text = qml_path.read_text(encoding="utf-8")
            with self.subTest(file=qml_path.name):
                self.assertIn("property var petWindowRef: null", text)
                self.assertIn("function petAccountReady()", text)
                self.assertIn("function bindStartupPetSettingsToCurrentUser()", text)
                self.assertIn("function destroyPetWindow()", text)

                bind_body = function_body(text, "bindStartupPetSettingsToCurrentUser")
                self.assertIn("startupPetSettings.bindAccount(shell.currentUserUid, shell.currentUserId)", bind_body)
                self.assertIn("startupPetSettings.load()", bind_body)

                open_body = function_body(text, "openPetWindow")
                self.assertIn("if (!root.petAccountReady())", open_body)
                self.assertIn("return null", open_body)
                self.assertIn("root.bindStartupPetSettingsToCurrentUser()", open_body)

                toggle_body = function_body(text, "togglePetWindow")
                self.assertIn("if (!root.petAccountReady())", toggle_body)
                self.assertIn("return", toggle_body)

                destroy_body = function_body(text, "destroyPetWindow")
                self.assertIn("startupPetTimer.stop()", destroy_body)
                self.assertIn("root.petWindowRef = null", destroy_body)
                self.assertIn("win.petAssetSettings = null", destroy_body)
                self.assertIn("win.petController = null", destroy_body)
                self.assertIn("win.agentController = null", destroy_body)
                self.assertIn("win.destroy()", destroy_body)

                sync_body = function_body(text, "syncWindowsByPage")
                self.assertLess(sync_body.index("destroyPetWindow()"), sync_body.index("destroyChatWindow()"))

                current_user_body = function_body(text, "onCurrentUserChanged")
                self.assertIn("if (!root.petAccountReady())", current_user_body)
                self.assertIn("root.destroyPetWindow()", current_user_body)
                self.assertIn("root.bindStartupPetSettingsToCurrentUser()", current_user_body)
                self.assertIn("petWindowRef.petAssetSettings = startupPetSettings", current_user_body)


if __name__ == "__main__":
    unittest.main()
