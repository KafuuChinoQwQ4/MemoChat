import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
QML_ROOT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml"
CHAT_FEATURE_VIEW = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/view"
AGENT_FEATURE_VIEW = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/agent/view"
R18_FEATURE_VIEW = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/r18/view"
QRC_ROOT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/resources/qrc"
QML_QRCS = (
    QRC_ROOT / "qml-shell.qrc",
    CLIENT / "features/chat/resources/chat.qrc",
    CLIENT / "features/agent/resources/agent.qrc",
    CLIENT / "features/r18/resources/r18.qrc",
)

CHAT_SHELL_PAGE = QML_ROOT / "app/ChatShellPage.qml"
CHAT_SHELL_RUNTIME = QML_ROOT / "runtime/ChatShellRuntime.js"
CHAT_NORMAL_FACE = CHAT_FEATURE_VIEW / "ChatNormalFace.qml"
CHAT_MODAL_LAYER = CHAT_FEATURE_VIEW / "ChatModalLayer.qml"
R18_FLIP_FACE = R18_FEATURE_VIEW / "R18FlipFace.qml"
AGENT_GAME_OVERLAY = AGENT_FEATURE_VIEW / "AgentGameOverlay.qml"


class ChatShellBoundaryTests(unittest.TestCase):
    def read(self, path):
        return path.read_text(encoding="utf-8")

    def qml_references(self, token):
        matches = []
        for path in (CLIENT / "qml").rglob("*.qml"):
            if token in self.read(path):
                matches.append(path.relative_to(CLIENT).as_posix())
        for path in (CLIENT / "features").rglob("*.qml"):
            if token in self.read(path):
                matches.append(path.relative_to(CLIENT).as_posix())
        return sorted(matches)

    def qrc_text(self):
        return "\n".join(self.read(path) for path in QML_QRCS)

    def test_extracted_shell_files_exist_and_are_registered(self):
        qrc = self.qrc_text()
        expected_files = (
            (CHAT_SHELL_RUNTIME, f"qml/{CHAT_SHELL_RUNTIME.relative_to(QML_ROOT).as_posix()}"),
            (CHAT_NORMAL_FACE, "features/chat/view/ChatNormalFace.qml"),
            (CHAT_MODAL_LAYER, "features/chat/view/ChatModalLayer.qml"),
            (R18_FLIP_FACE, "features/r18/view/R18FlipFace.qml"),
            (AGENT_GAME_OVERLAY, "features/agent/view/AgentGameOverlay.qml"),
        )

        for path, alias in expected_files:
            with self.subTest(path=path):
                self.assertTrue(path.is_file())
                self.assertIn(alias, qrc)

    def test_chat_shell_page_is_a_coordinator(self):
        shell = self.read(CHAT_SHELL_PAGE)

        self.assertLessEqual(len(shell.splitlines()), 260)
        self.assertIn('import "qrc:/qml/runtime/ChatShellRuntime.js" as ChatShellRuntime', shell)
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
        self.assertEqual(["features/r18/view/R18FlipFace.qml"], self.qml_references("R18ShellPane"))
        self.assertEqual(["features/agent/view/AgentGameOverlay.qml"], self.qml_references("AgentGamePane"))

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
