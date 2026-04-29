from __future__ import annotations

import unittest
from types import SimpleNamespace

from harness.contracts import AgentSkill, AgentTrace, MemorySnapshot, PlanStep, ToolObservation
from harness.feedback.trace_store import AgentTraceStore
from harness.layers import list_harness_layers
from harness.orchestration.agent_service import AgentHarnessService
from harness.skills.registry import SkillRegistry
from llm.base import LLMResponse, LLMUsage


class FakePlanner:
    def resolve_skill(self, request):
        return AgentSkill(
            name="knowledge_copilot",
            display_name="Knowledge",
            description="",
            system_prompt="system",
            default_actions=["knowledge_search"],
            allow_knowledge=True,
        )

    def build_plan(self, request, skill):
        return [PlanStep(action="knowledge_search", reason="test")]

    def build_system_prompt(self, skill, memory_snapshot, observations):
        return skill.system_prompt

    def build_user_prompt(self, request, observations):
        return request.content


class FakeMemory:
    async def load(self, uid: int, session_id: str, include_graph: bool = False):
        return MemorySnapshot()

    async def save_after_response(self, uid: int, session_id: str, user_message: str, ai_message: str):
        return None


class FakeToolExecutor:
    def list_tools(self):
        return []

    async def execute(
        self,
        uid: int,
        content: str,
        plan_steps: list[PlanStep],
        target_lang: str = "",
        requested_tools: list[str] | None = None,
        tool_arguments: dict[str, dict] | None = None,
    ):
        return [ToolObservation(name="knowledge_search", source="builtin", output="doc")]


class FakeLLM:
    async def complete(self, messages, prefer_backend="", model_name="", deployment_preference="any", **kwargs):
        return LLMResponse(content="answer", model="fake-model", usage=LLMUsage(total_tokens=2))


class FakeTraceStore:
    def __init__(self):
        self.traces: dict[str, AgentTrace] = {}

    async def start_run(self, uid, session_id, skill_name, request_summary, plan_steps):
        trace = AgentTrace(
            trace_id="trace-1",
            uid=uid,
            session_id=session_id,
            skill=skill_name,
            request_summary=request_summary,
            plan_steps=plan_steps,
        )
        self.traces[trace.trace_id] = trace
        return trace

    async def add_event(self, trace_id, event):
        self.traces[trace_id].events.append(event)

    async def finish_run(self, trace_id, status, response_content, model, feedback_summary, observations):
        trace = self.traces[trace_id]
        trace.status = status
        trace.response_content = response_content
        trace.model = model
        trace.feedback_summary = feedback_summary
        trace.observations = observations

    def get_trace(self, trace_id):
        return self.traces.get(trace_id)

    async def get_trace_or_load(self, trace_id):
        return self.get_trace(trace_id)


class FakeFeedback:
    def build_summary(self, skill_name, observations, response_text):
        return f"skill={skill_name}; tools={len(observations)}"


class HarnessStructureTests(unittest.TestCase):
    def test_layer_catalog_contains_expected_boundaries(self):
        layers = list_harness_layers()
        names = {layer["name"] for layer in layers}

        self.assertIn("orchestration", names)
        self.assertIn("execution", names)
        self.assertIn("feedback", names)
        self.assertIn("memory", names)
        self.assertIn("runtime", names)

        layers[0]["responsibilities"].append("mutated")
        fresh_layers = list_harness_layers()
        self.assertNotIn("mutated", fresh_layers[0]["responsibilities"])

    def test_skill_registry_resolves_core_agent_intents(self):
        registry = SkillRegistry()

        self.assertEqual(
            registry.resolve("", "根据知识库文档回答", "").name,
            "knowledge_copilot",
        )
        self.assertEqual(
            registry.resolve("", "帮我总结这段聊天", "").name,
            "summarize_thread",
        )
        self.assertEqual(
            registry.resolve("graph_recommender", "随便问", "").name,
            "graph_recommender",
        )

    def test_trace_store_decodes_persisted_trace_payloads(self):
        store = AgentTraceStore()
        steps = store._decode_plan_steps(
            '[{"action":"knowledge_search","reason":"kb","parameters":{"top_k":3}}]'
        )

        self.assertEqual(len(steps), 1)
        self.assertEqual(steps[0].action, "knowledge_search")
        self.assertEqual(steps[0].parameters["top_k"], 3)

        event = store._decode_event(
            {
                "layer": "execution",
                "step_name": "tool_execution",
                "status": "ok",
                "summary": "observations=1",
                "detail": "done",
                "metadata_json": '{"tokens": 5}',
                "duration_ms": 20,
                "created_at": 1000,
            }
        )

        self.assertEqual(event.layer, "execution")
        self.assertEqual(event.name, "tool_execution")
        self.assertEqual(event.started_at, 980)
        self.assertEqual(event.metadata["tokens"], 5)


class HarnessRunTraceTests(unittest.IsolatedAsyncioTestCase):
    async def test_run_turn_records_core_layer_events(self):
        service = AgentHarnessService(
            planner=FakePlanner(),
            llm_registry=FakeLLM(),
            tool_executor=FakeToolExecutor(),
            memory_service=FakeMemory(),
            trace_store=FakeTraceStore(),
            feedback_evaluator=FakeFeedback(),
        )
        request = SimpleNamespace(
            uid=1,
            session_id="",
            content="根据知识库回答",
            model_type="",
            model_name="",
            deployment_preference="any",
            target_lang="",
            requested_tools=[],
            tool_arguments={},
        )

        result = await service.run_turn(request)
        layers = [event.layer for event in result.events]

        self.assertEqual(result.skill, "knowledge_copilot")
        self.assertIn("orchestration", layers)
        self.assertIn("memory", layers)
        self.assertIn("execution", layers)
        self.assertIn("feedback", layers)


if __name__ == "__main__":
    unittest.main()
