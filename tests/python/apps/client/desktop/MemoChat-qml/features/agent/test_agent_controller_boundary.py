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


def free_function_body(source: str, name: str) -> str:
    match = re.search(r"\b[\w:<>]+\s+" + re.escape(name) + r"\s*\([^)]*\)\s*\{", source)
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


def qml_handler_body(source: str, handler_name: str) -> str:
    marker = handler_name + ":"
    start = source.find(marker)
    if start == -1:
        return ""
    open_brace = source.find("{", start)
    if open_brace == -1:
        return ""

    depth = 1
    index = open_brace + 1
    while index < len(source) and depth > 0:
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        index += 1
    return source[open_brace + 1 : index - 1]


def qml_property_names(header_text: str) -> set[str]:
    return set(re.findall(r"Q_PROPERTY\([^)\n]+\s+(\w+)\s+READ\s", header_text))


def qml_invokable_names(header_text: str) -> set[str]:
    public_surface = header_text.split("signals:", 1)[0]
    return set(re.findall(r"Q_INVOKABLE\s+[\w:<>*&]+\s+(\w+)\s*\(", public_surface))


class AgentControllerBoundaryTests(unittest.TestCase):
    def test_agent_controller_qml_surface_is_explicitly_capped(self):
        header = read(AGENT_CONTROLLER_DIR / "AgentController.h")

        expected_properties = {
            "sessions",
            "model",
            "currentSessionId",
            "currentModel",
            "availableModels",
            "modelRefreshBusy",
            "apiProviderBusy",
            "apiProviderStatus",
            "thinkingEnabled",
            "currentModelSupportsThinking",
            "knowledgeBases",
            "knowledgeSearchResult",
            "knowledgeBusy",
            "knowledgeStatusText",
            "knowledgeError",
            "memories",
            "memoryBusy",
            "memoryStatusText",
            "memoryError",
            "agentTasks",
            "agentTaskBusy",
            "agentTaskStatusText",
            "agentTaskError",
            "currentTraceId",
            "currentSkill",
            "currentFeedbackSummary",
            "traceEvents",
            "traceObservations",
            "agentSkillMode",
            "agentSkillDisplay",
            "gameRooms",
            "gameTemplates",
            "gameTemplatePresets",
            "gameState",
            "currentGameRoomId",
            "gameRulesets",
            "gameRolePresets",
            "gameBusy",
            "gameStatusText",
            "gameError",
            "loading",
            "error",
            "streaming",
            "currentGeneratingMsgId",
        }
        expected_invokables = {
            "sendMessage",
            "sendStreamMessage",
            "resetForLogout",
            "createSession",
            "switchSession",
            "deleteSession",
            "renameSession",
            "loadHistory",
            "loadSessions",
            "switchModel",
            "switchAgentSkillMode",
            "refreshModelList",
            "registerApiProvider",
            "deleteApiProvider",
            "summarizeChat",
            "suggestReply",
            "translateMessage",
            "translateMessageWithSource",
            "uploadDocument",
            "chooseAndUploadDocument",
            "searchKnowledgeBase",
            "listKnowledgeBases",
            "deleteKnowledgeBase",
            "listMemories",
            "createMemory",
            "deleteMemory",
            "listAgentTasks",
            "createAgentTask",
            "cancelAgentTask",
            "resumeAgentTask",
            "listGameRulesets",
            "loadGameRolePresets",
            "listGameRooms",
            "listGameTemplates",
            "listGameTemplatePresets",
            "loadGameRoom",
            "deleteGameRoom",
            "createGameRoom",
            "saveGameTemplate",
            "deleteGameTemplate",
            "cloneGameTemplatePreset",
            "exportGameTemplate",
            "importGameTemplate",
            "createGameRoomFromTemplate",
            "startGameRoom",
            "restartGameRoom",
            "tickGameRoom",
            "autoTickGameRoom",
            "submitGameAction",
            "updateGameParticipant",
            "cancelStream",
        }

        self.assertEqual(sorted(expected_properties), sorted(qml_property_names(header)))
        self.assertEqual(sorted(expected_invokables), sorted(qml_invokable_names(header)))

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

    def test_stream_client_uses_gate_protocol_fallback_before_reporting_network_error(self):
        header = (AGENT_TRANSPORT_DIR / "AgentStreamClient.h").read_text(encoding="utf-8")
        source = (AGENT_TRANSPORT_DIR / "AgentStreamClient.cpp").read_text(encoding="utf-8")

        self.assertIn('#include "HttpMgrRequestUtils.h"', source)
        self.assertIn("gateProtocolFallbackUrls(url)", source)
        self.assertIn("QVector<QUrl> _fallbackUrls", header)
        self.assertIn("bool _receivedAnyChunk = false", header)
        self.assertIn("void startRequest(const QUrl& url)", header)
        self.assertLess(source.index("!receivedAnyChunk"), source.index("startRequest(fallback)"))
        self.assertLess(source.index("startRequest(fallback)"), source.index("emit errorOccurred"))
        self.assertIn("updateGatePrefixesFromReplyUrl(replyUrl)", source)

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

    def test_ai_gateway_requests_use_bearer_header_and_strip_legacy_auth_fields(self):
        header = read(AGENT_CONTROLLER_DIR / "AgentController.h")
        controller = read(AGENT_CONTROLLER_DIR / "AgentController.cpp")
        request_utils = read(REPO_ROOT / "apps/client/desktop/MemoChat-qml/core/network/HttpMgrRequestUtils.cpp")
        direct_agent_request_utils = read(AGENT_TRANSPORT_DIR / "AgentNetworkRequestUtils.h")

        self.assertIn("void addAuthToPayload(QJsonObject& payload) const;", header)
        self.assertIn("void addAuthToQuery(QUrlQuery& query) const;", header)
        self.assertIn('payload.remove(QStringLiteral("uid"));', controller)
        self.assertIn('payload.remove(QStringLiteral("token"));', controller)
        self.assertIn('query.removeAllQueryItems(QStringLiteral("uid"));', controller)
        self.assertIn('query.removeAllQueryItems(QStringLiteral("token"));', controller)
        self.assertIn('request.setRawHeader(QByteArrayLiteral("Authorization")', request_utils)
        self.assertIn("applyBearerAccessTokenHeader(request);", direct_agent_request_utils)

        expected_payload_calls = {
            "AgentControllerChat.cpp": 4,
            "AgentControllerStream.cpp": 1,
            "AgentControllerSessions.cpp": 3,
            "AgentControllerKnowledge.cpp": 3,
            "AgentControllerMemory.cpp": 2,
            "AgentControllerAgentTasks.cpp": 3,
            "AgentControllerModels.cpp": 2,
        }
        for file_name, count in expected_payload_calls.items():
            with self.subTest(file=file_name, helper="payload"):
                self.assertEqual(read(AGENT_CONTROLLER_DIR / file_name).count("addAuthToPayload(payload);"), count)

        expected_query_calls = {
            "AgentControllerSessions.cpp": 2,
            "AgentControllerKnowledge.cpp": 1,
            "AgentControllerMemory.cpp": 1,
            "AgentControllerAgentTasks.cpp": 1,
            "AgentControllerModels.cpp": 1,
        }
        for file_name, count in expected_query_calls.items():
            with self.subTest(file=file_name, helper="query"):
                self.assertEqual(read(AGENT_CONTROLLER_DIR / file_name).count("addAuthToQuery(query);"), count)

        game_network = read(AGENT_GAME_DIR / "AgentControllerGameNetwork.cpp")
        self.assertEqual(game_network.count("addAuthToQuery(query);"), 2)
        self.assertEqual(game_network.count("addAuthToPayload(authedPayload);"), 1)

        bearer_only_sources = {
            **{file_name: read(AGENT_CONTROLLER_DIR / file_name) for file_name in expected_payload_calls},
            "AgentControllerGameRooms.cpp": read(AGENT_GAME_DIR / "AgentControllerGameRooms.cpp"),
            "AgentControllerGameTemplates.cpp": read(AGENT_GAME_DIR / "AgentControllerGameTemplates.cpp"),
        }
        for file_name, source in bearer_only_sources.items():
            with self.subTest(file=file_name, helper="no_http_uid_auth"):
                self.assertNotIn('query.addQueryItem("uid"', source)
                self.assertNotIn('query.addQueryItem(QStringLiteral("uid")', source)
                self.assertNotIn('payload["uid"]', source)
                self.assertNotIn('payload[QStringLiteral("uid")]', source)

    def test_ai_tab_entry_stays_lightweight_and_non_stacking(self):
        client = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
        chat_left_panel = read(client / "features/chat/view/ChatLeftPanel.qml")
        chat_normal_face = read(client / "features/chat/view/ChatNormalFace.qml")
        app_navigation = read(client / "app/controller/AppControllerNavigation.cpp")
        models = read(AGENT_CONTROLLER_DIR / "AgentControllerModels.cpp")
        request_utils = read(client / "core/network/HttpMgrRequestUtils.cpp")
        direct_agent_request_utils = read(AGENT_TRANSPORT_DIR / "AgentNetworkRequestUtils.h")

        tab_handler = qml_handler_body(chat_left_panel, "onCurrentTabChanged")
        self.assertNotIn("agentRefreshRequested()", tab_handler)

        agent_refresh_handler = qml_handler_body(chat_normal_face, "onAgentRefreshRequested")
        self.assertIn("agent.loadSessions()", agent_refresh_handler)
        self.assertIn("agent.refreshModelList()", agent_refresh_handler)
        self.assertNotIn("agent.listGameRooms()", agent_refresh_handler)
        self.assertNotIn("agent.listGameRulesets()", agent_refresh_handler)
        self.assertNotIn("agent.listGameTemplates()", agent_refresh_handler)

        switch_tab_body = free_function_body(app_navigation, "AppController::switchChatTab")
        self.assertIn("target == AgentTabPage", switch_tab_body)
        self.assertIn("_features.agentController.loadSessions();", switch_tab_body)
        self.assertIn("_features.agentController.refreshModelList();", switch_tab_body)

        refresh_model_body = function_body(models, "refreshModelList")
        self.assertIn("if (_model_refresh_busy)", refresh_model_body)
        self.assertLess(
            refresh_model_body.find("if (_model_refresh_busy)"),
            refresh_model_body.find("setModelRefreshBusy(true);"),
        )

        long_ai_body = free_function_body(request_utils, "isLongAiRequest")
        self.assertIn('QStringLiteral("/ai/chat")', long_ai_body)
        self.assertIn('QStringLiteral("/ai/chat/stream")', long_ai_body)
        self.assertIn('QStringLiteral("/ai/model/api/register")', long_ai_body)
        self.assertNotIn('QStringLiteral("/ai/session")', long_ai_body)
        self.assertNotIn('QStringLiteral("/ai/session/list")', long_ai_body)
        self.assertNotIn('QStringLiteral("/ai/history")', long_ai_body)

        self.assertIn("request.setTransferTimeout(10000);", direct_agent_request_utils)


if __name__ == "__main__":
    unittest.main()
