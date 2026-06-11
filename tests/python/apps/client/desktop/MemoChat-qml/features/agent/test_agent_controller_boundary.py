import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
AGENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/agent"
AGENT_CONTROLLER_DIR = AGENT_DIR / "controller"
AGENT_GAME_DIR = AGENT_DIR / "game"
AGENT_MODEL_DIR = AGENT_DIR / "model"
AGENT_TRANSPORT_DIR = AGENT_DIR / "transport"
AGENT_SOURCES_CMAKE = AGENT_DIR / "sources.cmake"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def function_body(source: str, name: str) -> str:
    match = re.search(r"\b[\w:<>]+\s+AgentController::" + re.escape(name) + r"\s*\([^)]*\)\s*\{", source)
    if not match:
        return ""

    depth = 1
    index = match.end()
    while index < len(source) and depth > 0:
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        index += 1
    return source[match.end() : index - 1]


class AgentControllerBoundaryTests(unittest.TestCase):
    def test_agent_request_tracker_files_exist_and_are_registered(self):
        expected_files = (
            AGENT_MODEL_DIR / "AgentRequestTypes.h",
            AGENT_TRANSPORT_DIR / "AgentRequestTracker.h",
            AGENT_TRANSPORT_DIR / "AgentRequestTracker.cpp",
        )

        for path in expected_files:
            with self.subTest(file=path.name):
                self.assertTrue(path.is_file())

        cmake_text = AGENT_SOURCES_CMAKE.read_text(encoding="utf-8")
        self.assertIn("features/agent/transport/AgentRequestTracker.cpp", cmake_text)
        self.assertIn("features/agent/model/AgentRequestTypes.h", cmake_text)
        self.assertIn("features/agent/transport/AgentRequestTracker.h", cmake_text)

    def test_agent_controller_uses_typed_request_tracker(self):
        header_text = (AGENT_CONTROLLER_DIR / "AgentController.h").read_text(encoding="utf-8")

        self.assertIn('#include "AgentRequestTracker.h"', header_text)
        self.assertIn("AgentRequestTracker _pending_requests;", header_text)
        self.assertNotIn("QMap<ReqId, QString> _pending_requests", header_text)
        self.assertIn("int uid = 0;", (AGENT_MODEL_DIR / "AgentRequestTypes.h").read_text(encoding="utf-8"))
        self.assertIn(
            "void track(ReqId id, AgentRequestKind kind, const QString& messageId = QString(), int uid = 0);",
            (AGENT_TRANSPORT_DIR / "AgentRequestTracker.h").read_text(encoding="utf-8"),
        )

    def test_agent_request_files_do_not_assign_string_request_types(self):
        request_files = (
            AGENT_CONTROLLER_DIR / "AgentControllerChat.cpp",
            AGENT_CONTROLLER_DIR / "AgentControllerModels.cpp",
            AGENT_CONTROLLER_DIR / "AgentControllerSessions.cpp",
            AGENT_CONTROLLER_DIR / "AgentControllerKnowledge.cpp",
            AGENT_CONTROLLER_DIR / "AgentControllerMemory.cpp",
            AGENT_CONTROLLER_DIR / "AgentControllerAgentTasks.cpp",
        )

        string_assignment = re.compile(r"_pending_requests\s*\[\s*reqId\s*\]\s*=\s*\"[^\"]+\"")

        for path in request_files:
            with self.subTest(file=path.name):
                text = path.read_text(encoding="utf-8")
                self.assertIsNone(string_assignment.search(text))
                self.assertIn("_pending_requests.track(reqId, AgentRequestKind::", text)

    def test_http_finish_routes_through_typed_request_kind(self):
        text = (AGENT_CONTROLLER_DIR / "AgentController.cpp").read_text(encoding="utf-8")

        self.assertIn("std::optional<AgentRequestRecord>", text)
        self.assertIn("AgentRequestKind::", text)
        self.assertIn("switch (record.kind)", text)
        self.assertIn("if (record.uid != 0 && record.uid != currentUid())", text)
        self.assertNotIn("QString reqType", text)
        self.assertNotIn('reqType == "model_list"', text)
        self.assertNotIn('reqType == "kb_upload"', text)

    def test_agent_direct_network_client_files_exist_and_are_registered(self):
        expected_files = (
            AGENT_TRANSPORT_DIR / "AgentNetworkRequestUtils.h",
            AGENT_TRANSPORT_DIR / "AgentStreamClient.h",
            AGENT_TRANSPORT_DIR / "AgentStreamClient.cpp",
            AGENT_GAME_DIR / "AgentGameClient.h",
            AGENT_GAME_DIR / "AgentGameClient.cpp",
        )

        for path in expected_files:
            with self.subTest(file=path.name):
                self.assertTrue(path.is_file())

        cmake_text = AGENT_SOURCES_CMAKE.read_text(encoding="utf-8")
        for path in expected_files:
            rel_path = path.relative_to(REPO_ROOT / "apps/client/desktop/MemoChat-qml").as_posix()
            with self.subTest(cmake_file=path.name):
                self.assertIn(rel_path, cmake_text)

    def test_agent_tls_request_helper_is_not_duplicated(self):
        helper_paths = (
            AGENT_TRANSPORT_DIR / "AgentNetworkRequestUtils.h",
            AGENT_TRANSPORT_DIR / "AgentNetworkRequestUtils.cpp",
        )
        helper_text = "\n".join(path.read_text(encoding="utf-8") for path in helper_paths if path.exists())
        stream_text = (AGENT_CONTROLLER_DIR / "AgentControllerStream.cpp").read_text(encoding="utf-8")
        game_text = (AGENT_GAME_DIR / "AgentControllerGameNetwork.cpp").read_text(encoding="utf-8")

        self.assertEqual(1, helper_text.count("setPeerVerifyMode(QSslSocket::VerifyNone)"))
        self.assertNotIn("setPeerVerifyMode(QSslSocket::VerifyNone)", stream_text)
        self.assertNotIn("setPeerVerifyMode(QSslSocket::VerifyNone)", game_text)
        self.assertNotIn("QSslConfiguration", stream_text)
        self.assertNotIn("QSslConfiguration", game_text)

    def test_stream_controller_no_longer_owns_raw_network_request(self):
        text = (AGENT_CONTROLLER_DIR / "AgentControllerStream.cpp").read_text(encoding="utf-8")

        self.assertNotIn("#include <QNetworkRequest>", text)
        self.assertNotIn("QNetworkRequest request", text)
        self.assertNotIn("_streamManager->post", text)
        self.assertIn("AgentStreamClient", (AGENT_CONTROLLER_DIR / "AgentController.h").read_text(encoding="utf-8"))

    def test_game_network_controller_uses_game_client_for_reply_parsing(self):
        text = (AGENT_GAME_DIR / "AgentControllerGameNetwork.cpp").read_text(encoding="utf-8")
        header = (AGENT_GAME_DIR / "AgentGameClient.h").read_text(encoding="utf-8")
        client = (AGENT_GAME_DIR / "AgentGameClient.cpp").read_text(encoding="utf-8")
        responses = (AGENT_GAME_DIR / "AgentControllerGameResponses.cpp").read_text(encoding="utf-8")

        self.assertNotIn("#include <QNetworkReply>", text)
        self.assertNotIn("QNetworkReply* reply", text)
        self.assertNotIn("QJsonDocument::fromJson(body)", text)
        self.assertNotIn("_gameNetwork->get", text)
        self.assertNotIn("_gameNetwork->post", text)
        self.assertNotIn("_gameNetwork->deleteResource", text)
        self.assertIn("AgentGameClient", (AGENT_CONTROLLER_DIR / "AgentController.h").read_text(encoding="utf-8"))
        self.assertIn("void responseReady(const QString& op, const QJsonObject& root, int uid);", header)
        self.assertIn("emit responseReady(op, doc.object(), uid);", client)
        self.assertIn("if (uid != 0 && uid != currentUid())", text)
        self.assertIn("if (uid != 0 && uid != currentUid())", responses)

    def test_agent_controller_resets_user_scoped_runtime_when_uid_changes(self):
        header = read(AGENT_CONTROLLER_DIR / "AgentController.h")
        sessions = read(AGENT_CONTROLLER_DIR / "AgentControllerSessions.cpp")
        reset_body = function_body(sessions, "resetUserScopedRuntime")
        ensure_body = function_body(sessions, "ensureUserScope")

        self.assertIn("int _scoped_uid = 0;", header)
        self.assertIn("Q_INVOKABLE void resetForLogout();", header)
        self.assertIn("void ensureUserScope();", header)
        self.assertIn("void resetUserScopedRuntime();", header)
        self.assertIn("const int uid = currentUid();", ensure_body)
        self.assertIn("if (_scoped_uid == uid)", ensure_body)
        self.assertIn("resetUserScopedRuntime();", ensure_body)
        self.assertIn("_scoped_uid = uid;", ensure_body)

        for token in (
            "_current_session_id.clear();",
            "_sessions.clear();",
            "_model->clear();",
            "_pending_requests.clear();",
            "_current_game_room_id.clear();",
            "_game_state.clear();",
            "_knowledge_bases.clear();",
            "_memories.clear();",
            "_agent_tasks.clear();",
            "_pendingDeleteSessionId.clear();",
            "_pendingDeleteGameRoomId.clear();",
            "_available_models.clear();",
            "_model_refresh_busy = false;",
            "_api_provider_busy = false;",
            "_api_provider_status.clear();",
            "emit sessionsChanged();",
            "emit modelsChanged();",
            "emit modelStateChanged();",
            "emit gameStateChanged();",
        ):
            self.assertIn(token, reset_body)

    def test_logout_flow_resets_agent_runtime_before_account_switch_reuse(self):
        header = read(AGENT_CONTROLLER_DIR / "AgentController.h")
        sessions = read(AGENT_CONTROLLER_DIR / "AgentControllerSessions.cpp")
        logout_binder = read(
            REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/composition/AppSessionLogoutPortBinder.cpp"
        )
        reset_logout_body = function_body(sessions, "resetForLogout")

        self.assertIn("Q_INVOKABLE void resetForLogout();", header)
        self.assertIn("resetUserScopedRuntime();", reset_logout_body)
        self.assertIn("_scoped_uid = currentUid();", reset_logout_body)
        self.assertIn("_features.agentController.resetForLogout();", logout_binder)

    def test_user_scoped_agent_entrypoints_guard_before_reusing_session_state(self):
        source_by_file = {
            "AgentControllerSessions.cpp": read(AGENT_CONTROLLER_DIR / "AgentControllerSessions.cpp"),
            "AgentControllerChat.cpp": read(AGENT_CONTROLLER_DIR / "AgentControllerChat.cpp"),
            "AgentControllerStream.cpp": read(AGENT_CONTROLLER_DIR / "AgentControllerStream.cpp"),
            "AgentControllerKnowledge.cpp": read(AGENT_CONTROLLER_DIR / "AgentControllerKnowledge.cpp"),
            "AgentControllerMemory.cpp": read(AGENT_CONTROLLER_DIR / "AgentControllerMemory.cpp"),
            "AgentControllerAgentTasks.cpp": read(AGENT_CONTROLLER_DIR / "AgentControllerAgentTasks.cpp"),
            "AgentControllerModels.cpp": read(AGENT_CONTROLLER_DIR / "AgentControllerModels.cpp"),
            "AgentControllerGameRooms.cpp": read(AGENT_GAME_DIR / "AgentControllerGameRooms.cpp"),
            "AgentControllerGameTemplates.cpp": read(AGENT_GAME_DIR / "AgentControllerGameTemplates.cpp"),
        }

        guarded_functions = {
            "AgentControllerSessions.cpp": (
                "loadSessions",
                "createSession",
                "switchSession",
                "deleteSession",
                "loadHistory",
            ),
            "AgentControllerChat.cpp": (
                "sendMessage",
                "summarizeChat",
                "suggestReply",
                "translateMessageWithSource",
            ),
            "AgentControllerStream.cpp": ("sendStreamMessage",),
            "AgentControllerKnowledge.cpp": (
                "uploadDocument",
                "searchKnowledgeBase",
                "listKnowledgeBases",
                "deleteKnowledgeBase",
            ),
            "AgentControllerMemory.cpp": (
                "listMemories",
                "createMemory",
                "deleteMemory",
            ),
            "AgentControllerAgentTasks.cpp": (
                "listAgentTasks",
                "createAgentTask",
                "cancelAgentTask",
                "resumeAgentTask",
            ),
            "AgentControllerModels.cpp": (
                "refreshModelList",
                "registerApiProvider",
                "deleteApiProvider",
            ),
            "AgentControllerGameRooms.cpp": (
                "loadGameRoom",
                "deleteGameRoom",
                "createGameRoom",
                "createGameRoomFromTemplate",
                "startGameRoom",
                "restartGameRoom",
                "tickGameRoom",
                "autoTickGameRoom",
                "submitGameAction",
                "updateGameParticipant",
            ),
            "AgentControllerGameTemplates.cpp": (
                "listGameRulesets",
                "loadGameRolePresets",
                "listGameRooms",
                "listGameTemplates",
                "listGameTemplatePresets",
                "saveGameTemplate",
                "deleteGameTemplate",
                "cloneGameTemplatePreset",
                "exportGameTemplate",
                "importGameTemplate",
            ),
        }

        for file_name, functions in guarded_functions.items():
            source = source_by_file[file_name]
            for name in functions:
                with self.subTest(file=file_name, function=name):
                    body = function_body(source, name)
                    self.assertTrue(body, f"{name} should be defined in {file_name}")
                    self.assertIn("ensureUserScope();", body)

        tracked_sources = "\n".join(source_by_file.values())
        self.assertNotIn("_pending_requests.track(reqId, AgentRequestKind::ChatMessage, msgId);", tracked_sources)
        for line in tracked_sources.splitlines():
            if "_pending_requests.track(" not in line:
                continue
            with self.subTest(track_call=line.strip()):
                self.assertRegex(line, r",\s*(?:uid|scopedUid\(\))\s*\);")
        self.assertIn("_currentStreamUid = uid;", read(AGENT_CONTROLLER_DIR / "AgentControllerStream.cpp"))
        self.assertIn(
            "if (_currentStreamUid != 0 && _currentStreamUid != currentUid())",
            read(AGENT_CONTROLLER_DIR / "AgentControllerStream.cpp"),
        )


if __name__ == "__main__":
    unittest.main()
