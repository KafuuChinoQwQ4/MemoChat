import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
APP_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app"
APP_CONTROLLER_HEADER = APP_DIR / "controller/AppController.h"
APP_CONTROLLER_SOURCE = APP_DIR / "controller/AppController.cpp"
APP_SESSION_AUTH_PORT_BINDER = APP_DIR / "composition/AppSessionAuthPortBinder.cpp"
APP_REGISTER_COUNTDOWN_PORT_BINDER = APP_DIR / "composition/AppRegisterCountdownPortBinder.cpp"
APP_SESSION_LOGOUT_PORT_BINDER = APP_DIR / "composition/AppSessionLogoutPortBinder.cpp"
APP_HTTP_EVENT_ROUTER = APP_DIR / "events/AppHttpEventRouter.cpp"
CALL_COORDINATOR_FILES = (
    APP_DIR / "coordinators/CallCoordinator.cpp",
    APP_DIR / "coordinators/CallCoordinatorCommands.cpp",
    APP_DIR / "coordinators/CallCoordinatorEvents.cpp",
    APP_DIR / "coordinators/CallCoordinatorHttp.cpp",
    APP_DIR / "coordinators/CallCoordinatorLivekit.cpp",
)
APP_COORDINATORS_HEADER = APP_DIR / "coordinators/AppCoordinators.h"
REGISTER_COUNTDOWN_CONTROLLER = APP_DIR / "coordinators/RegisterCountdownController.cpp"
SESSION_AUTH_RESPONSES = APP_DIR / "session/SessionAuthCoordinatorAuthResponses.cpp"
SESSION_AUTH_COMMANDS = APP_DIR / "session/SessionAuthCoordinatorCommands.cpp"
SESSION_AUTH_LOGIN_RESPONSE = APP_DIR / "session/SessionAuthCoordinatorLoginResponse.cpp"
SESSION_LOGOUT = APP_DIR / "session/AppSessionCoordinatorLogout.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def app_owned_reset_text() -> str:
    roots = (
        APP_DIR / "controller",
        APP_DIR / "session",
        APP_DIR / "coordinators",
    )
    sources: list[str] = []
    for root in roots:
        for path in sorted(root.rglob("*")):
            if path.suffix in {".cpp", ".h", ".hpp", ".cc", ".cxx"}:
                sources.append(read(path))
    return "\n".join(sources)


def call_coordinator_text() -> str:
    return "\n".join(read(path) for path in CALL_COORDINATOR_FILES)


class AppControllerAuthBoundaryTests(unittest.TestCase):
    def test_app_controller_header_no_longer_declares_auth_validation_helpers(self):
        header = read(APP_CONTROLLER_HEADER)
        private_start = header.index("private:")
        private_header = header[private_start:]

        forbidden_declarations = (
            "bool checkEmailValid(const QString& email);",
            "bool checkPwdValid(const QString& password);",
            "bool checkUserValid(const QString& user);",
            "bool checkVerifyCodeValid(const QString& code);",
        )
        for declaration in forbidden_declarations:
            with self.subTest(declaration=declaration):
                self.assertNotIn(declaration, private_header)

        self.assertNotIn("bool parseJson(const QString& res, QJsonObject& obj);", private_header)

    def test_app_controller_auth_no_longer_defines_validation_wrappers(self):
        self.assertFalse((APP_DIR / "controller/AppControllerAuth.cpp").exists())
        source = read(APP_CONTROLLER_SOURCE)

        forbidden_definitions = (
            "bool AppController::checkEmailValid",
            "bool AppController::checkPwdValid",
            "bool AppController::checkUserValid",
            "bool AppController::checkVerifyCodeValid",
        )
        for definition in forbidden_definitions:
            with self.subTest(definition=definition):
                self.assertNotIn(definition, source)

        self.assertNotIn("bool AppController::parseJson", source)

    def test_session_auth_coordinator_owns_validation_boundary(self):
        header = read(APP_DIR / "coordinators/AppCoordinators.h")
        source = read(SESSION_AUTH_COMMANDS)

        expected_helpers = (
            "bool checkEmailValid(const QString& email);",
            "bool checkPasswordValid(const QString& password);",
            "bool checkUserValid(const QString& user);",
            "bool checkVerifyCodeValid(const QString& code);",
        )
        for helper in expected_helpers:
            with self.subTest(helper=helper):
                self.assertIn(helper, header)

        expected_definitions = (
            "bool SessionAuthCoordinator::checkEmailValid(const QString& email)",
            "bool SessionAuthCoordinator::checkPasswordValid(const QString& password)",
            "bool SessionAuthCoordinator::checkUserValid(const QString& user)",
            "bool SessionAuthCoordinator::checkVerifyCodeValid(const QString& code)",
        )
        for definition in expected_definitions:
            with self.subTest(definition=definition):
                self.assertIn(definition, source)

        self.assertIn("_port.checkEmail(email, &errorText)", source)
        self.assertIn("_port.checkPassword(password, &errorText)", source)
        self.assertIn("_port.checkUser(user, &errorText)", source)
        self.assertIn("_port.checkVerifyCode(code, &errorText)", source)
        self.assertEqual(4, len(re.findall(r"_port\.setTip\(errorText, true\);", source)))

        forbidden_calls = (
            "_app.",
            "_app.checkEmailValid",
            "_app.checkPwdValid",
            "_app.checkUserValid",
            "_app.checkVerifyCodeValid",
        )
        for call in forbidden_calls:
            with self.subTest(call=call):
                self.assertNotIn(call, source)

    def test_session_auth_response_parsing_uses_auth_port(self):
        http_event_router = read(APP_HTTP_EVENT_ROUTER)
        port_binder = read(APP_SESSION_AUTH_PORT_BINDER)
        call_coordinator = call_coordinator_text()
        session_auth_responses = read(SESSION_AUTH_RESPONSES)
        session_login_response = read(SESSION_AUTH_LOGIN_RESPONSE)

        self.assertNotIn("_features.authController.parseJson(res, obj)", http_event_router)
        self.assertNotIn("_features.authController.parseJson(res, obj)", read(APP_CONTROLLER_SOURCE))
        self.assertIn("_features.authController.parseJson(res, obj)", port_binder)
        self.assertIn("_port.parseJson(res, obj)", call_coordinator)
        self.assertNotIn("_app._features.authController.parseJson(res, obj)", call_coordinator)
        self.assertIn("_port.parseJson(res, obj)", session_auth_responses)
        self.assertIn("_port.parseJson(res, obj)", session_login_response)

        forbidden_calls = (
            "if (!parseJson(res, obj))",
            "if (!_app.parseJson(res, obj))",
            "_app._features.authController.parseJson(res, obj)",
        )
        for source_name, source in (
            ("SessionAuthCoordinatorAuthResponses.cpp", session_auth_responses),
            ("SessionAuthCoordinatorLoginResponse.cpp", session_login_response),
        ):
            for call in forbidden_calls:
                with self.subTest(source=source_name, call=call):
                    self.assertNotIn(call, source)

    def test_session_auth_coordinator_uses_narrow_port(self):
        header = read(APP_CONTROLLER_HEADER)
        controller_source = read(APP_CONTROLLER_SOURCE)
        app_source = read(APP_SESSION_AUTH_PORT_BINDER)
        coordinators = read(APP_COORDINATORS_HEADER)
        auth_commands = read(SESSION_AUTH_COMMANDS)
        auth_responses = read(SESSION_AUTH_RESPONSES)
        auth_login = read(SESSION_AUTH_LOGIN_RESPONSE)

        self.assertNotIn("friend class SessionAuthCoordinator;", header)
        self.assertIn("struct SessionAuthPort", coordinators)
        self.assertIn("explicit SessionAuthCoordinator(SessionAuthPort port);", coordinators)
        self.assertIn("SessionAuthPort _port;", coordinators)

        auth_block = coordinators.split("class SessionAuthCoordinator", 1)[1].split(
            "class SessionRelationBootstrap", 1
        )[0]
        self.assertNotIn("AppController& _app;", auth_block)
        self.assertNotIn("AppController& controller", auth_block)
        self.assertNotIn("_app", auth_block)

        for source_name, source in (
            ("SessionAuthCoordinatorCommands.cpp", auth_commands),
            ("SessionAuthCoordinatorAuthResponses.cpp", auth_responses),
            ("SessionAuthCoordinatorLoginResponse.cpp", auth_login),
        ):
            with self.subTest(source=source_name):
                self.assertNotIn("_app.", source)
                self.assertNotIn('#include "AppController.h"', source)

        for member in (
            "std::function<bool(const QString&, QString*)> checkEmail;",
            "std::function<bool(const QString&, QJsonObject&)> parseJson;",
            "std::function<void()> prepareLoginAttempt;",
            "std::function<void(const ServerInfo&, const QJsonObject&)> applyLoginSuccess;",
        ):
            with self.subTest(member=member):
                self.assertIn(member, coordinators)

        self.assertIn("SessionAuthPort{", app_source)
        self.assertIn("_session_coordinator->resetLoginAttemptState", app_source)
        self.assertIn("_session_coordinator->setIgnoreNextLoginDisconnect", app_source)
        self.assertIn("_session_coordinator->applyLoginSuccessState", app_source)
        self.assertIn("_features.chatFeatureController.setMessageDownloadAuthContext", app_source)
        self.assertIn("setIconDownloadAuthContext", app_source)
        self.assertIn("_gateway.chatTransport()->connectToServer(serverInfo);", app_source)
        self.assertNotIn("SessionAuthPort{", controller_source)
        self.assertNotIn("_session_coordinator->applyLoginSuccessState", controller_source)

    def test_register_countdown_controller_uses_narrow_port(self):
        header = read(APP_CONTROLLER_HEADER)
        controller_source = read(APP_CONTROLLER_SOURCE)
        app_source = read(APP_REGISTER_COUNTDOWN_PORT_BINDER) + "\n" + read(APP_SESSION_LOGOUT_PORT_BINDER)
        reset_owner = app_owned_reset_text()
        coordinators = read(APP_COORDINATORS_HEADER)
        countdown = read(REGISTER_COUNTDOWN_CONTROLLER)
        logout = read(SESSION_LOGOUT)

        self.assertNotIn("friend class RegisterCountdownController;", header)
        self.assertIn("struct RegisterCountdownPort", coordinators)
        for port_member in (
            "std::function<int()> countdownSeconds;",
            "std::function<void(int)> setCountdownSeconds;",
            "std::function<void(bool)> setRegisterSuccessPage;",
            "std::function<void()> switchToLogin;",
        ):
            with self.subTest(port_member=port_member):
                self.assertIn(port_member, coordinators)
        self.assertNotIn("std::function<void()> stopTimer;", coordinators)

        self.assertIn("explicit RegisterCountdownController(RegisterCountdownPort port);", coordinators)
        self.assertIn("RegisterCountdownPort _port;", coordinators)

        controller_block = coordinators.split("class RegisterCountdownController", 1)[1].split(
            "class AppSessionCoordinator", 1
        )[0]
        self.assertNotIn("AppController& _app;", controller_block)
        self.assertNotIn("AppController& controller", controller_block)
        self.assertNotIn("_app", controller_block)
        self.assertNotIn("_app.", countdown)
        self.assertNotIn('#include "AppController.h"', countdown)

        self.assertIn("_port.countdownSeconds()", countdown)
        self.assertIn("_port.setCountdownSeconds", countdown)
        self.assertIn("stopTimer();", countdown)
        self.assertIn("_timer.stop();", countdown)
        self.assertIn("_port.setRegisterSuccessPage(false);", countdown)
        self.assertIn("_port.switchToLogin();", countdown)

        self.assertIn("RegisterCountdownPort{", app_source)
        self.assertIn("SessionLogoutPort port;", app_source)
        self.assertNotIn("RegisterCountdownPort{", controller_source)
        self.assertNotIn("SessionLogoutPort{", controller_source)
        self.assertNotIn("_register_countdown_timer", app_source + controller_source)
        self.assertIn("void AppSessionCoordinator::resetForLogout()", logout)
        self.assertIn("stopRegisterCountdownTimer();", logout)
        self.assertIn("invokeIfSet(_logout_port.resetAuthShellState);", logout)
        self.assertIn("_features.authViewModel.syncRegisterSuccessPage(false);", app_source)
        self.assertIn("std::function<void()> resetAuthShellState;", coordinators)
        self.assertIn("switchToLogin", reset_owner)


if __name__ == "__main__":
    unittest.main()
