import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
APP_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app"
APP_CONTROLLER_HEADER = APP_DIR / "controller/AppController.h"
APP_CONTROLLER_AUTH = APP_DIR / "controller/AppControllerAuth.cpp"
APP_CONTROLLER_AUTH_RESPONSES = APP_DIR / "controller/AppControllerAuthResponses.cpp"
CALL_COORDINATOR = APP_DIR / "coordinators/CallCoordinator.cpp"
SESSION_AUTH_RESPONSES = APP_DIR / "session/SessionAuthCoordinatorAuthResponses.cpp"
SESSION_AUTH_COMMANDS = APP_DIR / "session/SessionAuthCoordinatorCommands.cpp"
SESSION_AUTH_LOGIN_RESPONSE = APP_DIR / "session/SessionAuthCoordinatorLoginResponse.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


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
        source = read(APP_CONTROLLER_AUTH)

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

        self.assertIn("_app._auth_controller.checkEmail(email, &errorText)", source)
        self.assertIn("_app._auth_controller.checkPassword(password, &errorText)", source)
        self.assertIn("_app._auth_controller.checkUser(user, &errorText)", source)
        self.assertIn("_app._auth_controller.checkVerifyCode(code, &errorText)", source)
        self.assertEqual(4, len(re.findall(r"_app\.setTip\(errorText, true\);", source)))

        forbidden_calls = (
            "_app.checkEmailValid",
            "_app.checkPwdValid",
            "_app.checkUserValid",
            "_app.checkVerifyCodeValid",
        )
        for call in forbidden_calls:
            with self.subTest(call=call):
                self.assertNotIn(call, source)

    def test_response_parsing_calls_auth_controller_directly(self):
        app_auth_responses = read(APP_CONTROLLER_AUTH_RESPONSES)
        call_coordinator = read(CALL_COORDINATOR)
        session_auth_responses = read(SESSION_AUTH_RESPONSES)
        session_login_response = read(SESSION_AUTH_LOGIN_RESPONSE)

        self.assertIn("_auth_controller.parseJson(res, obj)", app_auth_responses)
        self.assertIn("_app._auth_controller.parseJson(res, obj)", call_coordinator)
        self.assertIn("_app._auth_controller.parseJson(res, obj)", session_auth_responses)
        self.assertIn("_app._auth_controller.parseJson(res, obj)", session_login_response)

        forbidden_calls = (
            "if (!parseJson(res, obj))",
            "if (!_app.parseJson(res, obj))",
        )
        for source_name, source in (
            ("AppControllerAuthResponses.cpp", app_auth_responses),
            ("CallCoordinator.cpp", call_coordinator),
            ("SessionAuthCoordinatorAuthResponses.cpp", session_auth_responses),
            ("SessionAuthCoordinatorLoginResponse.cpp", session_login_response),
        ):
            for call in forbidden_calls:
                with self.subTest(source=source_name, call=call):
                    self.assertNotIn(call, source)


if __name__ == "__main__":
    unittest.main()
