import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
SETTINGS = CLIENT / "features/settings"
PROFILE = CLIENT / "features/profile"
CHAT_VIEW = CLIENT / "features/chat/view"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class SettingsFeatureBoundaryContractTests(unittest.TestCase):
    def test_settings_is_qml_only_and_documents_profile_delegation(self):
        self.assertTrue((SETTINGS / "view/SettingsPane.qml").is_file())
        self.assertTrue((SETTINGS / "view/MorePane.qml").is_file())
        self.assertTrue((SETTINGS / "resources/settings.qrc").is_file())
        self.assertTrue((SETTINGS / "README.md").is_file())
        self.assertFalse((SETTINGS / "controller").exists())
        self.assertFalse((SETTINGS / "viewmodel").exists())

        readme = read(SETTINGS / "README.md")
        self.assertIn("UI-only feature", readme)
        self.assertIn("ProfileController", readme)

    def test_settings_pane_exposes_profile_action_signals_without_controller_dependency(self):
        settings_pane = read(SETTINGS / "view/SettingsPane.qml")
        profile_header = read(PROFILE / "controller/ProfileController.h")
        chat_normal_face = read(CHAT_VIEW / "ChatNormalFace.qml")

        for token in (
            "signal chooseAvatarRequested(int source)",
            "signal saveProfileRequested(string nick, string desc)",
            "signal statusCleared()",
        ):
            with self.subTest(token=token):
                self.assertIn(token, settings_pane)

        self.assertNotIn("property var settingsController", settings_pane)
        self.assertIn("Q_INVOKABLE void chooseAvatar(int source = 0);", profile_header)
        self.assertIn("Q_INVOKABLE void saveProfile(const QString& nick, const QString& desc);", profile_header)
        self.assertIn("onChooseAvatarRequested: function(source) { profile.chooseAvatar(source) }", chat_normal_face)
        self.assertIn(
            "onSaveProfileRequested: function(nick, desc) { profile.saveProfile(nick, desc) }", chat_normal_face
        )


if __name__ == "__main__":
    unittest.main()
