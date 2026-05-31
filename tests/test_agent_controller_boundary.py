import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
AGENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/agent"
QML_CMAKE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/CMakeLists.txt"


class AgentControllerBoundaryTests(unittest.TestCase):
    def test_agent_request_tracker_files_exist_and_are_registered(self):
        expected_files = (
            "AgentRequestTypes.h",
            "AgentRequestTracker.h",
            "AgentRequestTracker.cpp",
        )

        for file_name in expected_files:
            with self.subTest(file=file_name):
                self.assertTrue((AGENT_DIR / file_name).is_file())

        cmake_text = QML_CMAKE.read_text(encoding="utf-8")
        self.assertIn("features/agent/AgentRequestTracker.cpp", cmake_text)
        self.assertIn("features/agent/AgentRequestTypes.h", cmake_text)
        self.assertIn("features/agent/AgentRequestTracker.h", cmake_text)

    def test_agent_controller_uses_typed_request_tracker(self):
        header_text = (AGENT_DIR / "AgentController.h").read_text(encoding="utf-8")

        self.assertIn('#include "AgentRequestTracker.h"', header_text)
        self.assertIn("AgentRequestTracker _pending_requests;", header_text)
        self.assertNotIn("QMap<ReqId, QString> _pending_requests", header_text)

    def test_agent_request_files_do_not_assign_string_request_types(self):
        request_files = (
            "AgentControllerChat.cpp",
            "AgentControllerModels.cpp",
            "AgentControllerSessions.cpp",
            "AgentControllerKnowledge.cpp",
            "AgentControllerMemory.cpp",
            "AgentControllerAgentTasks.cpp",
        )

        string_assignment = re.compile(r"_pending_requests\s*\[\s*reqId\s*\]\s*=\s*\"[^\"]+\"")

        for file_name in request_files:
            with self.subTest(file=file_name):
                text = (AGENT_DIR / file_name).read_text(encoding="utf-8")
                self.assertIsNone(string_assignment.search(text))
                self.assertIn("_pending_requests.track(reqId, AgentRequestKind::", text)

    def test_http_finish_routes_through_typed_request_kind(self):
        text = (AGENT_DIR / "AgentController.cpp").read_text(encoding="utf-8")

        self.assertIn("std::optional<AgentRequestRecord>", text)
        self.assertIn("AgentRequestKind::", text)
        self.assertIn("switch (record.kind)", text)
        self.assertNotIn("QString reqType", text)
        self.assertNotIn('reqType == "model_list"', text)
        self.assertNotIn('reqType == "kb_upload"', text)

    def test_agent_direct_network_client_files_exist_and_are_registered(self):
        expected_files = (
            "AgentNetworkRequestUtils.h",
            "AgentStreamClient.h",
            "AgentStreamClient.cpp",
            "AgentGameClient.h",
            "AgentGameClient.cpp",
        )

        for file_name in expected_files:
            with self.subTest(file=file_name):
                self.assertTrue((AGENT_DIR / file_name).is_file())

        cmake_text = QML_CMAKE.read_text(encoding="utf-8")
        for file_name in expected_files:
            with self.subTest(cmake_file=file_name):
                self.assertIn(f"features/agent/{file_name}", cmake_text)

    def test_agent_tls_request_helper_is_not_duplicated(self):
        helper_paths = (
            AGENT_DIR / "AgentNetworkRequestUtils.h",
            AGENT_DIR / "AgentNetworkRequestUtils.cpp",
        )
        helper_text = "\n".join(path.read_text(encoding="utf-8") for path in helper_paths if path.exists())
        stream_text = (AGENT_DIR / "AgentControllerStream.cpp").read_text(encoding="utf-8")
        game_text = (AGENT_DIR / "AgentControllerGameNetwork.cpp").read_text(encoding="utf-8")

        self.assertEqual(1, helper_text.count("setPeerVerifyMode(QSslSocket::VerifyNone)"))
        self.assertNotIn("setPeerVerifyMode(QSslSocket::VerifyNone)", stream_text)
        self.assertNotIn("setPeerVerifyMode(QSslSocket::VerifyNone)", game_text)
        self.assertNotIn("QSslConfiguration", stream_text)
        self.assertNotIn("QSslConfiguration", game_text)

    def test_stream_controller_no_longer_owns_raw_network_request(self):
        text = (AGENT_DIR / "AgentControllerStream.cpp").read_text(encoding="utf-8")

        self.assertNotIn("#include <QNetworkRequest>", text)
        self.assertNotIn("QNetworkRequest request", text)
        self.assertNotIn("_streamManager->post", text)
        self.assertIn("AgentStreamClient", (AGENT_DIR / "AgentController.h").read_text(encoding="utf-8"))

    def test_game_network_controller_uses_game_client_for_reply_parsing(self):
        text = (AGENT_DIR / "AgentControllerGameNetwork.cpp").read_text(encoding="utf-8")

        self.assertNotIn("#include <QNetworkReply>", text)
        self.assertNotIn("QNetworkReply* reply", text)
        self.assertNotIn("QJsonDocument::fromJson(body)", text)
        self.assertNotIn("_gameNetwork->get", text)
        self.assertNotIn("_gameNetwork->post", text)
        self.assertNotIn("_gameNetwork->deleteResource", text)
        self.assertIn("AgentGameClient", (AGENT_DIR / "AgentController.h").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
