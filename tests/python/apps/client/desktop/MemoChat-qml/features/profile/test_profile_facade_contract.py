import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
PROFILE = CLIENT / "features/profile"
APP = CLIENT / "app"
APP_FEATURE_REGISTRY_H = APP / "composition/AppFeatureRegistry.h"
APP_PROFILE_FEATURE_PORT_BINDER = APP / "composition/AppProfileFeaturePortBinder.cpp"
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
            "Q_INVOKABLE void clearStatusState();",
            "bool validateProfile(const QString& nick, const QString& desc, QString* errorText) const;",
            "void syncStatus(const QString& text, bool isError);",
            "void setCommandPort(ProfileCommandPort port);",
            "void setStatePort(ProfileStatePort port);",
            "void handleSettingsHttpFinished(ReqId id, const QString& res, ErrorCodes err);",
            "void applyCurrentUserProfile(const QJsonObject& profile, bool preserveExistingIcon);",
            "struct ProfileCommandPort",
            "struct ProfileStatePort",
            "struct ProfileCurrentUserState",
            "struct ProfileAppliedUserState",
            "void statusChanged();",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, header)

        for token in (
            "void chooseAvatarRequested(int source);",
            "void saveProfileRequested(const QString& nick, const QString& desc);",
            "void clearStatusRequested();",
        ):
            with self.subTest(removed_signal=token):
                self.assertNotIn(token, header)

    def test_appcontroller_exposes_additive_profile_composition_surface(self):
        header = read(APP / "controller/AppController.h")
        registry = read(APP_FEATURE_REGISTRY_H)
        controller = read(APP / "controller/AppController.cpp")
        port_binder = read(APP_PROFILE_FEATURE_PORT_BINDER)
        model_props = read(APP / "controller/AppControllerModelProperties.cpp")
        status_state = read(APP / "controller/AppControllerStatusState.cpp")
        composition = read(APP / "composition/AppComposition.cpp")
        public = header[header.index("public:") : header.index("signals:")]
        private = header[header.index("private:") :]

        self.assertNotIn("Q_PROPERTY(ProfileController* profile READ profile CONSTANT)", header)
        self.assertNotIn("ProfileController* profile();", header)
        self.assertIn("ProfileController* profileController();", public)
        self.assertIn("AppFeatureRegistry _features", header)
        self.assertIn("ProfileController profileController;", registry)

        self.assertNotIn("ProfileController* AppController::profile()", model_props)
        self.assertIn("return &_features.profileController;", model_props)
        self.assertIn("ProfileController* AppController::profileController()", model_props)

        controller_tokens = normalized(controller)
        port_binder_tokens = normalized(port_binder)
        self.assertIn("_features.profileController.setCommandPort(ProfileCommandPort{", port_binder_tokens)
        self.assertIn("_features.profileController.setStatePort( ProfileStatePort{", port_binder_tokens)
        self.assertNotIn("_features.profileController.setCommandPort(ProfileCommandPort{", controller_tokens)
        self.assertNotIn("_features.profileController.setStatePort( ProfileStatePort{", controller_tokens)
        self.assertNotIn("ProfileController::chooseAvatarRequested", controller)
        self.assertNotIn("ProfileController::saveProfileRequested", controller)
        self.assertNotIn("ProfileController::clearStatusRequested", controller)
        self.assertNotIn("_feature_status_state.settingsText", controller)
        self.assertIn("_features.profileController.syncStatus(text, isError);", status_state)
        self.assertIn('context.setContextProperty("profile", profile())', composition)

    def test_profile_response_reducer_is_feature_owned(self):
        http_event_router = read(APP / "events/AppHttpEventRouter.cpp")
        profile_controller = read(PROFILE / "controller/ProfileController.cpp")
        profile_state = read(APP / "controller/AppControllerProfileState.cpp")

        self.assertIn("_profile_controller.handleSettingsHttpFinished(id, res, err);", http_event_router)
        self.assertIn("_contact_controller.handleContactHttpFinished(id, res, err);", http_event_router)
        for forbidden in (
            "_features.authController.parseJson",
            "UpdateNickAndDesc",
            "UpdateIcon",
            'obj.value("nick")',
            'obj.value("desc")',
            'obj.value("icon")',
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, http_event_router)

        self.assertIn("void ProfileController::handleSettingsHttpFinished", profile_controller)
        self.assertIn("void ProfileController::applyCurrentUserProfile", profile_controller)
        self.assertIn("parseProfileResponse", profile_controller)
        self.assertIn("userMgr->SetUserInfo", profile_controller)
        self.assertIn("userInfo->_icon = nextIcon;", profile_controller)
        self.assertIn("_state_port.syncCurrentUser", profile_controller)

        self.assertIn("_features.profileController.applyCurrentUserProfile", profile_state)
        self.assertIn("void AppController::syncCurrentUserProfileState", profile_state)
        self.assertNotIn("userInfo->_icon = nextIcon;", profile_state)
        self.assertNotIn("_gateway.userMgr()->SetUserInfo", profile_state)

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

    def test_appcontroller_prunes_legacy_profile_qml_surface_and_cpp_wrappers(self):
        header = read(APP / "controller/AppController.h")
        public = header[header.index("public:") : header.index("signals:")]
        private = header[header.index("private:") :]

        legacy_qml_tokens = (
            "Q_INVOKABLE void chooseAvatar(int source = 0);",
            "Q_INVOKABLE void saveProfile(const QString& nick, const QString& desc);",
            "Q_INVOKABLE void clearSettingsStatus();",
            "Q_PROPERTY(QString settingsStatusText READ settingsStatusText NOTIFY settingsStatusChanged)",
            "Q_PROPERTY(bool settingsStatusError READ settingsStatusError NOTIFY settingsStatusChanged)",
            "QString settingsStatusText() const;",
            "bool settingsStatusError() const;",
        )
        for token in legacy_qml_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        removed_cpp_targets = (
            "void chooseAvatar(int source = 0);",
            "void saveProfile(const QString& nick, const QString& desc);",
            "void clearSettingsStatus();",
        )
        for token in removed_cpp_targets:
            with self.subTest(token=token):
                self.assertNotIn(token, header)


if __name__ == "__main__":
    unittest.main()
