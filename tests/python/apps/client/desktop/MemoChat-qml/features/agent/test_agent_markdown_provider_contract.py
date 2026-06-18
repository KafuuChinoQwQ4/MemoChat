import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
AGENT_VIEW = CLIENT / "features/agent/view"
AGENT_RUNTIME = CLIENT / "features/agent/runtime"
AGENT_QRC = CLIENT / "features/agent/resources/agent.qrc"
AGENT_PANE = AGENT_VIEW / "AgentPane.qml"
AGENT_CONVERSATION = AGENT_VIEW / "AgentConversationPane.qml"
AGENT_DELEGATE = AGENT_VIEW / "AgentMessageDelegate.qml"
AGENT_MARKDOWN_TEXT = AGENT_VIEW / "AgentMarkdownText.qml"
AGENT_MARKDOWN_RUNTIME = AGENT_RUNTIME / "AgentMarkdownRuntime.js"
AGENT_PROVIDER_RUNTIME = AGENT_RUNTIME / "AgentProviderRuntime.js"
AGENT_PROVIDER_ICONS = CLIENT / "features/agent/resources/icons"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class AgentMarkdownProviderContractTests(unittest.TestCase):
    def test_agent_answer_markdown_text_uses_sanitized_rich_text_renderer(self):
        markdown_qml = read(AGENT_MARKDOWN_TEXT)
        runtime = read(AGENT_MARKDOWN_RUNTIME)

        self.assertIn("AgentMarkdownRuntime.renderRichText", markdown_qml)
        self.assertIn("textFormat: TextEdit.RichText", markdown_qml)
        self.assertIn("function renderRichText", runtime)
        self.assertIn("function renderInlineMarkdown", runtime)
        self.assertIn("function renderMathBlock", runtime)
        self.assertIn("function renderInlineMath", runtime)
        self.assertIn("function renderLatexTokens", runtime)
        self.assertIn("function escapeHtml", runtime)

        for token in (
            "\\*\\*",
            "$$",
            "\\(",
            "\\[",
            "&lt;",
            "&gt;",
            'command.name === "frac"',
            'command.name === "sqrt"',
            "<sup>",
            "<sub>",
        ):
            with self.subTest(token=token):
                self.assertIn(token, runtime)

    def test_provider_avatar_runtime_centralizes_common_model_suppliers(self):
        self.assertTrue(AGENT_PROVIDER_RUNTIME.is_file())
        runtime = read(AGENT_PROVIDER_RUNTIME)
        qrc = read(AGENT_QRC)

        for function_name in (
            "providerKey",
            "providerDisplayName",
            "providerAvatarSource",
            "providerPalette",
        ):
            with self.subTest(function=function_name):
                self.assertRegex(runtime, rf"function\s+{re.escape(function_name)}\s*\(")

        for token in (
            "openai",
            "gpt",
            "anthropic",
            "claude",
            "google",
            "gemini",
            "deepseek",
            "qwen",
            "alibaba",
            "zhipu",
            "glm",
            "kimi",
            "moonshot",
            "ollama",
            "custom-api",
        ):
            with self.subTest(provider=token):
                self.assertIn(token, runtime)

        self.assertIn('alias="features/agent/runtime/AgentProviderRuntime.js"', qrc)
        for icon in (
            "ai_provider_openai.svg",
            "ai_provider_claude.svg",
            "ai_provider_gemini.svg",
            "ai_provider_deepseek.svg",
            "ai_provider_qwen.svg",
            "ai_provider_glm.svg",
            "ai_provider_kimi.svg",
            "ai_provider_generic.svg",
        ):
            with self.subTest(icon=icon):
                self.assertIn(f"icons/{icon}", qrc + "\n" + runtime)

    def test_message_delegate_receives_model_name_and_provider_avatar_from_runtime(self):
        pane = read(AGENT_PANE)
        conversation = read(AGENT_CONVERSATION)
        delegate = read(AGENT_DELEGATE)

        self.assertIn("currentModel: root.currentModel", pane)
        self.assertIn('import "../runtime/AgentProviderRuntime.js" as AgentProviderRuntime', conversation)
        self.assertRegex(conversation, r"\bproperty\s+string\s+currentModel\s*:")
        self.assertIn("modelName: model.modelName || root.currentModel || \"\"", conversation)
        self.assertIn("aiAvatar: AgentProviderRuntime.providerAvatarSource", conversation)

        self.assertRegex(delegate, r"\bproperty\s+string\s+modelName\s*:")
        self.assertRegex(delegate, r"\bproperty\s+string\s+providerDisplayName\s*:")

    def test_provider_avatar_is_centered_and_rendered_at_high_source_size(self):
        delegate = read(AGENT_DELEGATE)

        for token in (
            "readonly property real aiAvatarIconSize:",
            "readonly property int aiAvatarSourceSize:",
            "id: aiAvatarImage",
            "anchors.centerIn: parent",
            "width: root.aiAvatarIconSize",
            "height: root.aiAvatarIconSize",
            "fillMode: Image.PreserveAspectFit",
            "sourceSize.width: root.aiAvatarSourceSize",
            "sourceSize.height: root.aiAvatarSourceSize",
            "smooth: true",
            "mipmap: true",
        ):
            with self.subTest(token=token):
                self.assertIn(token, delegate)

        self.assertNotIn("anchors.fill: parent\n                source: root.aiAvatar", delegate)

    def test_provider_avatars_use_brand_logo_assets_not_text_placeholders(self):
        expected_titles = {
            "ai_provider_openai.svg": "OpenAI",
            "ai_provider_claude.svg": "Claude",
            "ai_provider_gemini.svg": "Gemini",
            "ai_provider_deepseek.svg": "DeepSeek",
            "ai_provider_qwen.svg": "Qwen",
            "ai_provider_glm.svg": "Zhipu",
            "ai_provider_kimi.svg": "Kimi",
        }

        for icon_name, title in expected_titles.items():
            icon = read(AGENT_PROVIDER_ICONS / icon_name)
            with self.subTest(icon=icon_name):
                self.assertIn(f"<title>{title}</title>", icon)
                self.assertIn("<path", icon)
                self.assertNotIn("<text", icon)
                for placeholder in ("GPT", "CL", "GM", "DS", "QW", "GLM", "KIMI"):
                    self.assertNotIn(f">{placeholder}<", icon)


if __name__ == "__main__":
    unittest.main()
