import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
MAIN_CPP = CLIENT_DIR / "main.cpp"
SHARED_MAIN_QML = CLIENT_DIR / "qml/Main.qml"
LINUX_MAIN_QML = CLIENT_DIR / "qml/linux/Main.qml"
LIVE2D_CPP = CLIENT_DIR / "Live2DRenderItem.cpp"
LIVE2D_H = CLIENT_DIR / "Live2DRenderItem.h"


def read(path):
    return path.read_text(encoding="utf-8")


def function_body(source, name):
    match = re.search(r"\bfunction\s+" + re.escape(name) + r"\s*\([^)]*\)\s*\{", source)
    if not match:
        return ""

    depth = 1
    index = match.end()
    while index < len(source) and depth > 0:
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        index += 1
    return source[match.end():index - 1]


def cpp_function_body(source, name):
    match = re.search(
        r"(?:^|\n)[\w:<>,\s*&]+\b" + re.escape(name) + r"\s*\([^)]*\)\s*(?:const\s*)?\{",
        source,
    )
    if not match:
        return ""

    open_brace = source.find("{", match.start())
    depth = 1
    index = open_brace + 1
    while index < len(source) and depth > 0:
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        index += 1
    return source[open_brace + 1:index - 1]


class UiStartupPerformanceContractTests(unittest.TestCase):
    def test_main_cpp_defines_top_level_centering_retry_contract(self):
        source = read(MAIN_CPP)

        self.assertRegex(
            source,
            r"\b(centerTopLevelWindow|scheduleInitialWindowCentering)\b",
            "main.cpp should expose a C++ top-level window centering helper/scheduler",
        )
        self.assertIn("QTimer", source)
        self.assertIn("availableGeometry", source)
        self.assertRegex(source, r"\b(QGuiApplication::primaryScreen|window->screen|win->screen)\s*\(")
        self.assertRegex(source, r"\bQGuiApplication::topLevelWindows\s*\(")
        self.assertRegex(source, r"\bQTimer::singleShot\s*\(")
        self.assertRegex(source, r"\bsetX\s*\(|\bsetPosition\s*\(")
        self.assertRegex(source, r"\bsetY\s*\(|\bsetPosition\s*\(")

    def test_main_cpp_sets_linux_threaded_render_loop_without_overriding_user_env(self):
        source = read(MAIN_CPP)

        linux_blocks = re.findall(r"#ifdef\s+Q_OS_LINUX(?P<body>.*?)#endif", source, flags=re.S)
        self.assertTrue(linux_blocks, "main.cpp should keep Linux-specific startup defaults guarded")
        linux_source = "\n".join(linux_blocks)

        self.assertIn("QSG_RENDER_LOOP", linux_source)
        self.assertIn("threaded", linux_source)
        self.assertRegex(source, r"\bvoid\s+setDefaultEnv\s*\(\s*const\s+char\s*\*\s*name\s*,\s*const\s+char\s*\*\s*value\s*\)")
        self.assertRegex(source, r"\b(qEnvironmentVariableIsSet|qEnvironmentVariableIsEmpty|qgetenv)\s*\(")
        self.assertRegex(
            source,
            r"(?:if\s*\(\s*qEnvironmentVariableIsEmpty\s*\(\s*name\s*\)|if\s*\(\s*!qEnvironmentVariableIsSet\s*\(\s*name\s*\)|if\s*\(\s*qgetenv\s*\(\s*name\s*\)\.isEmpty\s*\(\s*\))",
            "setDefaultEnv must only set missing or empty environment values",
        )
        self.assertRegex(
            linux_source,
            r"\b(?:setDefaultEnv|qputenv)\s*\(\s*\"QSG_RENDER_LOOP\"\s*,\s*\"threaded\"",
        )

    def test_shared_and_linux_main_qml_retry_center_login_and_chat_windows(self):
        for path in (SHARED_MAIN_QML, LINUX_MAIN_QML):
            with self.subTest(path=path):
                source = read(path)
                self.assertRegex(
                    source,
                    r"\b(centerWindowWithRetry|centerRetryTimer|centerWindowRetryTimer|Timer\s*\{)",
                    f"{path.name} should include a retry-capable centering helper",
                )
                self.assertIn("availableGeometry", source)
                self.assertRegex(source, r"\b(retry|attempt|remaining|repeat)\b", re.I)

                for function_name in ("showLoginWindow", "showChatWindow"):
                    body = function_body(source, function_name)
                    self.assertTrue(body, f"{path} must define {function_name}()")
                    self.assertRegex(
                        body,
                        r"\b(centerWindowWithRetry|centerRetryTimer|centerWindowRetryTimer|start\s*\()",
                        f"{function_name}() should use the retry centering path",
                    )
                    self.assertRegex(body, r"\bshow\s*\(")

    def test_live2d_render_item_uses_precise_60fps_timer_and_fbo_target(self):
        source = read(LIVE2D_CPP) + "\n" + read(LIVE2D_H)
        ctor = cpp_function_body(source, "Live2DRenderItem")

        self.assertIn("QTimer", source)
        self.assertRegex(source, r"\b(Qt::PreciseTimer|setTimerType\s*\(\s*Qt::PreciseTimer\s*\))")
        self.assertRegex(source, r"\b(setInterval\s*\(\s*16\s*\)|start\s*\(\s*16\s*\))")
        self.assertRegex(source, r"\b(setRenderTarget\s*\(\s*QQuickPaintedItem::FramebufferObject\s*\)|FramebufferObject\b)")
        self.assertRegex(source, r"\bconnect\s*\(")
        self.assertRegex(source, r"\b(startAnimation|stopAnimation|updateAnimationTimer|setVisible|visibleChanged|windowChanged)\b")
        self.assertRegex(source, r"\b(isVisible\s*\(|window\s*\(\s*\))")
        self.assertRegex(source, r"\b(update\s*\(|advance|elapsed|phase|animation)\b")
        self.assertTrue(
            not ctor or "setRenderTarget" in ctor or "FramebufferObject" in ctor,
            "Live2DRenderItem should configure the FBO render target during construction",
        )


if __name__ == "__main__":
    unittest.main()
