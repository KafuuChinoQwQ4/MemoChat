import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
APP_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app"
APP_CONTROLLER_HEADER = APP_DIR / "controller/AppController.h"
APP_CONTROLLER_CPP = APP_DIR / "controller/AppController.cpp"
APP_CONNECTION_PORT_BINDER = APP_DIR / "composition/AppConnectionPortBinder.cpp"
APP_POST_LOGIN_PORT_BINDER = APP_DIR / "composition/AppPostLoginBootstrapPortBinder.cpp"
APP_SESSION_LOGOUT_PORT_BINDER = APP_DIR / "composition/AppSessionLogoutPortBinder.cpp"
APP_SIGNAL_BINDER = APP_DIR / "composition/AppSignalBinder.cpp"
APP_CHAT_TRANSPORT_SIGNAL_BINDER = APP_DIR / "composition/AppChatTransportSignalBinder.cpp"
APP_CHAT_DISPATCHER_SIGNAL_BINDER = APP_DIR / "composition/AppChatDispatcherSignalBinder.cpp"
APP_TIMER_SIGNAL_BINDER = APP_DIR / "composition/AppTimerSignalBinder.cpp"
APP_CHAT_DISPATCHER_EVENT_ROUTER = APP_DIR / "events/AppChatDispatcherEventRouter.cpp"
APP_CONTROLLER_NAVIGATION = APP_DIR / "controller/AppControllerNavigation.cpp"
SESSION_LOGOUT = APP_DIR / "session/AppSessionCoordinatorLogout.cpp"
APP_SOURCES_CMAKE = APP_DIR / "sources.cmake"
SESSION_AUTH_LOGIN_RESPONSE = APP_DIR / "session/SessionAuthCoordinatorLoginResponse.cpp"
SESSION_CHAT_ENTRY = APP_DIR / "session/SessionChatEntryCoordinator.cpp"
CONNECTION_COORDINATOR = APP_DIR / "connection/AppChatConnectionCoordinator.cpp"
CONNECTION_COORDINATOR_HEADER = APP_DIR / "connection/AppChatConnectionCoordinator.h"
CONNECTION_POLICY = APP_DIR / "connection/AppChatConnectionPolicy.cpp"
CONNECTION_POLICY_HEADER = APP_DIR / "connection/AppChatConnectionPolicy.h"
CONNECTION_STATE_HEADER = APP_DIR / "connection/AppControllerConnectionState.h"
APP_COORDINATORS_HEADER = APP_DIR / "coordinators/AppCoordinators.h"
GLOBAL_HEADER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core/common/global.h"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def signal_binding_text() -> str:
    return "\n".join(
        read(path)
        for path in (
            APP_SIGNAL_BINDER,
            APP_CHAT_TRANSPORT_SIGNAL_BINDER,
            APP_CHAT_DISPATCHER_SIGNAL_BINDER,
            APP_CHAT_DISPATCHER_EVENT_ROUTER,
            APP_TIMER_SIGNAL_BINDER,
        )
    )


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

        self.assertNotIn("friend class AppChatConnectionCoordinator;", header)

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

    def test_connection_coordinator_uses_chat_connection_port_not_app_controller(self):
        header = read(CONNECTION_COORDINATOR_HEADER)
        source = read(CONNECTION_COORDINATOR)
        combined = header + "\n" + source

        self.assertIn("ChatConnectionPort", header)
        self.assertNotIn('#include "AppController.h"', combined)
        self.assertNotIn("#include <AppController.h>", combined)
        self.assertNotIn("class AppController;", header)

        forbidden_controller_links = (
            "AppController&",
            "AppController *",
            "AppController*",
            "AppController::",
            "_app.",
        )
        for token in forbidden_controller_links:
            with self.subTest(token=token):
                self.assertNotIn(token, combined)

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
        connection_binder = read(APP_CONNECTION_PORT_BINDER)
        session_connection_ports = read(APP_POST_LOGIN_PORT_BINDER) + "\n" + read(APP_SESSION_LOGOUT_PORT_BINDER)
        signal_binder = read(APP_TIMER_SIGNAL_BINDER)
        login_timeout_block = signal_binder[signal_binder.index("connect(&_chat_login_timeout_timer") :]
        self.assertIn("_chat_connection_coordinator->onLoginTimeout();", login_timeout_block)
        self.assertNotIn(
            '_chat_connection_coordinator->tryLoginFallbackToTcp(QStringLiteral("login_timeout"))', login_timeout_block
        )
        self.assertNotIn("loginTcpFallbackAttempted", login_timeout_block)
        self.assertNotIn('if (tryLoginFallbackToTcp(QStringLiteral("login_timeout")))', login_timeout_block)

        navigation = read(APP_CONTROLLER_NAVIGATION)
        logout = read(SESSION_LOGOUT)
        self.assertIn("_session_coordinator->resetForLogout();", navigation)
        self.assertIn("invokeIfSet(_logout_port.resetConnectionRuntime);", logout)
        self.assertNotIn("invokeIfSet(_logout_port.resetReconnectState);", logout)
        self.assertNotIn("invokeIfSet(_logout_port.resetHeartbeatTracking);", logout)
        self.assertIn("_chat_connection_coordinator->resetReconnectState();", session_connection_ports)
        self.assertIn("_chat_connection_coordinator->resetHeartbeatTracking();", session_connection_ports)
        self.assertNotIn("_chat_connection_coordinator->resetReconnectState();", app_controller)
        self.assertNotIn("_chat_connection_coordinator->resetHeartbeatTracking();", app_controller)
        self.assertNotRegex(navigation, r"(?<!->)\bresetReconnectState\(\);")
        self.assertNotRegex(navigation, r"(?<!->)\bresetHeartbeatTracking\(\);")

        auth_response = read(SESSION_AUTH_LOGIN_RESPONSE)
        self.assertNotIn("_app._chat_connection_coordinator->resetReconnectState();", auth_response)
        self.assertNotIn("_app.resetReconnectState();", auth_response)
        self.assertIn("_chat_connection_coordinator->resetReconnectState();", session_connection_ports)
        self.assertIn("std::make_unique<AppChatConnectionCoordinator>", connection_binder)

        chat_entry = read(SESSION_CHAT_ENTRY)
        self.assertIn("_port.resetReconnectState();", chat_entry)
        self.assertIn("_port.resetHeartbeatTracking();", chat_entry)
        self.assertIn("_port.startHeartbeatTimer(kHeartbeatIntervalMs);", chat_entry)
        self.assertNotIn("_app.resetReconnectState();", chat_entry)
        self.assertNotIn("_app.resetHeartbeatTracking();", chat_entry)
        self.assertNotIn("_app.onHeartbeatTimeout();", chat_entry)
        self.assertNotIn("_app._chat_connection_coordinator", chat_entry)

    def test_app_controller_connects_connection_events_to_coordinator_lambdas(self):
        source = signal_binding_text()
        app_controller = read(APP_CONTROLLER_CPP)

        expected_connections = (
            (
                "&IChatTransport::sig_con_success",
                "_chat_connection_coordinator->onTcpConnectFinished(success);",
            ),
            (
                "&ChatMessageDispatcher::sig_login_failed",
                "_connection_coordinator.onChatLoginFailed(err);",
            ),
            (
                "&ChatMessageDispatcher::sig_notify_offline",
                "_connection_coordinator.onNotifyOffline();",
            ),
            (
                "&IChatTransport::sig_connection_closed",
                "_chat_connection_coordinator->onConnectionClosed();",
            ),
            (
                "&ChatMessageDispatcher::sig_heartbeat_ack",
                "_connection_coordinator.onHeartbeatAck(ackAtMs);",
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
                self.assertNotIn(slot_target, app_controller)

        self.assertIn("bindAppControllerSignals();", app_controller)
        self.assertNotIn("&IChatTransport::sig_con_success", app_controller)
        self.assertNotIn("connect(&_heartbeat_timer", app_controller)

    def test_connection_coordinator_owns_stale_disconnect_recovery_order(self):
        source = read(CONNECTION_COORDINATOR)
        body = extract_cpp_function(source, "void AppChatConnectionCoordinator::onConnectionClosed")

        ignore_index = body.index("ignoreNextLoginDisconnect")
        page_branch_index = body.index("isChatPage")
        chat_reconnect_index = body.index("callVisible")

        self.assertLess(ignore_index, page_branch_index)
        self.assertLess(ignore_index, chat_reconnect_index)
        ignored_block = body[ignore_index:page_branch_index]
        self.assertIn("setIgnoreNextLoginDisconnect(false)", ignored_block)
        self.assertIn("resetReconnectState();", ignored_block)
        self.assertIn("resetHeartbeatTracking();", ignored_block)
        self.assertIn("return;", ignored_block)

    def test_connection_coordinator_delegates_visible_call_cleanup_to_call_coordinator(self):
        source = read(CONNECTION_COORDINATOR)
        body = extract_cpp_function(source, "void AppChatConnectionCoordinator::onConnectionClosed")

        self.assertIn("callVisible", body)
        self.assertIn(
            'finalizeEndedCall(QStringLiteral("通话链路已断开"));',
            body,
        )
        self.assertNotIn("_app.", body)

    def test_chat_login_contract_uses_v3_login_ticket_only(self):
        coordinator_header = read(CONNECTION_COORDINATOR_HEADER)
        coordinator_source = read(CONNECTION_COORDINATOR)
        policy_header = read(CONNECTION_POLICY_HEADER)
        policy_source = read(CONNECTION_POLICY)
        login_response = read(SESSION_AUTH_LOGIN_RESPONSE)
        connection_state = read(CONNECTION_STATE_HEADER)
        coordinators = read(APP_COORDINATORS_HEADER)
        global_header = read(GLOBAL_HEADER)
        on_connect = extract_cpp_function(
            coordinator_source,
            "void AppChatConnectionCoordinator::onTcpConnectFinished",
        )

        for text, label in (
            (coordinator_header, "coordinator snapshot"),
            (policy_header, "policy snapshot"),
            (connection_state, "controller connection state"),
            (coordinators, "coordinator state"),
            (global_header, "ServerInfo"),
        ):
            with self.subTest(default_owner=label):
                expected_default = "ProtocolVersion = 3" if label == "ServerInfo" else "protocolVersion = 3"
                self.assertIn(expected_default, text)

        self.assertIn("snapshot.protocolVersion != 3 || snapshot.loginTicket.trimmed().isEmpty()", on_connect)
        self.assertIn('obj["protocol_version"] = 3;', on_connect)
        self.assertIn('obj["login_ticket"] = snapshot.loginTicket;', on_connect)
        self.assertNotIn('obj["uid"]', on_connect)
        self.assertNotIn('obj["token"]', on_connect)

        self.assertIn("invalidChatTicket", policy_source)
        self.assertIn("snapshot.protocolVersion != 3 || snapshot.loginTicket.trimmed().isEmpty()", policy_source)
        self.assertNotIn("missingCredential", policy_source)
        self.assertIn("toInt(3)", login_response)
        self.assertNotIn("toInt(2)", login_response)


if __name__ == "__main__":
    unittest.main()
