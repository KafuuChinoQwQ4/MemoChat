import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
PROFILE = CLIENT / "features/profile"
APP = CLIENT / "app"
CHAT_VIEW = CLIENT / "features/chat/view"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


class ProfileFacadeContractTests(unittest.TestCase):
    def test_profile_controller_exposes_qml_status_and_actions(self):
        header = read(PROFILE / "controller/ProfileController.h")

        expected_tokens = (
            "class ProfileController : public QObject",
            "Q_OBJECT",
            "Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)",
            "Q_PROPERTY(bool statusError READ statusError NOTIFY statusChanged)",
            "QString statusText() const;",
            "bool statusError() const;",
            "Q_INVOKABLE void chooseAvatar(int source = 0);",
            "Q_INVOKABLE void saveProfile(const QString& nick, const QString& desc);",
            "Q_INVOKABLE void clearStatus();",
            "void syncStatus(const QString& text, bool isError);",
            "void statusChanged();",
            "void chooseAvatarRequested(int source);",
            "void saveProfileRequested(const QString& nick, const QString& desc);",
            "void clearStatusRequested();",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, header)

    def test_appcontroller_exposes_additive_profile_surface(self):
        header = read(APP / "controller/AppController.h")
        controller = read(APP / "controller/AppController.cpp")
        model_props = read(APP / "controller/AppControllerModelProperties.cpp")
        status_state = read(APP / "controller/AppControllerStatusState.cpp")
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")

        self.assertNotIn("Q_PROPERTY(ProfileController* profile READ profile CONSTANT)", header)
        self.assertIn("ProfileController* profile();", header)
        self.assertIn("ProfileController* profileController();", header)
        self.assertIn("ProfileController _profile_controller;", header)

        self.assertIn("ProfileController* AppController::profile()", model_props)
        self.assertIn("return &_profile_controller;", model_props)
        self.assertIn("ProfileController* AppController::profileController()", model_props)

        self.assertIn(
            "connect(&_profile_controller, &ProfileController::chooseAvatarRequested, this, &AppController::chooseAvatar);",
            controller,
        )
        self.assertIn(
            "connect(&_profile_controller, &ProfileController::saveProfileRequested, this, &AppController::saveProfile);",
            controller,
        )
        self.assertIn(
            "connect(&_profile_controller, &ProfileController::clearStatusRequested, this, "
            "&AppController::clearSettingsStatus);",
            normalized(controller),
        )
        self.assertIn(
            "_profile_controller.syncStatus(_feature_status_state.settingsText, _feature_status_state.settingsError);",
            controller,
        )
        self.assertIn("_profile_controller.syncStatus(text, isError);", status_state)
        self.assertIn('setContextProperty("profile", controller.profileController())', engine)

    def test_chat_profile_center_uses_profile_facade_for_settings_surface(self):
        qml = read(CHAT_VIEW / "ChatNormalFace.qml")

        expected_profile_tokens = (
            "statusText: profile.statusText",
            "statusError: profile.statusError",
            "onChooseAvatarRequested: function(source) { profile.chooseAvatar(source) }",
            "onSaveProfileRequested: function(nick, desc) { profile.saveProfile(nick, desc) }",
            "onStatusCleared: profile.clearStatus()",
        )
        for token in expected_profile_tokens:
            with self.subTest(token=token):
                self.assertIn(token, qml)

        forbidden_controller_tokens = (
            "statusText: controller.settingsStatusText",
            "statusError: controller.settingsStatusError",
            "controller.chooseAvatar(source)",
            "controller.saveProfile(nick, desc)",
            "controller.clearSettingsStatus()",
        )
        for token in forbidden_controller_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, qml)

    def test_appcontroller_prunes_legacy_profile_qml_surface_but_keeps_cpp_targets(self):
        header = read(APP / "controller/AppController.h")

        legacy_qml_tokens = (
            "Q_INVOKABLE void chooseAvatar(int source = 0);",
            "Q_INVOKABLE void saveProfile(const QString& nick, const QString& desc);",
            "Q_INVOKABLE void clearSettingsStatus();",
            "Q_PROPERTY(QString settingsStatusText READ settingsStatusText NOTIFY settingsStatusChanged)",
            "Q_PROPERTY(bool settingsStatusError READ settingsStatusError NOTIFY settingsStatusChanged)",
        )
        for token in legacy_qml_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        cpp_targets = (
            "void chooseAvatar(int source = 0);",
            "void saveProfile(const QString& nick, const QString& desc);",
            "void clearSettingsStatus();",
            "QString settingsStatusText() const;",
            "bool settingsStatusError() const;",
        )
        for token in cpp_targets:
            with self.subTest(token=token):
                self.assertIn(token, header)


if __name__ == "__main__":
    unittest.main()
