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
LIVE2D_RENDERER_H = CLIENT_DIR / "Live2DRenderer.h"
LIVE2D_CORE_CPP = CLIENT_DIR / "Live2DCoreRenderer.cpp"
LIVE2D_CORE_H = CLIENT_DIR / "Live2DCoreRenderer.h"
LIVE2D_OFFICIAL_CPP = CLIENT_DIR / "Live2DOfficialOpenGLRenderer.cpp"
LIVE2D_OFFICIAL_H = CLIENT_DIR / "Live2DOfficialOpenGLRenderer.h"
LIVE2D_PLACEHOLDER_CPP = CLIENT_DIR / "Live2DPlaceholderRenderer.cpp"
LIVE2D_PLACEHOLDER_H = CLIENT_DIR / "Live2DPlaceholderRenderer.h"
CLIENT_CMAKE = CLIENT_DIR / "CMakeLists.txt"
APP_CONTROLLER_SESSION_CPP = CLIENT_DIR / "AppControllerSession.cpp"


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
                self.assertIn("startupShowRetryTimer", source)
                self.assertIn("ensureLoginWindowVisible", source)
                self.assertIn("logWindowState", source)
                self.assertIn("availableGeometry", source)
                self.assertRegex(source, r"\b(retry|attempt|remaining|repeat)\b", re.I)

                for function_name in ("showLoginWindow", "showChatWindow"):
                    body = function_body(source, function_name)
                    self.assertTrue(body, f"{path} must define {function_name}()")
                    self.assertRegex(
                        body,
                        r"\b(centerWindowWithRetry|centerRetryTimer|centerWindowRetryTimer|startupShowRetryTimer|start\s*\()",
                        f"{function_name}() should use the retry centering path",
                    )
                    self.assertRegex(body, r"\bshow\s*\(")

    def test_live2d_render_item_uses_precise_60fps_timer_and_fbo_target(self):
        source = read(LIVE2D_CPP) + "\n" + read(LIVE2D_H)
        ctor = cpp_function_body(source, "Live2DRenderItem")

        self.assertIn("QTimer", source)
        self.assertRegex(source, r"\b(Qt::PreciseTimer|setTimerType\s*\(\s*Qt::PreciseTimer\s*\))")
        self.assertRegex(source, r"\b(setInterval\s*\(\s*16\s*\)|start\s*\(\s*16\s*\))")
        self.assertIn("QQuickFramebufferObject", source)
        self.assertIn("createFramebufferObject", source)
        self.assertIn("QOpenGLFramebufferObject", source)
        self.assertNotIn("QQuickPaintedItem", source)
        self.assertNotIn("#ifdef Q_OS_LINUX", ctor)
        self.assertRegex(source, r"\bconnect\s*\(")
        self.assertRegex(source, r"\b(startAnimation|stopAnimation|updateAnimationTimer|setVisible|visibleChanged|windowChanged)\b")
        self.assertRegex(source, r"\b(isVisible\s*\(|window\s*\(\s*\))")
        self.assertRegex(source, r"\b(update\s*\(|advance|elapsed|phase|animation)\b")
        self.assertTrue(
            "QQuickFramebufferObject" in source and "Renderer *createRenderer" in source,
            "Live2DRenderItem should render through a Qt Quick FBO renderer",
        )

    def test_live2d_render_item_has_renderer_interface_boundary(self):
        render_item_source = read(LIVE2D_CPP) + "\n" + read(LIVE2D_H)
        renderer_header = read(LIVE2D_RENDERER_H)
        core_source = read(LIVE2D_CORE_CPP) + "\n" + read(LIVE2D_CORE_H)
        official_source = read(LIVE2D_OFFICIAL_CPP) + "\n" + read(LIVE2D_OFFICIAL_H)
        placeholder_source = read(LIVE2D_PLACEHOLDER_CPP) + "\n" + read(LIVE2D_PLACEHOLDER_H)
        cmake = read(CLIENT_CMAKE)

        for token in (
            "struct Live2DVisualState",
            "class Live2DRenderer",
            "virtual void paint",
            "rendererName",
            "isNative",
        ):
            self.assertIn(token, renderer_header)

        self.assertIn("Live2DCoreRenderer", core_source)
        for token in (
            "Live2DOfficialOpenGLRenderer",
            "CubismRenderer_OpenGLES2",
            "CubismMoc::Create",
            "CubismPhysics::Create",
            "CubismExpressionMotion::Create",
            "CubismRenderer::Create",
            "BindTexture",
            "DrawModel",
            "SetMvpMatrix",
            "glewInit",
            "QImage::Format_RGBA8888",
            "glTexImage2D",
            "glGenerateMipmap",
            "resetSpecialExpressionParameters",
            "loadExpressions",
            "loadPhysics",
        ):
            self.assertIn(token, official_source)
        self.assertNotIn('aliasExpression(QStringLiteral("concerned"), QStringLiteral("脸黑"))', official_source)
        self.assertIn("Live2DOfficialOpenGLRenderer", render_item_source)
        self.assertIn("Live2DPlaceholderRenderer", placeholder_source)
        self.assertIn("Live2DRenderer", render_item_source)
        self.assertIn("std::unique_ptr<Live2DOfficialOpenGLRenderer>", render_item_source)
        self.assertIn("std::make_unique<Live2DOfficialOpenGLRenderer>", render_item_source)
        self.assertIn("synchronize(QQuickFramebufferObject", render_item_source)
        self.assertIn("void render() override", render_item_source)
        self.assertIn("QQuickOpenGLUtils::resetOpenGLState", render_item_source)
        self.assertIn("visualState()", render_item_source)
        self.assertIn("official OpenGL renderer unavailable", render_item_source)
        self.assertIn("drawModelErrorMarker", render_item_source)
        self.assertIn("模型显示错误", render_item_source)
        self.assertIn("Q_PROPERTY(QString renderStatus", render_item_source)
        self.assertIn("Q_PROPERTY(QString renderError", render_item_source)
        self.assertIn("setRenderStatusFromRenderer", render_item_source)
        self.assertNotIn("Live2DPlaceholderRenderer.h", render_item_source)
        self.assertIn("Live2DRenderer.h", cmake)
        self.assertIn("Live2DCoreRenderer.cpp", cmake)
        self.assertIn("Live2DCoreRenderer.h", cmake)
        self.assertIn("Live2DOfficialOpenGLRenderer.cpp", cmake)
        self.assertIn("Live2DOfficialOpenGLRenderer.h", cmake)
        self.assertIn("MemoChatLive2DCubismFramework", cmake)
        self.assertIn("CSM_TARGET_LINUX_GL", cmake)
        self.assertIn("GLEW::GLEW", cmake)
        self.assertIn("OpenGL::GL", cmake)
        self.assertIn("Live2DPlaceholderRenderer.cpp", cmake)
        self.assertIn("Live2DPlaceholderRenderer.h", cmake)
        self.assertNotIn("#if MEMOCHAT_ENABLE_LIVE2D_NATIVE", render_item_source)
        self.assertIn("drawPlaceholderPet", placeholder_source)
        self.assertNotIn("paintKafuuChinoPreview", placeholder_source)

    def test_live2d_render_item_accepts_custom_model_source_and_rebuilds_renderer(self):
        header = read(LIVE2D_H)
        source = read(LIVE2D_CPP)
        official_header = read(LIVE2D_OFFICIAL_H)
        official_source = read(LIVE2D_OFFICIAL_CPP)

        self.assertIn("Q_PROPERTY(QString modelRoot", header)
        self.assertIn("Q_PROPERTY(QString modelJson", header)
        self.assertIn("setModelRoot", source)
        self.assertIn("setModelJson", source)
        self.assertIn("resolvedModelPath", source)
        self.assertIn("resolveModelPath", source)
        self.assertIn("Live2DOfficialOpenGLRenderer(const QString &modelPath", official_header)
        self.assertIn("defaultModelPath()", official_source)
        self.assertIn("resolveModelPath(inputModelPath)", official_source)

    def test_main_cpp_forces_opengl_backend_for_official_live2d_renderer(self):
        source = read(MAIN_CPP)

        self.assertIn("QSGRendererInterface", source)
        self.assertIn("QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL)", source)
        self.assertIn('setDefaultEnv("QSG_RHI_BACKEND", "opengl")', source)

    def test_post_login_chat_bootstrap_delay_is_100ms(self):
        source = read(APP_CONTROLLER_SESSION_CPP)

        self.assertRegex(source, r"constexpr\s+int\s+kPostLoginBootstrapDelayMs\s*=\s*100\s*;")
        self.assertRegex(
            source,
            r"QTimer::singleShot\s*\(\s*kPostLoginBootstrapDelayMs\s*,\s*this\s*,\s*\[this\]",
        )


if __name__ == "__main__":
    unittest.main()
