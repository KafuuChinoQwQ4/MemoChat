import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
AI_ROUTE_MODULES = REPO_ROOT / "apps/server/core/GateServer/AIRouteModules.cpp"
PET_SMOKE = REPO_ROOT / "tools/scripts/pet_smoke.ps1"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class PetGateBoundaryContractTests(unittest.TestCase):
    def test_gate_proxy_maps_ai_pet_to_orchestrator_pet(self):
        source = read(AI_ROUTE_MODULES)

        self.assertIn("ProxyAiOrchestratorPrefix(", source)
        self.assertIn('"/ai/pet"', source)
        self.assertIn('"/pet"', source)
        self.assertIn('"gate.ai.pet.proxy"', source)
        self.assertIn('logic.RegGetPrefix("/ai/pet"', source)
        self.assertIn('logic.RegPostPrefix("/ai/pet"', source)

    def test_gate_pet_proxy_forwards_trace_and_request_headers(self):
        source = read(AI_ROUTE_MODULES)

        self.assertGreaterEqual(source.count('req.set("X-Trace-Id"'), 2)
        self.assertGreaterEqual(source.count('req.set("X-Request-Id"'), 2)
        self.assertIn('connection ? connection->GetTraceId() : ""', source)
        self.assertIn('connection ? connection->GetRequestId() : ""', source)
        self.assertIn("res.base().find(http::field::content_type)", source)
        self.assertIn("upstream_content_type.empty()", source)

    def test_gate_pet_stream_uses_single_stream_start_path(self):
        source = read(AI_ROUTE_MODULES)
        route_match = re.search(
            r'logic\.RegGetPrefix\("/ai/pet".*?logic\.RegPostPrefix\("/ai/pet"',
            source,
            flags=re.S,
        )
        self.assertIsNotNone(route_match)
        route_block = route_match.group(0)

        self.assertIn("ProxyAiOrchestratorPetStream(connection)", route_block)
        self.assertIn("connection->StartSseStream();", route_block)

        proxy_match = re.search(
            r"static void ProxyAiOrchestratorPetStream.*?static void AIHttpServiceRoutes_PLACEHOLDER",
            source.replace("void AIHttpServiceRoutes::RegisterRoutes", "static void AIHttpServiceRoutes_PLACEHOLDER"),
            flags=re.S,
        )
        self.assertIsNotNone(proxy_match)
        self.assertNotIn("connection->StartSseStream();", proxy_match.group(0))

    def test_gate_pet_stream_marks_streaming_before_worker_dispatch(self):
        source = read(AI_ROUTE_MODULES)
        route_match = re.search(
            r'if \(target\.find\("/stream"\).*?return true;',
            source,
            flags=re.S,
        )
        self.assertIsNotNone(route_match)
        stream_block = route_match.group(0)

        self.assertLess(
            stream_block.find("connection->StartSseStream();"),
            stream_block.find("GateWorkerPool::GetInstance()->post"),
        )

    def test_gate_pet_stream_emits_structured_error_events(self):
        source = read(AI_ROUTE_MODULES)

        self.assertIn('event["type"] = "pet.error"', source)
        self.assertIn('"AIOrchestrator returned "', source)
        self.assertIn('"AIOrchestrator pet stream failed: "', source)
        self.assertIn('http::field::accept, "text/event-stream"', source)

    def test_pet_smoke_script_covers_session_input_observation_interrupt_stream(self):
        script = read(PET_SMOKE)

        for token in (
            "/ai/pet/sessions",
            "/input",
            "/observation",
            "/interrupt",
            "/stream",
            "text/event-stream",
            "X-Trace-Id",
            "X-Request-Id",
            "Parse-SseEvents",
            "ResponseHeadersRead",
        ):
            self.assertIn(token, script)


if __name__ == "__main__":
    unittest.main()
