import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
AGENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/agent"
AGENT_CONTROLLER_DIR = AGENT_DIR / "controller"
AGENT_GAME_DIR = AGENT_DIR / "game"
AGENT_MODEL_DIR = AGENT_DIR / "model"
AGENT_TRANSPORT_DIR = AGENT_DIR / "transport"
AGENT_SOURCES_CMAKE = AGENT_DIR / "sources.cmake"


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

        self.assertNotIn("#include <QNetworkReply>", text)
        self.assertNotIn("QNetworkReply* reply", text)
        self.assertNotIn("QJsonDocument::fromJson(body)", text)
        self.assertNotIn("_gameNetwork->get", text)
        self.assertNotIn("_gameNetwork->post", text)
        self.assertNotIn("_gameNetwork->deleteResource", text)
        self.assertIn("AgentGameClient", (AGENT_CONTROLLER_DIR / "AgentController.h").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
