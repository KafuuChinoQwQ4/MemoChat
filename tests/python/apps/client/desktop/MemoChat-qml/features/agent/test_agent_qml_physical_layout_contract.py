import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
FEATURE_VIEW = CLIENT / "features/agent/view"
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
    "AgentGameRuntime.js",
    "AgentGameSetupHeader.qml",
    "AgentGameSetupPane.qml",
    "AgentGameTemplatePane.qml",
    "AgentMarkdownCodeBlock.qml",
    "AgentMarkdownRuntime.js",
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
    "AgentPaneRuntime.js",
    "AgentStatusOverlay.qml",
    "AgentTaskPane.qml",
    "AgentTracePane.qml",
    "KnowledgeBasePane.qml",
    "SmartFeatureBar.qml",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class AgentQmlPhysicalLayoutContractTests(unittest.TestCase):
    def test_agent_qml_lives_under_feature_view(self):
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertTrue((FEATURE_VIEW / file_name).is_file())
                self.assertFalse((LEGACY_VIEW / file_name).exists())

    def test_agent_qrc_uses_feature_aliases_only(self):
        self.assertFalse(LEGACY_QRC.exists())
        feature_qrc = read(FEATURE_QRC)
        legacy_qrc = read(LEGACY_QRC)
        for file_name in FEATURE_FILES:
            with self.subTest(file=file_name):
                self.assertIn(f'alias="features/agent/view/{file_name}">../view/{file_name}</file>', feature_qrc)
                self.assertNotIn(f'alias="qml/agent/{file_name}"', legacy_qrc)

    def test_agent_qrc_is_registered_by_feature_manifest(self):
        self.assertIn("features/agent/resources/agent.qrc", read(SOURCES))


if __name__ == "__main__":
    unittest.main()
