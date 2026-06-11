import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CALL = CLIENT / "features/call"
APP = CLIENT / "app"
APP_FEATURE_REGISTRY_H = APP / "composition/AppFeatureRegistry.h"
APP_FEATURE_REGISTRY_CPP = APP / "composition/AppFeatureRegistry.cpp"
APP_CALL_PORT_BINDER = APP / "composition/AppCallPortBinder.cpp"
CHAT_VIEW = CLIENT / "features/chat/view"
SHELL_PAGE = CLIENT / "qml/app/ChatShellPage.qml"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


class CallFacadeContractTests(unittest.TestCase):
    def test_call_controller_exposes_qml_surface_and_preserves_transport_helpers(self):
        header = read(CALL / "controller/CallController.h")
        compact_header = normalized(header)

        expected_tokens = (
            "class CallController : public QObject",
            "Q_OBJECT",
            "Q_PROPERTY(CallSessionModel* callSession READ callSession NOTIFY callSurfaceChanged)",
            "Q_PROPERTY(LivekitBridge* livekitBridge READ livekitBridge NOTIFY callSurfaceChanged)",
            "explicit CallController(ClientGateway* gateway, QObject* parent = nullptr);",
            "CallSessionModel* callSession() const;",
            "LivekitBridge* livekitBridge() const;",
            "Q_INVOKABLE void startVoiceChat();",
            "Q_INVOKABLE void startVideoChat();",
            "Q_INVOKABLE void acceptIncomingCall();",
            "Q_INVOKABLE void rejectIncomingCall();",
            "Q_INVOKABLE void endCurrentCall();",
            "Q_INVOKABLE void toggleCallMuted();",
            "Q_INVOKABLE void toggleCallCamera();",
            "void syncSurface(CallSessionModel* callSession, LivekitBridge* livekitBridge);",
            "void setCommandPort(CallCommandPort port);",
            "struct CallCommandPort",
            "std::function<void()> startVoiceChat;",
            "std::function<void()> toggleCallCamera;",
            "void callSurfaceChanged();",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, compact_header if "\n" not in token else header)

        transport_helpers = (
            "void startCall(int uid, const QString& token, int peerUid, const QString& callType) const;",
            "void acceptCall(int uid, const QString& token, const QString& callId) const;",
            "void rejectCall(int uid, const QString& token, const QString& callId) const;",
            "void cancelCall(int uid, const QString& token, const QString& callId) const;",
            "void hangupCall(int uid, const QString& token, const QString& callId) const;",
            "void fetchToken(int uid, const QString& token, const QString& callId, const QString& role) const;",
        )
        for token in transport_helpers:
            with self.subTest(token=token):
                self.assertIn(token, header)

    def test_call_controller_invokables_use_command_port(self):
        controller = read(CALL / "controller/CallController.cpp")
        compact = normalized(controller)

        expected_pairs = (
            ("CallController::startVoiceChat()", "_command_port.startVoiceChat();"),
            ("CallController::startVideoChat()", "_command_port.startVideoChat();"),
            ("CallController::acceptIncomingCall()", "_command_port.acceptIncomingCall();"),
            ("CallController::rejectIncomingCall()", "_command_port.rejectIncomingCall();"),
            ("CallController::endCurrentCall()", "_command_port.endCurrentCall();"),
            ("CallController::toggleCallMuted()", "_command_port.toggleCallMuted();"),
            ("CallController::toggleCallCamera()", "_command_port.toggleCallCamera();"),
        )
        for method, invoked in expected_pairs:
            with self.subTest(method=method):
                pattern = re.escape(method) + r".*?" + re.escape(invoked)
                self.assertRegex(compact, pattern)
        self.assertNotIn("Requested();", controller)

    def test_appcontroller_exposes_additive_call_composition_surface(self):
        header = read(APP / "controller/AppController.h")
        registry = read(APP_FEATURE_REGISTRY_H)
        registry_constructor = read(APP_FEATURE_REGISTRY_CPP)
        controller = read(APP / "controller/AppController.cpp")
        port_binder = read(APP_CALL_PORT_BINDER)
        model_props = read(APP / "controller/AppControllerModelProperties.cpp")
        composition = read(APP / "composition/AppComposition.cpp")
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")
        compact_controller = normalized(controller)
        compact_port_binder = normalized(port_binder)
        public = header[header.index("public:") : header.index("signals:")]
        private = header[header.index("private:") :]

        self.assertNotIn("Q_PROPERTY(CallController* call READ call CONSTANT)", header)
        self.assertNotIn("CallController* call();", header)
        self.assertIn("CallController* callController();", public)
        self.assertIn("AppFeatureRegistry _features", header)
        self.assertIn("CallController callController;", registry)
        self.assertIn("void syncCallControllerState();", private)

        self.assertIn(", _features(_gateway, this)", controller)
        self.assertIn(", callController(&gateway, parent)", registry_constructor)
        self.assertNotIn("CallController* AppController::call()", model_props)
        self.assertIn("CallController* AppController::callController()", model_props)
        self.assertIn("return &_features.callController;", model_props)
        self.assertNotIn("CallSessionModel* AppController::callSession()", controller)
        self.assertNotIn("LivekitBridge* AppController::livekitBridge()", controller)
        self.assertNotIn("CallSessionModel _call_session_model;", header)
        self.assertNotIn("LivekitBridge _livekit_bridge;", header)

        self.assertIn("_features.callController.setCommandPort(CallCommandPort{", compact_port_binder)
        self.assertNotIn("_features.callController.setCommandPort(CallCommandPort{", compact_controller)
        self.assertNotIn("CallController::startVoiceChatRequested", compact_controller + compact_port_binder)
        self.assertNotIn("CallController::toggleCallCameraRequested", compact_controller + compact_port_binder)

        self.assertIn("void AppController::syncCallControllerState()", controller)
        self.assertIn("syncCallControllerState();", controller)
        self.assertIn('context.setContextProperty("call", call())', composition)
        self.assertNotIn('setContextProperty("livekitBridge"', engine)

    def test_qml_uses_call_facade_for_call_actions_and_state(self):
        shell_page = read(SHELL_PAGE)
        shell_content = read(CHAT_VIEW / "ChatShellContent.qml")
        modal_layer = read(CHAT_VIEW / "ChatModalLayer.qml")
        call_overlay = read(CALL / "view/CallOverlay.qml")
        combined = "\n".join((shell_page, shell_content, modal_layer, call_overlay))

        expected_tokens = (
            "callSession: call.callSession",
            "livekitBridge: call.livekitBridge",
            "onAcceptRequested: call.acceptIncomingCall()",
            "onRejectRequested: call.rejectIncomingCall()",
            "onEndRequested: call.endCurrentCall()",
            "onMuteToggled: call.toggleCallMuted()",
            "onCameraToggled: call.toggleCallCamera()",
            "onSendVoiceCall: call.startVoiceChat()",
            "onSendVideoCall: call.startVideoChat()",
            "onVoiceChatRequested: call.startVoiceChat()",
            "onVideoChatRequested: call.startVideoChat()",
            "property var livekitBridge: null",
            "livekitBridge: root.livekitBridge",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, combined)

    def test_migrated_qml_avoids_old_global_call_surface(self):
        qml_sources = {
            "ChatShellPage.qml": read(SHELL_PAGE),
            "ChatShellContent.qml": read(CHAT_VIEW / "ChatShellContent.qml"),
            "ChatModalLayer.qml": read(CHAT_VIEW / "ChatModalLayer.qml"),
            "CallOverlay.qml": read(CALL / "view/CallOverlay.qml"),
        }
        forbidden_tokens = (
            "controller.callSession",
            "controller.livekitBridge",
            "controller.startVoiceChat(",
            "controller.startVideoChat(",
            "controller.acceptIncomingCall(",
            "controller.rejectIncomingCall(",
            "controller.endCurrentCall(",
            "controller.toggleCallMuted(",
            "controller.toggleCallCamera(",
            "chat.startVoiceChat(",
            "chat.startVideoChat(",
        )
        for file_name, source in qml_sources.items():
            compact_source = normalized(source)
            for token in forbidden_tokens:
                with self.subTest(file=file_name, token=token):
                    self.assertNotIn(token, compact_source)

        overlay = read(CALL / "view/CallOverlay.qml")
        global_bridge_refs = re.findall(r"(?<![.\w])livekitBridge(?!\s*:)", overlay)
        self.assertEqual(
            [],
            global_bridge_refs,
            "CallOverlay should use root.livekitBridge instead of implicit global livekitBridge lookups.",
        )

    def test_appcontroller_prunes_legacy_call_qml_surface_and_cpp_wrappers(self):
        header = read(APP / "controller/AppController.h")

        legacy_qml_tokens = (
            "Q_PROPERTY(CallSessionModel* callSession READ callSession CONSTANT)",
            "Q_PROPERTY(LivekitBridge* livekitBridge READ livekitBridge CONSTANT)",
            "Q_INVOKABLE void startVoiceChat();",
            "Q_INVOKABLE void startVideoChat();",
            "Q_INVOKABLE void acceptIncomingCall();",
            "Q_INVOKABLE void rejectIncomingCall();",
            "Q_INVOKABLE void endCurrentCall();",
            "Q_INVOKABLE void toggleCallMuted();",
            "Q_INVOKABLE void toggleCallCamera();",
            "CallSessionModel* callSession();",
            "LivekitBridge* livekitBridge();",
        )
        for token in legacy_qml_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        removed_cpp_targets = (
            "void startVoiceChat();",
            "void startVideoChat();",
            "void acceptIncomingCall();",
            "void rejectIncomingCall();",
            "void endCurrentCall();",
            "void toggleCallMuted();",
            "void toggleCallCamera();",
        )
        for token in removed_cpp_targets:
            with self.subTest(token=token):
                self.assertNotIn(token, header)


if __name__ == "__main__":
    unittest.main()
