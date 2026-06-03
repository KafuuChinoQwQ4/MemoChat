import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
APP = CLIENT / "app"
FEATURES = CLIENT / "features"
QML_ROOT = CLIENT / "qml"
SHELL_DIR = APP / "shell"


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
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")

        self.assertIn('setContextProperty("shell", controller.shellViewModel())', engine)
        self.assertNotIn('setContextProperty("controller"', engine)

    def test_appcontroller_owns_shell_facade_without_shell_qml_compatibility(self):
        header = read(APP / "controller/AppController.h")
        controller = normalized(read(APP / "controller/AppController.cpp"))

        self.assertIn('#include "ShellViewModel.h"', header)
        self.assertNotIn("Q_PROPERTY(ShellViewModel* shell READ shell CONSTANT)", header)
        self.assertIn("ShellViewModel* shell();", header)
        self.assertIn("ShellViewModel* shellViewModel();", header)
        self.assertIn("ShellViewModel _shell_view_model;", header)
        self.assertIn("void syncShellViewModelState();", header)

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
            "QVariantMap contactProfileByUid(int uid) const;",
        )
        for token in plain_methods:
            with self.subTest(token=token):
                self.assertIn(token, header)
                self.assertNotIn(f"Q_INVOKABLE {token}", header)

        expected_connections = (
            "connect(&_shell_view_model, &ShellViewModel::switchToLoginRequested, this, &AppController::switchToLogin);",
            "connect(&_shell_view_model, &ShellViewModel::switchToRegisterRequested, this, &AppController::switchToRegister);",
            "connect(&_shell_view_model, &ShellViewModel::switchToResetRequested, this, &AppController::switchToReset);",
            "connect(&_shell_view_model, &ShellViewModel::switchChatTabRequested, this, &AppController::switchChatTab);",
            "connect(&_shell_view_model, &ShellViewModel::beginPostLoginBootstrapRequested, this, &AppController::beginPostLoginBootstrap);",
            "connect(&_shell_view_model, &ShellViewModel::openExternalResourceRequested, this, &AppController::openExternalResource);",
        )
        for token in expected_connections:
            with self.subTest(token=token):
                self.assertIn(token, controller)

        sync_tokens = (
            "void AppController::syncShellViewModelState()",
            "_shell_view_model.syncPage(static_cast<int>(_page));",
            "_shell_view_model.syncChatTab(static_cast<int>(_chat_tab));",
            "_shell_view_model.syncCurrentUser(currentUserName(), currentUserNick(), currentUserIcon(), "
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
        navigation = normalized(read(APP / "controller/AppControllerNavigation.cpp"))

        self.assertIn("const int previous_user_uid = currentUserUid();", navigation)
        self.assertIn("const bool userChanged = previous_user_uid != 0 ||", navigation)
        self.assertLess(
            navigation.index("const int previous_user_uid = currentUserUid();"),
            navigation.index("_gateway.userMgr()->ResetSession();"),
        )
        self.assertLess(
            navigation.index("const bool userChanged = previous_user_uid != 0 ||"),
            navigation.index("syncShellViewModelState(); emit currentUserChanged();"),
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
                "readonly property bool chatPageActive: shell.page === AppController.ChatPage",
                "shell.beginPostLoginBootstrap()",
                '"selfAvatar": shell.currentUserIcon',
                "shell.switchToRegister()",
                "visible: shell.page === AppController.LoginPage",
            ),
            "qml/linux/Main.qml": (
                "readonly property bool chatPageActive: shell.page === AppController.ChatPage",
                "shell.beginPostLoginBootstrap()",
                '"selfAvatar": shell.currentUserIcon',
                "shell.switchToReset()",
                "visible: shell.page === AppController.ResetPage",
            ),
            "qml/app/ChatShellPage.qml": (
                "property int lastMainTab: shell.chatTab",
                "shell.switchChatTab(AppController.ChatTabPage)",
                "shell.switchToLogin()",
            ),
            "features/chat/view/ChatNormalFace.qml": (
                "userIcon: shell.currentUserIcon",
                "shell.switchChatTab(tab)",
                "userNick: shell.currentUserNick",
                "userId: shell.currentUserId",
            ),
            "features/chat/view/ChatShellContent.qml": (
                "shell.chatTab === AppController.ChatTabPage",
                "selfName: shell.currentUserNick",
                "selfAvatar: shell.currentUserIcon",
                "shell.openExternalResource(url)",
                "shell.chatTab === AppController.ContactTabPage",
            ),
            "features/agent/view/AgentGameOverlay.qml": (
                "shell.chatTab === AppController.AgentTabPage",
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

    def test_appcontroller_enum_registration_remains_for_qml_constants(self):
        registry = read(APP / "bootstrap/MainQmlTypeRegistry.cpp")

        self.assertIn(
            'qmlRegisterUncreatableType<AppController>("MemoChat", 1, 0, "AppController", "Enum only")', registry
        )


if __name__ == "__main__":
    unittest.main()
