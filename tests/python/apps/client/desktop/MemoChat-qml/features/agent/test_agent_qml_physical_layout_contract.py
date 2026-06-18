import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
FEATURE_VIEW = CLIENT / "features/agent/view"
FEATURE_RUNTIME = CLIENT / "features/agent/runtime"
LEGACY_VIEW = CLIENT / "qml/agent"
FEATURE_QRC = CLIENT / "features/agent/resources/agent.qrc"
LEGACY_QRC = CLIENT / "resources/qrc/qml-agent.qrc"
SOURCES = CLIENT / "features/agent/sources.cmake"

FEATURE_FILES = (
    "AgentComposerBar.qml",
    "AgentConversationPane.qml",
    "AgentGameAgentCard.qml",
    "AgentGameOptionCombo.qml",
    "AgentGameOverlay.qml",
    "AgentGamePane.qml",
    "AgentGamePlayPane.qml",
    "AgentGameRoomPane.qml",
    "AgentGameSetupHeader.qml",
    "AgentGameSetupPane.qml",
    "AgentGameTemplatePane.qml",
    "AgentMarkdownCodeBlock.qml",
    "AgentMarkdownText.qml",
    "AgentMemoryPane.qml",
    "AgentMessageDelegate.qml",
    "AgentModelControlBar.qml",
    "AgentMultiAiChatPane.qml",
    "AgentMultiAiControls.qml",
    "AgentMultiAiParticipantEditor.qml",
    "AgentMultiAiSessionPanel.qml",
    "AgentPane.qml",
    "AgentPaneHeader.qml",
    "AgentStatusOverlay.qml",
    "AgentTaskPane.qml",
    "AgentTracePane.qml",
    "KnowledgeBasePane.qml",
    "SmartFeatureBar.qml",
)

# Pure JavaScript runtime helpers live in a dedicated runtime/ folder, not view/.
RUNTIME_FILES = (
    "AgentGameRuntime.js",
    "AgentMarkdownRuntime.js",
    "AgentPaneRuntime.js",
    "AgentProviderRuntime.js",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class AgentQmlPhysicalLayoutContractTests(unittest.TestCase):
    def test_agent_qml_lives_under_feature_view(self):
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertTrue((FEATURE_VIEW / file_name).is_file())
                self.assertFalse((LEGACY_VIEW / file_name).exists())

    def test_agent_runtime_js_lives_under_feature_runtime(self):
        for file_name in RUNTIME_FILES:
            with self.subTest(file=file_name):
                self.assertTrue((FEATURE_RUNTIME / file_name).is_file())
                self.assertFalse((FEATURE_VIEW / file_name).exists())

    def test_agent_qrc_uses_feature_aliases_only(self):
        self.assertFalse(LEGACY_QRC.exists())
        feature_qrc = read(FEATURE_QRC)
        legacy_qrc = read(LEGACY_QRC)
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertIn(f'alias="features/agent/view/{file_name}">../view/{file_name}</file>', feature_qrc)
                self.assertNotIn(f'alias="qml/agent/{file_name}"', legacy_qrc)
        for file_name in RUNTIME_FILES:
            with self.subTest(file=file_name):
                self.assertIn(f'alias="features/agent/runtime/{file_name}">../runtime/{file_name}</file>', feature_qrc)

    def test_agent_qrc_is_registered_by_feature_manifest(self):
        self.assertIn("features/agent/resources/agent.qrc", read(SOURCES))

    def test_agent_conversation_empty_prompt_follows_visible_message_count(self):
        pane = read(FEATURE_VIEW / "AgentConversationPane.qml")

        self.assertIn("readonly property int modelMessageCount:", pane)
        self.assertIn("root.messageModel.count", pane)
        self.assertIn("root.modelMessageCount === 0", pane)
        self.assertIn("!root.modelHasStreamingMessage", pane)
        self.assertIn("visible: root.showEmptyPrompt", pane)
        self.assertNotIn("messageListView.count === 0", pane)

    def test_agent_generation_status_follows_row_streaming_state(self):
        pane = read(FEATURE_VIEW / "AgentConversationPane.qml")
        delegate = read(FEATURE_VIEW / "AgentMessageDelegate.qml")

        self.assertIn("isStreaming: model.isStreaming || false", pane)
        self.assertNotIn("readonly property bool activeGenerating:", pane)
        self.assertNotIn("currentGeneratingMsgId", pane)
        self.assertNotIn("model.msgId === root.currentGeneratingMsgId", pane)
        self.assertNotIn("&& (model.isStreaming || false)", pane)
        self.assertNotIn("isStreaming: (root.loading || root.streaming) && (model.isStreaming || false)", pane)
        self.assertIn(
            "readonly property bool showStreamingStatus: root.isStreaming && root.errorMessage.length === 0",
            delegate,
        )
        self.assertIn("visible: root.showStreamingStatus", delegate)

    def test_multi_ai_empty_prompt_follows_real_chat_message_count(self):
        pane = read(FEATURE_VIEW / "AgentMultiAiChatPane.qml")

        self.assertIn("readonly property var displayEventRows:", pane)
        self.assertIn("readonly property int chatMessageCount:", pane)
        self.assertIn("if (root.isChatEvent(rows[i] || {}))", pane)
        self.assertIn("model: root.displayEventRows", pane)
        self.assertIn("visible: root.chatMessageCount === 0", pane)
        self.assertNotIn("visible: messageListView.count === 0", pane)


if __name__ == "__main__":
    unittest.main()
