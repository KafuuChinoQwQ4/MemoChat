import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
APP_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app"
APP_CONTROLLER_HEADER = APP_DIR / "AppController.h"
APP_CONTROLLER_CPP = APP_DIR / "AppController.cpp"
APP_CONTROLLER_NAVIGATION = APP_DIR / "AppControllerNavigation.cpp"
APP_SOURCES_CMAKE = APP_DIR.parent / "cmake/AppSources.cmake"
SESSION_AUTH_LOGIN_RESPONSE = APP_DIR / "SessionAuthCoordinatorLoginResponse.cpp"
SESSION_CHAT_ENTRY = APP_DIR / "SessionChatEntryCoordinator.cpp"
CONNECTION_COORDINATOR = APP_DIR / "AppChatConnectionCoordinator.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


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
    raise AssertionError(f"Function body not found for {signature}")


class AppControllerConnectionBoundaryTests(unittest.TestCase):
    def test_app_controller_header_no_longer_declares_recovery_helpers(self):
        header = read(APP_CONTROLLER_HEADER)
        private_start = header.index("private:")
        private_header = header[private_start:]

        forbidden_declarations = (
            "bool tryReconnectChat();",
            "void handleChatConnectFailureDuringRecovery();",
            "bool tryLoginFallbackToTcp(const QString& reason);",
            "void resetReconnectState();",
            "void resetHeartbeatTracking();",
            "bool isHeartbeatLikelyTimeout() const;",
        )
        for declaration in forbidden_declarations:
            with self.subTest(declaration=declaration):
                self.assertNotIn(declaration, private_header)

        forbidden_slots = (
            "void onTcpConnectFinished(bool success);",
            "void onChatLoginFailed(int err);",
            "void onConnectionClosed();",
            "void onHeartbeatTimeout();",
            "void onHeartbeatAck(qint64 ackAtMs);",
            "void onNotifyOffline();",
        )
        for slot in forbidden_slots:
            with self.subTest(slot=slot):
                self.assertNotIn(slot, header)

    def test_connection_slot_forwarder_files_are_removed_from_sources(self):
        cmake_text = read(APP_SOURCES_CMAKE)
        removed_files = (
            "app/AppControllerConnectionLogin.cpp",
            "app/AppControllerHeartbeat.cpp",
            "app/AppControllerReconnect.cpp",
        )
        for file_name in removed_files:
            with self.subTest(file=file_name):
                self.assertNotIn(file_name, cmake_text)
                self.assertFalse((APP_DIR.parent / file_name).exists())

    def test_internal_callers_use_connection_coordinator_directly(self):
        app_controller = read(APP_CONTROLLER_CPP)
        login_timeout_block = app_controller[
            app_controller.index("connect(&_chat_login_timeout_timer") : app_controller.index(
                "AppController::Page AppController::page"
            )
        ]
        self.assertIn(
            '_chat_connection_coordinator->tryLoginFallbackToTcp(QStringLiteral("login_timeout"))', login_timeout_block
        )
        self.assertNotIn('if (tryLoginFallbackToTcp(QStringLiteral("login_timeout")))', login_timeout_block)

        navigation = read(APP_CONTROLLER_NAVIGATION)
        self.assertIn("_chat_connection_coordinator->resetReconnectState();", navigation)
        self.assertIn("_chat_connection_coordinator->resetHeartbeatTracking();", navigation)
        self.assertNotRegex(navigation, r"(?<!->)\bresetReconnectState\(\);")
        self.assertNotRegex(navigation, r"(?<!->)\bresetHeartbeatTracking\(\);")

        auth_response = read(SESSION_AUTH_LOGIN_RESPONSE)
        self.assertIn("_app._chat_connection_coordinator->resetReconnectState();", auth_response)
        self.assertNotIn("_app.resetReconnectState();", auth_response)

        chat_entry = read(SESSION_CHAT_ENTRY)
        self.assertIn("_app._chat_connection_coordinator->resetReconnectState();", chat_entry)
        self.assertIn("_app._chat_connection_coordinator->resetHeartbeatTracking();", chat_entry)
        self.assertIn("_app._chat_connection_coordinator->onHeartbeatTimeout();", chat_entry)
        self.assertNotIn("_app.resetReconnectState();", chat_entry)
        self.assertNotIn("_app.resetHeartbeatTracking();", chat_entry)
        self.assertNotIn("_app.onHeartbeatTimeout();", chat_entry)

    def test_app_controller_connects_connection_events_to_coordinator_lambdas(self):
        source = read(APP_CONTROLLER_CPP)

        expected_connections = (
            (
                "&IChatTransport::sig_con_success",
                "_chat_connection_coordinator->onTcpConnectFinished(success);",
            ),
            (
                "&ChatMessageDispatcher::sig_login_failed",
                "_chat_connection_coordinator->onChatLoginFailed(err);",
            ),
            (
                "&ChatMessageDispatcher::sig_notify_offline",
                "_chat_connection_coordinator->onNotifyOffline();",
            ),
            (
                "&IChatTransport::sig_connection_closed",
                "_chat_connection_coordinator->onConnectionClosed();",
            ),
            (
                "&ChatMessageDispatcher::sig_heartbeat_ack",
                "_chat_connection_coordinator->onHeartbeatAck(ackAtMs);",
            ),
            (
                "connect(&_heartbeat_timer",
                "_chat_connection_coordinator->onHeartbeatTimeout();",
            ),
        )
        for signal_token, coordinator_call in expected_connections:
            with self.subTest(signal=signal_token):
                self.assertIn(signal_token, source)
                self.assertIn(coordinator_call, source)

        self.assertIn("&QTimer::timeout", source)

        forbidden_slot_targets = (
            "&AppController::onTcpConnectFinished",
            "&AppController::onChatLoginFailed",
            "&AppController::onNotifyOffline",
            "&AppController::onConnectionClosed",
            "&AppController::onHeartbeatAck",
            "&AppController::onHeartbeatTimeout",
        )
        for slot_target in forbidden_slot_targets:
            with self.subTest(slot_target=slot_target):
                self.assertNotIn(slot_target, source)

    def test_connection_coordinator_owns_stale_disconnect_recovery_order(self):
        source = read(CONNECTION_COORDINATOR)
        body = extract_cpp_function(source, "void AppChatConnectionCoordinator::onConnectionClosed")

        ignore_index = body.index("if (_app._chat_recovery_state.ignoreNextLoginDisconnect)")
        page_branch_index = body.index("if (_app._page != AppController::ChatPage)")
        chat_reconnect_index = body.index("if (_app._call_session_model.visible())")

        self.assertLess(ignore_index, page_branch_index)
        self.assertLess(ignore_index, chat_reconnect_index)
        ignored_block = body[ignore_index:page_branch_index]
        self.assertIn("_app._chat_recovery_state.ignoreNextLoginDisconnect = false;", ignored_block)
        self.assertIn("resetReconnectState();", ignored_block)
        self.assertIn("resetHeartbeatTracking();", ignored_block)
        self.assertIn("return;", ignored_block)

    def test_connection_coordinator_delegates_visible_call_cleanup_to_call_coordinator(self):
        source = read(CONNECTION_COORDINATOR)
        body = extract_cpp_function(source, "void AppChatConnectionCoordinator::onConnectionClosed")

        self.assertIn("_app._call_session_model.visible()", body)
        self.assertIn(
            '_app._call_coordinator->finalizeEndedCall(QStringLiteral("通话链路已断开"));',
            body,
        )
        self.assertNotIn("_app.finalizeEndedCall", body)


if __name__ == "__main__":
    unittest.main()
