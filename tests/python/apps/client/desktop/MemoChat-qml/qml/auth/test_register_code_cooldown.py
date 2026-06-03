import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
APP_DIR = CLIENT_DIR / "app"
AUTH_VIEWMODEL_HEADER = CLIENT_DIR / "features/auth/viewmodel/AuthViewModel.h"
REGISTER_PAGE = CLIENT_DIR / "features/auth/view/RegisterPage.qml"
APP_CONTROLLER_HEADER = APP_DIR / "controller/AppController.h"
APP_CONTROLLER_STATE = APP_DIR / "controller/AppControllerRuntimeState.h"
APP_CONTROLLER_PROPERTIES = APP_DIR / "controller/AppControllerUiProperties.cpp"
APP_CONTROLLER_STATUS = APP_DIR / "controller/AppControllerStatusState.cpp"
APP_CONTROLLER_AUTH_RESPONSES = APP_DIR / "controller/AppControllerAuthResponses.cpp"
SESSION_COMMANDS = APP_DIR / "session/SessionAuthCoordinatorCommands.cpp"
SESSION_RESPONSES = APP_DIR / "session/SessionAuthCoordinatorAuthResponses.cpp"
COORDINATORS_HEADER = APP_DIR / "coordinators/AppCoordinators.h"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class RegisterCodeCooldownContractTests(unittest.TestCase):
    def test_auth_viewmodel_exposes_register_code_cooldown_state_to_qml(self):
        auth_header = read(AUTH_VIEWMODEL_HEADER)
        header = read(APP_CONTROLLER_HEADER)
        state = read(APP_CONTROLLER_STATE)
        properties = read(APP_CONTROLLER_PROPERTIES)
        status = read(APP_CONTROLLER_STATUS)
        responses = read(APP_CONTROLLER_AUTH_RESPONSES)
        coordinators = read(COORDINATORS_HEADER)

        self.assertIn(
            "Q_PROPERTY(int registerCodeCooldownSeconds READ registerCodeCooldownSeconds NOTIFY stateChanged)",
            auth_header,
        )
        self.assertIn(
            "Q_PROPERTY(bool registerCodeRequestPending READ registerCodeRequestPending NOTIFY stateChanged)",
            auth_header,
        )
        self.assertIn("int registerCodeCooldownSeconds() const;", header)
        self.assertIn("bool registerCodeRequestPending() const;", header)
        self.assertIn("void registerCodeCooldownChanged();", header)
        self.assertIn("void setRegisterCodeCooldownSeconds(int seconds);", header)
        self.assertIn("void setRegisterCodeRequestPending(bool pending);", header)
        self.assertIn("QTimer _register_code_cooldown_timer;", header)

        self.assertIn("int registerCodeCooldownSeconds = 0;", state)
        self.assertIn("bool registerCodeRequestPending = false;", state)
        self.assertIn("int AppController::registerCodeCooldownSeconds() const", properties)
        self.assertIn("bool AppController::registerCodeRequestPending() const", properties)
        self.assertIn("void AppController::setRegisterCodeCooldownSeconds(int seconds)", status)
        self.assertIn("void AppController::setRegisterCodeRequestPending(bool pending)", status)
        self.assertGreaterEqual(status.count("emit registerCodeCooldownChanged();"), 2)
        self.assertIn("void AppController::onRegisterCodeCooldownTimeout()", responses)
        self.assertIn("void onRegisterCodeCooldownTimeout();", coordinators)

    def test_register_code_request_is_guarded_before_http_send(self):
        source = read(SESSION_COMMANDS)
        request_body = source.split("void SessionAuthCoordinator::requestRegisterCode(const QString& email)", 1)[
            1
        ].split("void SessionAuthCoordinator::registerUser", 1)[0]

        guard_pos = request_body.find("_shell_state.registerCodeRequestPending")
        cooldown_guard_pos = request_body.find("_shell_state.registerCodeCooldownSeconds > 0")
        pending_set_pos = request_body.find("setRegisterCodeRequestPending(true)")
        cooldown_set_pos = request_body.find("setRegisterCodeCooldownSeconds(kRegisterCodeCooldownSeconds)")
        timer_start_pos = request_body.find("_register_code_cooldown_timer.start(1000)")
        send_pos = request_body.find("sendVerifyCode(email, Modules::REGISTERMOD)")

        for label, pos in (
            ("pending guard", guard_pos),
            ("cooldown guard", cooldown_guard_pos),
            ("pending setter", pending_set_pos),
            ("cooldown setter", cooldown_set_pos),
            ("cooldown timer start", timer_start_pos),
            ("HTTP send", send_pos),
        ):
            with self.subTest(label=label):
                self.assertNotEqual(-1, pos)

        self.assertLess(guard_pos, send_pos)
        self.assertLess(cooldown_guard_pos, send_pos)
        self.assertLess(pending_set_pos, send_pos)
        self.assertLess(cooldown_set_pos, send_pos)
        self.assertLess(timer_start_pos, send_pos)
        self.assertIn("constexpr int kRegisterCodeCooldownSeconds = 60;", source)
        self.assertIn("void SessionAuthCoordinator::onRegisterCodeCooldownTimeout()", source)

        response_source = read(SESSION_RESPONSES)
        self.assertIn("setRegisterCodeCooldownSeconds(0)", response_source)
        self.assertIn("_register_code_cooldown_timer.stop()", response_source)
        success_get_code_block = response_source.split('_app.setTip("验证码已发送到邮箱，注意查收", false);', 1)[
            0
        ].rsplit("if (id == ReqId::ID_GET_VARIFY_CODE)", 1)[1]
        self.assertIn("setRegisterCodeRequestPending(false)", success_get_code_block)

    def test_register_page_disables_code_button_and_shows_countdown(self):
        qml = read(REGISTER_PAGE)
        code_section = qml.split("id: registerCodeBtn", 1)[1].split("id: registerSubmitBtn", 1)[0]

        self.assertIn("property bool registerCodeCoolingDown", qml)
        self.assertIn("auth.registerCodeCooldownSeconds > 0", qml)
        self.assertIn("property bool registerCodeUnavailable", qml)
        self.assertIn("auth.registerCodeRequestPending", qml)
        self.assertIn("registerRoot.registerCodeUnavailable", code_section)
        self.assertIn('auth.registerCodeCooldownSeconds + "s"', code_section)
        self.assertIn("registerRoot.requestRegisterCode()", qml)
        self.assertIn("if (registerRoot.registerCodeUnavailable)", qml)
        self.assertIn("auth.requestRegisterCode(emailField.text)", qml)
        self.assertIn("onClicked: registerRoot.requestRegisterCode()", code_section)
        self.assertNotRegex(code_section, r"onClicked:\s*auth\.requestRegisterCode\s*\(")

        width_match = re.search(r"Layout\.preferredWidth:\s*(\d+)", code_section)
        self.assertIsNotNone(width_match)
        self.assertGreaterEqual(int(width_match.group(1)), 72)


if __name__ == "__main__":
    unittest.main()
