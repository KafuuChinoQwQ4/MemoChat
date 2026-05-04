import csv
import importlib.util
import json
import socket
import struct
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = REPO_ROOT / "tools" / "loadtest" / "python-loadtest" / "py_loadtest.py"


def load_module():
    spec = importlib.util.spec_from_file_location("py_loadtest", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class PythonLoadtestTests(unittest.TestCase):
    def test_percentiles_use_nearest_rank_values(self):
        mod = load_module()
        stats = mod.summarize_latencies([1.0, 2.0, 3.0, 4.0, 100.0])

        self.assertEqual(stats["min_ms"], 1.0)
        self.assertEqual(stats["avg_ms"], 22.0)
        self.assertEqual(stats["p50_ms"], 3.0)
        self.assertEqual(stats["p95_ms"], 100.0)
        self.assertEqual(stats["p99_ms"], 100.0)
        self.assertEqual(stats["max_ms"], 100.0)

    def test_load_accounts_prefers_last_password_when_requested(self):
        mod = load_module()
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            csv_path = Path(tmp) / "accounts.csv"
            with csv_path.open("w", newline="", encoding="utf-8") as f:
                writer = csv.DictWriter(f, fieldnames=["email", "password", "last_password", "uid"])
                writer.writeheader()
                writer.writerow(
                    {
                        "email": "a@example.test",
                        "password": "old-pass",
                        "last_password": "new-pass",
                        "uid": "42",
                    }
                )

            accounts = mod.load_accounts(csv_path, prefer_last_password=True)

        self.assertEqual(len(accounts), 1)
        self.assertEqual(accounts[0].email, "a@example.test")
        self.assertEqual(accounts[0].password, "new-pass")
        self.assertEqual(accounts[0].uid, "42")

    def test_encode_frame_uses_network_order_header(self):
        mod = load_module()
        frame = mod.encode_frame(1005, b'{"uid":42}')

        self.assertEqual(frame[:4], struct.pack("!HH", 1005, 10))
        self.assertEqual(frame[4:], b'{"uid":42}')

    def test_tcp_chat_login_round_trip_with_fake_server(self):
        mod = load_module()

        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.bind(("127.0.0.1", 0))
        server.listen(1)
        host, port = server.getsockname()
        captured = {}

        def handle_once():
            conn, _ = server.accept()
            with conn:
                header = conn.recv(4)
                msg_id, size = struct.unpack("!HH", header)
                payload = conn.recv(size)
                captured["msg_id"] = msg_id
                captured["payload"] = json.loads(payload.decode("utf-8"))
                conn.sendall(mod.encode_frame(1006, b'{"error":0}'))
            server.close()

        import threading

        thread = threading.Thread(target=handle_once, daemon=True)
        thread.start()

        gate_response = {
            "uid": 42,
            "token": "token",
            "login_ticket": "ticket",
            "ticket_expire_ms": 123456789,
        }
        result = mod.tcp_chat_login(host, port, gate_response, timeout=2.0)
        thread.join(timeout=2.0)

        self.assertTrue(result.ok)
        self.assertEqual(result.error_code, 0)
        self.assertEqual(captured["msg_id"], 1005)
        self.assertEqual(captured["payload"]["uid"], 42)
        self.assertEqual(captured["payload"]["login_ticket"], "ticket")

    def test_build_report_includes_quality_fields(self):
        mod = load_module()
        result = mod.ScenarioResult(
            scenario="rag_search",
            total=2,
            success=1,
            failed=1,
            latencies_ms=[10.0, 30.0],
            started_at="2026-05-03T00:00:00Z",
            finished_at="2026-05-03T00:00:01Z",
            wall_elapsed_sec=0.05,
            errors={"timeout": 1},
            quality={"recall_at_3": 0.5, "hallucination_rate": 0.25},
        )

        report = mod.build_report(result)

        self.assertEqual(report["summary"]["success_rate"], 0.5)
        self.assertEqual(report["summary"]["wall_elapsed_sec"], 0.05)
        self.assertEqual(report["summary"]["throughput_rps"], 40.0)
        self.assertEqual(report["summary"]["latency_ms"]["p95_ms"], 30.0)
        self.assertEqual(report["quality"]["recall_at_3"], 0.5)
        self.assertEqual(report["quality"]["hallucination_rate"], 0.25)


if __name__ == "__main__":
    unittest.main()
