import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
OPS_DIR = REPO_ROOT / "infra/Memo_ops/client/MemoOps-qml"
OPS_CMAKE = OPS_DIR / "CMakeLists.txt"


class MemoOpsClientBoundaryTests(unittest.TestCase):
    def test_ops_boundary_files_exist_and_are_registered(self):
        expected_files = (
            "OpsApiTransport.h",
            "OpsApiTransport.cpp",
            "OpsLogQueryBuilder.h",
            "OpsLogQueryBuilder.cpp",
            "OpsApiClientOverview.cpp",
            "OpsApiClientLogs.cpp",
            "OpsApiClientMetrics.cpp",
            "OpsApiClientLoadtests.cpp",
            "OpsApiClientAdmin.cpp",
        )

        for file_name in expected_files:
            with self.subTest(file=file_name):
                self.assertTrue((OPS_DIR / file_name).is_file())

        cmake_text = OPS_CMAKE.read_text(encoding="utf-8")
        for file_name in expected_files:
            with self.subTest(cmake_file=file_name):
                self.assertIn(file_name, cmake_text)

    def test_ops_api_client_cpp_is_shell_only(self):
        text = (OPS_DIR / "OpsApiClient.cpp").read_text(encoding="utf-8")
        non_empty_lines = [line for line in text.splitlines() if line.strip()]

        self.assertLessEqual(len(non_empty_lines), 180)
        self.assertIn("OpsApiClient::OpsApiClient", text)
        self.assertIn("void OpsApiClient::refreshAll()", text)
        self.assertIn("void OpsApiClient::setBusy", text)
        self.assertIn("void OpsApiClient::setLastError", text)
        self.assertNotIn("void OpsApiClient::refreshLogSearch", text)
        self.assertNotIn("void OpsApiClient::refreshServices", text)
        self.assertNotIn("void OpsApiClient::startLoadtest", text)
        self.assertNotIn("void OpsApiClient::collectNow", text)

    def test_transport_and_query_builder_own_cross_cutting_logic(self):
        header_text = (OPS_DIR / "OpsApiClient.h").read_text(encoding="utf-8")
        cpp_text = (OPS_DIR / "OpsApiClient.cpp").read_text(encoding="utf-8")
        transport_text = "\n".join(
            (OPS_DIR / path).read_text(encoding="utf-8")
            for path in ("OpsApiTransport.h", "OpsApiTransport.cpp")
            if (OPS_DIR / path).exists()
        )
        query_text = "\n".join(
            (OPS_DIR / path).read_text(encoding="utf-8")
            for path in ("OpsLogQueryBuilder.h", "OpsLogQueryBuilder.cpp")
            if (OPS_DIR / path).exists()
        )

        self.assertNotIn("QNetworkAccessManager m_network", header_text)
        self.assertNotIn("void getJson", header_text)
        self.assertNotIn("void postJson", header_text)
        self.assertNotIn("QNetworkReply", cpp_text)
        self.assertNotIn("QNetworkRequest", cpp_text)
        self.assertNotIn("QJsonDocument::fromJson(body)", cpp_text)
        self.assertIn("void OpsApiTransport::getJson", transport_text)
        self.assertIn("void OpsApiTransport::postJson", transport_text)
        self.assertIn("void OpsApiTransport::postJsonWithQuery", transport_text)
        self.assertIn("buildLogFilterState", query_text)
        self.assertIn("buildLogQuery", query_text)

    def test_public_qml_contract_names_remain_stable(self):
        header_text = (OPS_DIR / "OpsApiClient.h").read_text(encoding="utf-8")
        expected_properties = (
            "Q_PROPERTY(QJsonObject overview READ overview NOTIFY overviewChanged)",
            "Q_PROPERTY(QJsonArray runs READ runs NOTIFY runsChanged)",
            "Q_PROPERTY(QJsonArray logs READ logs NOTIFY logsChanged)",
            "Q_PROPERTY(QJsonArray services READ services NOTIFY servicesChanged)",
            "Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)",
        )
        expected_invokables = (
            "refreshAll",
            "refreshOverview",
            "refreshRuns",
            "refreshLogSearch",
            "refreshLogTrend",
            "refreshTrace",
            "refreshServices",
            "refreshLoadtestTrend",
            "refreshAlerts",
            "refreshDataSources",
            "collectNow",
            "importReports",
            "importLogs",
            "refreshSystemMetrics",
            "refreshLoadtestStatus",
            "startLoadtest",
            "fetchTailLogs",
            "stopTailLogs",
        )

        for prop in expected_properties:
            with self.subTest(property=prop):
                self.assertIn(prop, header_text)
        for invokable in expected_invokables:
            with self.subTest(invokable=invokable):
                self.assertRegex(header_text, rf"Q_INVOKABLE[\s\S]{{0,120}}\b{re.escape(invokable)}\s*\(")


if __name__ == "__main__":
    unittest.main()
