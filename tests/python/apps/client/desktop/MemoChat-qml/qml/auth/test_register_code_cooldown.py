import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
APP_DIR = CLIENT_DIR / "app"
AUTH_VIEWMODEL_HEADER = CLIENT_DIR / "features/auth/viewmodel/AuthViewModel.h"
AUTH_STATE_HEADER = CLIENT_DIR / "features/auth/model/AuthState.h"
REGISTER_PAGE = CLIENT_DIR / "features/auth/view/RegisterPage.qml"
APP_CONTROLLER_HEADER = APP_DIR / "controller/AppController.h"
SESSION_COMMANDS = APP_DIR / "session/SessionAuthCoordinatorCommands.cpp"
SESSION_RESPONSES = APP_DIR / "session/SessionAuthCoordinatorAuthResponses.cpp"
SESSION_AUTH = APP_DIR / "session/SessionAuthCoordinator.cpp"
COORDINATORS_HEADER = APP_DIR / "coordinators/AppCoordinators.h"
REGISTER_COUNTDOWN_CONTROLLER = APP_DIR / "coordinators/RegisterCountdownController.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class RegisterCodeCooldownContractTests(unittest.TestCase):
    def test_auth_viewmodel_exposes_register_code_cooldown_state_to_qml(self):
        auth_header = read(AUTH_VIEWMODEL_HEADER)
        auth_state = read(AUTH_STATE_HEADER)
        header = read(APP_CONTROLLER_HEADER)
        coordinators = read(COORDINATORS_HEADER)
        session_auth = read(SESSION_AUTH)

        self.assertIn(
            "Q_PROPERTY(int registerCodeCooldownSeconds READ registerCodeCooldownSeconds NOTIFY stateChanged)",
            auth_header,
        )
        self.assertIn(
            "Q_PROPERTY(bool registerCodeRequestPending READ registerCodeRequestPending NOTIFY stateChanged)",
            auth_header,
        )
        self.assertNotIn("int registerCodeCooldownSeconds() const;", header)
        self.assertNotIn("bool registerCodeRequestPending() const;", header)
        self.assertNotIn("void registerCodeCooldownChanged();", header)
        self.assertNotIn("void setRegisterCodeCooldownSeconds(int seconds);", header)
        self.assertNotIn("void setRegisterCodeRequestPending(bool pending);", header)
        self.assertNotIn("QTimer _register_code_cooldown_timer;", header)
        private_header = header[header.index("private:") :]
        public_header = header[: header.index("private:")]
        self.assertNotIn("int registerCodeCooldownSeconds() const;", public_header)
        self.assertNotIn("bool registerCodeRequestPending() const;", public_header)
        self.assertNotIn("int registerCodeCooldownSeconds() const;", private_header)
        self.assertNotIn("bool registerCodeRequestPending() const;", private_header)

        self.assertIn("int registerCodeCooldownSeconds = 0;", auth_state)
        self.assertIn("bool registerCodeRequestPending = false;", auth_state)
        self.assertIn("QTimer _registerCodeCooldownTimer;", coordinators)
        self.assertIn("_registerCodeCooldownTimer.start(1000);", session_auth)
        self.assertIn("_registerCodeCooldownTimer.stop();", session_auth)
        self.assertIn("void onRegisterCodeCooldownTimeout();", coordinators)

    def test_register_code_request_is_guarded_before_http_send(self):
        source = read(SESSION_COMMANDS)
        request_body = source.split("void SessionAuthCoordinator::requestRegisterCode(const QString& email)", 1)[
            1
        ].split("void SessionAuthCoordinator::registerUser", 1)[0]

        guard_pos = request_body.find("_port.registerCodeRequestPending()")
        cooldown_guard_pos = request_body.find("_port.registerCodeCooldownSeconds() > 0")
        pending_set_pos = request_body.find("setRegisterCodeRequestPending(true)")
        cooldown_set_pos = request_body.find("setRegisterCodeCooldownSeconds(kRegisterCodeCooldownSeconds)")
        timer_start_pos = request_body.find("startRegisterCodeCooldownTimer()")
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
        self.assertIn("stopRegisterCodeCooldownTimer()", response_source)
        success_get_code_block = response_source.split('_port.setTip("验证码已发送到邮箱，注意查收", false);', 1)[
            0
        ].rsplit("if (id == ReqId::ID_GET_VARIFY_CODE)", 1)[1]
        self.assertIn("setRegisterCodeRequestPending(false)", success_get_code_block)

    def test_register_success_countdown_timeout_uses_port_callbacks(self):
        countdown = read(REGISTER_COUNTDOWN_CONTROLLER)

        decrement_pos = countdown.find("_port.setCountdownSeconds(_port.countdownSeconds() - 1)")
        stop_pos = countdown.find("stopTimer();")
        clear_pos = countdown.find("_port.setRegisterSuccessPage(false);")
        login_pos = countdown.find("_port.switchToLogin();")

        for label, pos in (
            ("decrement countdown", decrement_pos),
            ("stop timer", stop_pos),
            ("clear success page", clear_pos),
            ("switch to login", login_pos),
        ):
            with self.subTest(label=label):
                self.assertNotEqual(-1, pos)

        self.assertLess(decrement_pos, stop_pos)
        self.assertLess(stop_pos, clear_pos)
        self.assertLess(clear_pos, login_pos)
        self.assertNotIn("_register_countdown_timer", countdown)
        self.assertIn("_timer.stop();", countdown)
        self.assertNotIn(
            "setRegisterSuccessPage(false);", countdown.replace("_port.setRegisterSuccessPage(false);", "")
        )
        self.assertNotIn("switchToLogin();", countdown.replace("_port.switchToLogin();", ""))

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
