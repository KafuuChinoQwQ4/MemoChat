import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
APP = CLIENT / "app"
APP_FEATURE_REGISTRY_H = APP / "composition/AppFeatureRegistry.h"
FEATURES = CLIENT / "features"
QML_ROOT = CLIENT / "qml"
SHELL_DIR = APP / "shell"
SESSION_LOGOUT = APP / "session/AppSessionCoordinatorLogout.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def app_owned_reset_text() -> str:
    roots = (
        APP / "controller",
        APP / "session",
        APP / "coordinators",
    )
    sources: list[str] = []
    for root in roots:
        for path in sorted(root.rglob("*")):
            if path.suffix in {".cpp", ".h", ".hpp", ".cc", ".cxx"}:
                sources.append(read(path))
    return "\n".join(sources)


def normalized(source: str) -> str:
    return " ".join(source.split())


def extract_cpp_function(source: str, signature: str) -> str:
    start = source.index(signature)
    open_brace = source.index("{", start)
    depth = 0
    for index in range(open_brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"Function not found: {signature}")


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


class ShellViewModelContractTests(unittest.TestCase):
    def test_shell_viewmodel_files_are_registered(self):
        shell_header = SHELL_DIR / "ShellViewModel.h"
        shell_source = SHELL_DIR / "ShellViewModel.cpp"
        app_sources = read(APP / "sources.cmake")
        cmake = read(CLIENT / "CMakeLists.txt")

        self.assertTrue(shell_header.is_file())
        self.assertTrue(shell_source.is_file())
        self.assertIn("app/shell/ShellViewModel.cpp", app_sources)
        self.assertIn("app/shell/ShellViewModel.h", app_sources)
        self.assertIn("${CMAKE_CURRENT_SOURCE_DIR}/app/shell", cmake)

    def test_shell_viewmodel_exposes_shell_user_contract(self):
        header = read(SHELL_DIR / "ShellViewModel.h")
        source = read(SHELL_DIR / "ShellViewModel.cpp")
        compact_header = normalized(header)
        compact_source = normalized(source)

        properties = (
            "Q_PROPERTY(int page READ page NOTIFY pageChanged)",
            "Q_PROPERTY(int chatTab READ chatTab NOTIFY chatTabChanged)",
            "Q_PROPERTY(QString currentUserName READ currentUserName NOTIFY currentUserChanged)",
            "Q_PROPERTY(QString currentUserNick READ currentUserNick NOTIFY currentUserChanged)",
            "Q_PROPERTY(QString currentUserIcon READ currentUserIcon NOTIFY currentUserChanged)",
            "Q_PROPERTY(QString currentUserDesc READ currentUserDesc NOTIFY currentUserChanged)",
            "Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY currentUserChanged)",
            "Q_PROPERTY(int currentUserUid READ currentUserUid NOTIFY currentUserChanged)",
        )
        for token in properties:
            with self.subTest(token=token):
                self.assertIn(token, header)

        invokables = (
            "Q_INVOKABLE void switchToLogin();",
            "Q_INVOKABLE void switchToRegister();",
            "Q_INVOKABLE void switchToReset();",
            "Q_INVOKABLE void switchChatTab(int tab);",
            "Q_INVOKABLE void beginPostLoginBootstrap();",
            "Q_INVOKABLE void openExternalResource(const QString& url);",
        )
        for token in invokables:
            with self.subTest(token=token):
                self.assertIn(token, header)

        sync_methods = (
            "void syncPage(int page);",
            "void syncChatTab(int chatTab);",
            "void syncCurrentUser(const QString& name, const QString& nick, const QString& icon, "
            "const QString& desc, const QString& userId, int uid);",
        )
        for token in sync_methods:
            with self.subTest(token=token):
                self.assertIn(token, compact_header)

        request_signals = (
            "void switchToLoginRequested();",
            "void switchToRegisterRequested();",
            "void switchToResetRequested();",
            "void switchChatTabRequested(int tab);",
            "void beginPostLoginBootstrapRequested();",
            "void openExternalResourceRequested(const QString& url);",
        )
        for token in request_signals:
            with self.subTest(token=token):
                self.assertIn(token, header)

        emitted_requests = (
            "emit switchToLoginRequested();",
            "emit switchToRegisterRequested();",
            "emit switchToResetRequested();",
            "emit switchChatTabRequested(tab);",
            "emit beginPostLoginBootstrapRequested();",
            "emit openExternalResourceRequested(url);",
        )
        for token in emitted_requests:
            with self.subTest(token=token):
                self.assertIn(token, compact_source)

    def test_engine_exposes_shell_context_and_removes_controller_context(self):
        composition = read(APP / "composition/AppComposition.cpp")
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")

        self.assertIn('context.setContextProperty("shell", shell())', composition)
        self.assertIn("composition.configureQmlContext(*engine.rootContext());", engine)
        self.assertNotIn('setContextProperty("controller"', engine)

    def test_appcontroller_owns_shell_facade_without_shell_qml_or_public_command_compatibility(self):
        header = read(APP / "controller/AppController.h")
        registry = read(APP_FEATURE_REGISTRY_H)
        controller = normalized(read(APP / "controller/AppController.cpp"))
        signal_binder = normalized(read(APP / "composition/AppShellSignalBinder.cpp"))
        public = appcontroller_public_section()
        private = appcontroller_private_section()

        self.assertIn('#include "AppFeatureRegistry.h"', header)
        self.assertIn('#include "ShellViewModel.h"', registry)
        self.assertNotIn("Q_PROPERTY(ShellViewModel* shell READ shell CONSTANT)", header)
        self.assertNotIn("ShellViewModel* shell();", header)
        self.assertIn("ShellViewModel* shellViewModel();", public)
        self.assertIn("AppFeatureRegistry _features", header)
        self.assertIn("ShellViewModel shellViewModel;", registry)
        self.assertIn("void syncShellViewModelState();", private)

        pruned_properties = (
            "Q_PROPERTY(Page page READ page NOTIFY pageChanged)",
            "Q_PROPERTY(ChatTab chatTab READ chatTab NOTIFY chatTabChanged)",
            "Q_PROPERTY(QString currentUserName READ currentUserName NOTIFY currentUserChanged)",
            "Q_PROPERTY(QString currentUserNick READ currentUserNick NOTIFY currentUserChanged)",
            "Q_PROPERTY(QString currentUserIcon READ currentUserIcon NOTIFY currentUserChanged)",
            "Q_PROPERTY(QString currentUserDesc READ currentUserDesc NOTIFY currentUserChanged)",
            "Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY currentUserChanged)",
            "Q_PROPERTY(int currentUserUid READ currentUserUid NOTIFY currentUserChanged)",
        )
        for token in pruned_properties:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        plain_methods = (
            "void switchToLogin();",
            "void switchToRegister();",
            "void switchToReset();",
            "void switchChatTab(int tab);",
            "void beginPostLoginBootstrap();",
            "void openExternalResource(const QString& url);",
        )
        for token in plain_methods:
            with self.subTest(token=token):
                self.assertIn(token, header)
                self.assertIn(token, private)
                self.assertNotIn(token, public)
                self.assertNotIn(f"Q_INVOKABLE {token}", header)

        expected_connections = (
            "connect(&_features.shellViewModel, &ShellViewModel::switchToLoginRequested, this, &AppController::switchToLogin);",
            "connect(&_features.shellViewModel, &ShellViewModel::switchToRegisterRequested, this, &AppController::switchToRegister);",
            "connect(&_features.shellViewModel, &ShellViewModel::switchToResetRequested, this, &AppController::switchToReset);",
            "connect(&_features.shellViewModel, &ShellViewModel::switchChatTabRequested, this, &AppController::switchChatTab);",
            "connect(&_features.shellViewModel, &ShellViewModel::beginPostLoginBootstrapRequested, this, &AppController::beginPostLoginBootstrap);",
            "connect(&_features.shellViewModel, &ShellViewModel::openExternalResourceRequested, this, &AppController::openExternalResource);",
        )
        for token in expected_connections:
            with self.subTest(token=token):
                self.assertIn(token, signal_binder)
                self.assertNotIn(token, controller)

        self.assertIn("void bindAppControllerSignals();", private)
        self.assertIn("bindAppControllerSignals();", controller)

        sync_tokens = (
            "void AppController::syncShellViewModelState()",
            "_features.shellViewModel.syncPage(_shell_state.page());",
            "_features.shellViewModel.syncChatTab(_shell_state.chatTab());",
            "_features.shellViewModel.syncCurrentUser(currentUserName(), currentUserNick(), currentUserIcon(), "
            "currentUserDesc(), currentUserId(), currentUserUid());",
        )
        for token in sync_tokens:
            with self.subTest(token=token):
                self.assertIn(token, controller)

    def test_contact_profile_popup_uses_contact_facade_not_appcontroller(self):
        contact_header = read(FEATURES / "contact/controller/ContactController.h")
        contact_source = read(FEATURES / "contact/controller/ContactController.cpp")
        popup = read(FEATURES / "contact/view/ContactProfilePopup.qml")
        modal = read(FEATURES / "chat/view/ChatModalLayer.qml")
        moments_feed = read(FEATURES / "moments/view/MomentsFeedPane.qml")
        shell_page = read(QML_ROOT / "app/ChatShellPage.qml")
        shell_content = read(FEATURES / "chat/view/ChatShellContent.qml")

        self.assertIn("Q_INVOKABLE QVariantMap contactProfileByUid(int uid) const;", contact_header)
        self.assertIn("QVariantMap ContactController::contactProfileByUid(int uid) const", contact_source)
        self.assertIn("property var contactController: null", popup)
        self.assertIn("contactController ? contactController.contactProfileByUid(profileUid) : ({})", popup)
        self.assertIn("contactController.deleteFriend(root.profileUid)", popup)
        self.assertNotIn("property var appController", popup)
        self.assertNotIn("appController.", popup)

        for source_name, source in (
            ("ChatModalLayer", modal),
            ("MomentsFeedPane", moments_feed),
            ("ChatShellPage", shell_page),
            ("ChatShellContent", shell_content),
        ):
            with self.subTest(source=source_name):
                self.assertNotIn("appController", source)
                self.assertNotIn("appController:", source)
                self.assertNotIn("property var appController", source)

        self.assertIn("contactController: contact", shell_page)
        self.assertIn("contactController: contact", shell_content)
        self.assertIn("contactController: root.contactController", modal)
        self.assertIn("contactController: root.contactController", moments_feed)
        self.assertIn("property int currentUserUid: 0", moments_feed)
        self.assertIn("currentUserUid: shell.currentUserUid", shell_content)

    def test_switch_to_login_syncs_shell_when_only_user_uid_changes(self):
        reset_owner = normalized(
            extract_cpp_function(read(SESSION_LOGOUT), "void AppSessionCoordinator::resetForLogout")
        )
        app_controller = normalized(read(APP / "controller/AppController.cpp"))
        app_reset_ports = normalized(read(APP / "composition/AppSessionLogoutPortBinder.cpp"))

        self.assertIn(
            "const int previousUserUid = _logout_port.currentUserUid ? _logout_port.currentUserUid() : 0;",
            reset_owner,
        )
        self.assertIn("_shell_state.resetCurrentUser(previousUserUid)", app_reset_ports)
        self.assertNotIn("const bool userChanged = previousUserUid != 0 ||", app_controller)
        self.assertLess(
            reset_owner.index(
                "const int previousUserUid = _logout_port.currentUserUid ? _logout_port.currentUserUid() : 0;"
            ),
            reset_owner.index("invokeIfSet(_logout_port.closeNetworkResources);"),
        )
        self.assertLess(
            reset_owner.index("invokeIfSet(_logout_port.closeNetworkResources);"),
            reset_owner.index("invokeIfSet(_logout_port.clearCurrentUserState, previousUserUid);"),
        )
        self.assertLess(
            app_reset_ports.index("_shell_state.resetCurrentUser(previousUserUid)"),
            app_reset_ports.index("syncShellViewModelState(); emit currentUserChanged();"),
        )

    def test_qml_uses_shell_context_for_shell_and_user_surface(self):
        forbidden_pattern = re.compile(
            r"controller\."
            r"(page|chatTab|currentUserName|currentUserNick|currentUserIcon|currentUserDesc|"
            r"currentUserId|currentUserUid|switchToLogin|switchToRegister|switchToReset|"
            r"switchChatTab|beginPostLoginBootstrap|openExternalResource|contactProfileByUid|deleteFriend)"
        )

        failures = []
        for name, source in qml_sources().items():
            for match in forbidden_pattern.finditer(source):
                failures.append(f"{name}: {match.group(0)}")
            if 'setContextProperty("controller"' in source:
                failures.append(f'{name}: setContextProperty("controller")')

        self.assertEqual([], failures)

        expected_shell_tokens = {
            "qml/app/Main.qml": (
                "readonly property bool chatPageActive: shell.page === ShellViewModel.ChatPage",
                "shell.beginPostLoginBootstrap()",
                '"selfAvatar": shell.currentUserIcon',
                "shell.switchToRegister()",
                "visible: shell.page === ShellViewModel.LoginPage",
            ),
            "qml/linux/Main.qml": (
                "readonly property bool chatPageActive: shell.page === ShellViewModel.ChatPage",
                "shell.beginPostLoginBootstrap()",
                '"selfAvatar": shell.currentUserIcon',
                "shell.switchToReset()",
                "visible: shell.page === ShellViewModel.ResetPage",
            ),
            "qml/app/ChatShellPage.qml": (
                "property int lastMainTab: shell.chatTab",
                "shell.switchChatTab(ShellViewModel.ChatTabPage)",
                "shell.switchToLogin()",
            ),
            "features/chat/view/ChatNormalFace.qml": (
                "userIcon: shell.currentUserIcon",
                "shell.switchChatTab(tab)",
                "userNick: shell.currentUserNick",
                "userId: shell.currentUserId",
            ),
            "features/chat/view/ChatShellContent.qml": (
                "shell.chatTab === ShellViewModel.ChatTabPage",
                "selfName: shell.currentUserNick",
                "selfAvatar: shell.currentUserIcon",
                "shell.openExternalResource(url)",
                "shell.chatTab === ShellViewModel.ContactTabPage",
            ),
            "features/agent/view/AgentGameOverlay.qml": (
                "shell.chatTab === ShellViewModel.AgentTabPage",
                "humanDisplayName: shell.currentUserNick",
            ),
            "features/auth/view/RegisterPage.qml": ("shell.switchToLogin()",),
            "features/auth/view/ResetPage.qml": ("shell.switchToLogin()",),
        }

        sources = qml_sources()
        for name, tokens in expected_shell_tokens.items():
            source = sources[name]
            for token in tokens:
                with self.subTest(name=name, token=token):
                    self.assertIn(token, source)

        self.assertNotIn("chat.chatTab", sources["features/chat/view/ChatShellContent.qml"])

    def test_shell_viewmodel_exposes_qml_enums_and_appcontroller_keeps_compatibility_boundary(self):
        shell_header = read(SHELL_DIR / "ShellViewModel.h")
        appcontroller_header = read(APP / "controller/AppController.h")
        registry = read(APP / "bootstrap/MainQmlTypeRegistry.cpp")

        enum_tokens = (
            "enum Page",
            "LoginPage = 0",
            "RegisterPage = 1",
            "ResetPage = 2",
            "ChatPage = 3",
            "Q_ENUM(Page)",
            "enum ChatTab",
            "ChatTabPage = 0",
            "ContactTabPage = 1",
            "SettingsTabPage = 2",
            "MomentsTabPage = 3",
            "AgentTabPage = 4",
            "Live2DTabPage = 5",
            "Q_ENUM(ChatTab)",
        )
        for token in enum_tokens:
            with self.subTest(token=token):
                self.assertIn(token, shell_header)

        self.assertIn(
            'qmlRegisterUncreatableType<ShellViewModel>("MemoChat", 1, 0, "ShellViewModel", "Enum only")',
            registry,
        )
        self.assertNotIn('qmlRegisterUncreatableType<AppController>("MemoChat", 1, 0, "AppController"', registry)

        self.assertIn("enum Page", appcontroller_header)
        self.assertIn("enum ChatTab", appcontroller_header)
        self.assertNotIn("Q_PROPERTY(Page page READ page NOTIFY pageChanged)", appcontroller_header)
        self.assertNotIn("Q_PROPERTY(ChatTab chatTab READ chatTab NOTIFY chatTabChanged)", appcontroller_header)

    def test_main_qml_entries_do_not_depend_on_appcontroller_enums(self):
        sources = qml_sources()
        failures = []
        for name in ("qml/app/Main.qml", "qml/linux/Main.qml"):
            source = sources[name]
            if "AppController." in source:
                failures.append(f"{name}: AppController enum reference")
            if "ShellViewModel." not in source:
                failures.append(f"{name}: missing ShellViewModel enum reference")
            if "shell." not in source and "shellViewModel." not in source:
                failures.append(f"{name}: missing shell view-model context")

        self.assertEqual([], failures)


if __name__ == "__main__":
    unittest.main()
