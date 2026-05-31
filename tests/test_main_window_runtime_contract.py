import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
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
            "shouldHideResize",
            "targetWindowSize",
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
                self.assertIn("AppWindowRuntime.shouldHideResize", text)
                self.assertRegex(
                    text,
                    r"function\s+targetWindowSize\s*\(\)\s*\{[\s\S]{0,220}"
                    r"AppWindowRuntime\.targetWindowSize\s*\(",
                )

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
            r"\b(?:x|y|width|height)\s*:\s*controller\.page",
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


if __name__ == "__main__":
    unittest.main()
