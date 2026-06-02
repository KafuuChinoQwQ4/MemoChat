import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
QML_ROOT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml"
QRC_ROOT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/resources/qrc"
QML_QRCS = (
    QRC_ROOT / "qml-shell.qrc",
    QRC_ROOT / "qml-chat.qrc",
    QRC_ROOT / "qml-agent.qrc",
    QRC_ROOT / "qml-r18.qrc",
)

CHAT_SHELL_PAGE = QML_ROOT / "app/ChatShellPage.qml"
CHAT_SHELL_RUNTIME = QML_ROOT / "app/ChatShellRuntime.js"
CHAT_NORMAL_FACE = QML_ROOT / "chat/ChatNormalFace.qml"
CHAT_MODAL_LAYER = QML_ROOT / "chat/ChatModalLayer.qml"
R18_FLIP_FACE = QML_ROOT / "r18/R18FlipFace.qml"
AGENT_GAME_OVERLAY = QML_ROOT / "agent/AgentGameOverlay.qml"


class ChatShellBoundaryTests(unittest.TestCase):
    def read(self, path):
        return path.read_text(encoding="utf-8")

    def qml_references(self, token):
        matches = []
        for path in QML_ROOT.rglob("*.qml"):
            if token in self.read(path):
                matches.append(path.relative_to(QML_ROOT).as_posix())
        return sorted(matches)

    def qrc_text(self):
        return "\n".join(self.read(path) for path in QML_QRCS)

    def test_extracted_shell_files_exist_and_are_registered(self):
        qrc = self.qrc_text()
        expected_files = (
            CHAT_SHELL_RUNTIME,
            CHAT_NORMAL_FACE,
            CHAT_MODAL_LAYER,
            R18_FLIP_FACE,
            AGENT_GAME_OVERLAY,
        )

        for path in expected_files:
            with self.subTest(path=path):
                self.assertTrue(path.is_file())
                self.assertIn(f"qml/{path.relative_to(QML_ROOT).as_posix()}", qrc)

    def test_chat_shell_page_is_a_coordinator(self):
        shell = self.read(CHAT_SHELL_PAGE)

        self.assertLessEqual(len(shell.splitlines()), 260)
        self.assertIn('import "ChatShellRuntime.js" as ChatShellRuntime', shell)
        self.assertIn("ChatNormalFace", shell)
        self.assertIn("R18FlipFace", shell)
        self.assertIn("AgentGameOverlay", shell)
        self.assertIn("ChatModalLayer", shell)
        self.assertNotIn("R18ShellPane", shell)
        self.assertNotIn("AgentGamePane", shell)
        self.assertNotIn("CreateGroupDialog", shell)
        self.assertNotIn("CallOverlay", shell)
        self.assertNotIn("ContactProfilePopup", shell)

    def test_runtime_script_uses_qml_library_pragma(self):
        runtime_lines = self.read(CHAT_SHELL_RUNTIME).splitlines()
        first_non_empty = next((line.strip() for line in runtime_lines if line.strip()), "")

        self.assertEqual(".pragma library", first_non_empty)

    def test_runtime_helpers_hold_pure_shell_logic(self):
        runtime = self.read(CHAT_SHELL_RUNTIME)

        self.assertIn("function stageValue", runtime)
        self.assertIn("function r18NavigationItems", runtime)
        self.assertIn("function defaultAgentGameSetupKind", runtime)

    def test_heavy_faces_are_owned_by_boundary_components(self):
        self.assertEqual(["r18/R18FlipFace.qml"], self.qml_references("R18ShellPane"))
        self.assertEqual(["agent/AgentGameOverlay.qml"], self.qml_references("AgentGamePane"))

        modal = self.read(CHAT_MODAL_LAYER)
        self.assertIn("CreateGroupDialog", modal)
        self.assertIn("CallOverlay", modal)
        self.assertIn("ContactProfilePopup", modal)

        shell = self.read(CHAT_SHELL_PAGE)
        for token in ("CreateGroupDialog", "CallOverlay", "ContactProfilePopup"):
            with self.subTest(token=token):
                self.assertNotIn(token, shell)


if __name__ == "__main__":
    unittest.main()
