import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
APP = CLIENT / "app"
FEATURES = CLIENT / "features"
QML_ROOT = CLIENT / "qml"
AUTH_VIEW = FEATURES / "auth/view"
CHAT_VIEW = FEATURES / "chat/view"
AGENT_VIEW = FEATURES / "agent/view"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


def qml_sources() -> dict[str, str]:
    sources: dict[str, str] = {}
    for root in (FEATURES, QML_ROOT):
        for path in root.rglob("*"):
            if path.suffix in {".qml", ".js"}:
                sources[path.relative_to(CLIENT).as_posix()] = read(path)
    return sources


def appcontroller_public_section() -> str:
    header = read(APP / "controller/AppController.h")
    return header[header.index("public:") : header.index("signals:")]


def appcontroller_private_section() -> str:
    header = read(APP / "controller/AppController.h")
    return header[header.index("private:") :]


class DirectFeatureContextContractTests(unittest.TestCase):
    def test_engine_exposes_direct_feature_contexts(self):
        composition = read(APP / "composition/AppComposition.cpp")
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")

        expected = (
            'context.setContextProperty("auth", auth())',
            'context.setContextProperty("agent", agent())',
            'context.setContextProperty("agentMessages", agentMessages())',
            'context.setContextProperty("moments", moments())',
            'context.setContextProperty("momentsModel", momentsModel())',
            'context.setContextProperty("pet", pet())',
            'context.setContextProperty("r18", r18())',
        )
        for token in expected:
            with self.subTest(token=token):
                self.assertIn(token, composition)
        self.assertIn("composition.configureQmlContext(*engine.rootContext());", engine)

    def test_appcontroller_prunes_auth_qml_compatibility_but_keeps_auth_composition_getter_and_private_targets(self):
        header = read(APP / "controller/AppController.h")
        normalized_header = normalized(header)
        public = appcontroller_public_section()
        private = appcontroller_private_section()

        self.assertNotIn("Q_PROPERTY(AuthViewModel* auth READ auth CONSTANT)", header)
        self.assertNotIn("AuthViewModel* auth();", header)
        self.assertIn("AuthViewModel* authViewModel();", public)

        pruned_auth_properties = (
            "Q_PROPERTY(QString tipText READ tipText NOTIFY tipChanged)",
            "Q_PROPERTY(bool tipError READ tipError NOTIFY tipChanged)",
            "Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)",
            "Q_PROPERTY(bool registerSuccessPage READ registerSuccessPage NOTIFY registerSuccessPageChanged)",
            "Q_PROPERTY(int registerCountdown READ registerCountdown NOTIFY registerCountdownChanged)",
            "Q_PROPERTY(int registerCodeCooldownSeconds READ registerCodeCooldownSeconds NOTIFY registerCodeCooldownChanged)",
            "Q_PROPERTY(bool registerCodeRequestPending READ registerCodeRequestPending NOTIFY registerCodeCooldownChanged)",
            "Q_PROPERTY(QString loginCredentialCacheJson READ loginCredentialCacheJson NOTIFY loginCredentialCacheChanged)",
        )
        for token in pruned_auth_properties:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        shell_methods = ("void clearTip();",)
        for token in shell_methods:
            with self.subTest(token=token):
                self.assertIn(token, normalized_header)
                self.assertIn(token, normalized(private))
                self.assertNotIn(token, normalized(public))
                self.assertNotIn(f"Q_INVOKABLE {token}", normalized_header)

        pruned_auth_command_methods = (
            "QString loginCredentialCacheJson() const;",
            "void saveLoginCredential(const QString& email, const QString& password);",
            "void login(const QString& email, const QString& password);",
            "void requestRegisterCode(const QString& email);",
            "void registerUser(const QString& user, const QString& email, const QString& password, "
            "const QString& confirm, const QString& verifyCode);",
            "void requestResetCode(const QString& email);",
            "void resetPassword(const QString& user, const QString& email, const QString& password, "
            "const QString& verifyCode);",
        )
        for token in pruned_auth_command_methods:
            with self.subTest(token=token):
                self.assertNotIn(token, normalized(private))
                self.assertNotIn(token, normalized(public))

        controller = normalized(read(APP / "controller/AppController.cpp"))
        port_binder = normalized(read(APP / "composition/AppAuthFeaturePortBinder.cpp"))
        forbidden_connections = (
            "connect(&_features.authViewModel, &AuthViewModel::clearTipRequested, this, &AppController::clearTip);",
            "connect(&_features.authViewModel, &AuthViewModel::loginRequested, this, &AppController::login);",
            "connect(&_features.authViewModel, &AuthViewModel::registerCodeRequested, this, &AppController::requestRegisterCode);",
            "connect(&_features.authViewModel, &AuthViewModel::resetPasswordRequested, this, &AppController::resetPassword);",
        )
        for token in forbidden_connections:
            with self.subTest(token=token):
                self.assertNotIn(token, controller)
        self.assertIn("_features.authViewModel.setCommandPort(AuthCommandPort{", port_binder)
        self.assertNotIn("_features.authViewModel.setCommandPort(AuthCommandPort{", controller)
        self.assertNotIn("_auth_credential_store", controller)

        auth_viewmodel = normalized(read(FEATURES / "auth/viewmodel/AuthViewModel.cpp"))
        self.assertIn("Q_UNUSED(password)", auth_viewmodel)
        self.assertIn("_credentialStore.saveLoginCredential(email, QString());", auth_viewmodel)

    def test_appcontroller_prunes_feature_object_qml_properties_but_keeps_cpp_getters(self):
        header = read(APP / "controller/AppController.h")

        pruned_properties = (
            "Q_PROPERTY(MomentsModel* momentsModel READ momentsModel CONSTANT)",
            "Q_PROPERTY(MomentsController* momentsController READ momentsController CONSTANT)",
            "Q_PROPERTY(AgentController* agentController READ agentController CONSTANT)",
            "Q_PROPERTY(AgentMessageModel* agentMessageModel READ agentMessageModel CONSTANT)",
            "Q_PROPERTY(PetController* petController READ petController CONSTANT)",
            "Q_PROPERTY(R18Controller* r18Controller READ r18Controller CONSTANT)",
        )
        for token in pruned_properties:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        kept_getters = (
            "MomentsModel* momentsModel() const;",
            "MomentsController* momentsController() const;",
            "AgentController* agentController() const;",
            "AgentMessageModel* agentMessageModel() const;",
            "PetController* petController() const;",
            "R18Controller* r18Controller() const;",
        )
        for token in kept_getters:
            with self.subTest(token=token):
                self.assertIn(token, header)

    def test_qml_uses_direct_feature_contexts_instead_of_appcontroller_feature_objects(self):
        forbidden_pattern = re.compile(
            r"controller\."
            r"(agentController|agentMessageModel|momentsController|momentsModel|petController|r18Controller)"
        )
        failures = []
        for name, source in qml_sources().items():
            for match in forbidden_pattern.finditer(source):
                failures.append(f"{name}: {match.group(0)}")

        self.assertEqual([], failures)

        shared_main = read(QML_ROOT / "app/Main.qml")
        linux_main = read(QML_ROOT / "linux/Main.qml")
        shell_page = read(QML_ROOT / "app/ChatShellPage.qml")
        shell_content = read(CHAT_VIEW / "ChatShellContent.qml")
        normal_face = read(CHAT_VIEW / "ChatNormalFace.qml")
        agent_overlay = read(AGENT_VIEW / "AgentGameOverlay.qml")

        for source_name, source in (("shared main", shared_main), ("linux main", linux_main)):
            with self.subTest(source=source_name):
                self.assertIn('"petController": pet', source)
                self.assertIn('"agentController": agent', source)

        for token in (
            "if (agent) {",
            "agent.refreshModelList()",
            "agent.createSession()",
            "if (moments) {",
            "moments.loadFeedForAuthor(uid)",
            "r18Controller: r18",
        ):
            with self.subTest(token=token):
                self.assertIn(token, shell_page)

        for token in (
            "agentController: agent",
            "messageModel: agentMessages",
            "petController: pet",
            "momentsModel: root.momentsDataModelContext",
            "momentsController: root.momentsControllerContext",
        ):
            with self.subTest(token=token):
                self.assertIn(token, shell_content)

        for token in (
            "agentSessions: agent ? agent.sessions : []",
            "agentBusy: agent ? (agent.loading",
            "agent.loadSessions()",
            "agent.deleteGameRoom(roomId)",
        ):
            with self.subTest(token=token):
                self.assertIn(token, normal_face)

        for token in (
            "agentController: agent",
            "availableModels: agent ? agent.availableModels : []",
            "gameRooms: agent ? agent.gameRooms : []",
            'gameError: agent ? agent.gameError : ""',
        ):
            with self.subTest(token=token):
                self.assertIn(token, agent_overlay)

    def test_auth_qml_uses_auth_context_instead_of_appcontroller_auth_compatibility(self):
        forbidden_pattern = re.compile(
            r"controller\."
            r"(tipText|tipError|busy|registerSuccessPage|registerCountdown|"
            r"registerCodeCooldownSeconds|registerCodeRequestPending|loginCredentialCacheJson|"
            r"clearTip|saveLoginCredential|login\s*\(|requestRegisterCode|registerUser|"
            r"requestResetCode|resetPassword)"
        )
        paths = (
            QML_ROOT / "app/Main.qml",
            QML_ROOT / "linux/Main.qml",
            AUTH_VIEW / "RegisterPage.qml",
            AUTH_VIEW / "ResetPage.qml",
        )
        failures = []
        for path in paths:
            source = read(path)
            for match in forbidden_pattern.finditer(source):
                failures.append(f"{path.relative_to(CLIENT).as_posix()}: {match.group(0)}")

        self.assertEqual([], failures)

        for path in (QML_ROOT / "app/Main.qml", QML_ROOT / "linux/Main.qml"):
            source = read(path)
            with self.subTest(path=path.name):
                self.assertIn("credentialProvider: auth", source)
                self.assertIn("tipText: auth.tipText", source)
                self.assertIn("busy: auth.busy", source)
                self.assertIn("onClearTipRequested: auth.clearTip()", source)
                self.assertIn("auth.login(email, password)", source)

        register = read(AUTH_VIEW / "RegisterPage.qml")
        reset = read(AUTH_VIEW / "ResetPage.qml")
        for token in (
            "auth.registerCodeCooldownSeconds > 0",
            "auth.registerCodeRequestPending",
            "auth.requestRegisterCode(emailField.text)",
            "currentIndex: auth.registerSuccessPage ? 1 : 0",
            "auth.registerUser(",
            '"注册成功，" + auth.registerCountdown + " s后返回登录"',
        ):
            with self.subTest(token=token):
                self.assertIn(token, register)
        for token in (
            "text: auth.tipText",
            "enabled: !auth.busy",
            "auth.requestResetCode(emailField.text)",
            "auth.resetPassword(",
        ):
            with self.subTest(token=token):
                self.assertIn(token, reset)


if __name__ == "__main__":
    unittest.main()
