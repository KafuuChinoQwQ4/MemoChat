from __future__ import annotations

import json
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace

from harness.contracts import (
    AgentCapabilities,
    AgentCard,
    AgentCardSkill,
    AgentSkill,
    AgentFlow,
    AgentMessage,
    AgentSpec,
    AgentTask,
    AgentTrace,
    HandoffStep,
    ContextPack,
    ContextSection,
    GuardrailResult,
    MemorySnapshot,
    PlanStep,
    RemoteAgentRef,
    RemoteAgentTask,
    RunEdge,
    RunGraph,
    RunNode,
    TraceEvent,
    ToolObservation,
    ToolSpec,
)
from harness.evals.service import AgentEvalCase, AgentEvalService
from harness.execution.tool_executor import ToolExecutor
from harness.feedback.trace_store import AgentTraceStore
from harness.guardrails.service import GuardrailService
from harness.handoffs.service import AgentHandoffService
from harness.interop.service import AgentInteropService
from harness.layers import list_harness_layers
from harness.memory.service import MemoryService
from harness.orchestration.agent_service import AgentHarnessService
from harness.orchestration.planner import PlanningPolicy
from harness.runtime.task_service import AgentTaskService
from harness.skills.registry import SkillRegistry
from harness.skills.specs import AgentSpecRegistry
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

    def list_tool_specs(self):
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


class RecordingLLM:
    def __init__(self):
        self.last_call: dict = {}

    async def complete(self, messages, prefer_backend="", model_name="", deployment_preference="any", **kwargs):
        self.last_call = {
            "prefer_backend": prefer_backend,
            "model_name": model_name,
            "deployment_preference": deployment_preference,
            **kwargs,
        }
        return LLMResponse(content="answer", model=model_name or "fake-model", usage=LLMUsage(total_tokens=2))


class FakeFlowAgentService:
    def __init__(self):
        self.calls: list[SimpleNamespace] = []

    async def run_turn(self, request):
        self.calls.append(request)
        index = len(self.calls)
        return SimpleNamespace(
            session_id=request.session_id or f"session-{index}",
            content=f"{request.skill_name} output {index}",
            tokens=2,
            model="fake-model",
            trace_id=f"trace-{index}",
            skill=request.skill_name,
            feedback_summary="ok",
            observations=[],
            events=[],
        )


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
    def test_stage_one_contracts_are_serializable(self):
        agent = AgentSpec(
            name="researcher",
            display_name="Researcher",
            description="Research assistant",
            system_prompt="Use cited evidence.",
            default_tools=["knowledge_base_search"],
            allowed_tools=["knowledge_base_search", "duckduckgo_search"],
            memory_policy={"include_graph": True},
        )
        tool = ToolSpec(
            name="delete_kb",
            display_name="Delete KB",
            description="Delete a knowledge base",
            source="mcp",
            permission="write",
            requires_confirmation=True,
            parameters_schema={"type": "object"},
        )
        guardrail = GuardrailResult(
            name="tool_confirmation",
            status="block",
            message="confirmation required",
            blocking=True,
        )
        graph = RunGraph(
            trace_id="trace-1",
            nodes=[
                RunNode(
                    node_id="n1",
                    name="plan",
                    layer="orchestration",
                    status="ok",
                    started_at=100,
                    finished_at=140,
                )
            ],
            edges=[RunEdge(source_id="n1", target_id="n2")],
            status="completed",
        )
        context_pack = ContextPack(
            sections=[
                ContextSection(
                    name="episodic",
                    source="memory",
                    content="User prefers concise replies.",
                    token_count=6,
                )
            ],
            token_budget=2048,
            source_metadata={"session_id": "s1"},
        )
        task = AgentTask(
            task_id="task-1",
            title="Run research",
            status="queued",
            payload={"query": "MemoChat"},
            checkpoints=[{"status": "queued"}],
        )
        message = AgentMessage(
            message_id="message-1",
            flow_id="flow-1",
            from_agent="researcher",
            to_agent="writer",
            role="handoff",
            content="evidence",
        )
        flow = AgentFlow(
            name="research_write_review",
            display_name="Research Write Review",
            description="test",
            steps=[HandoffStep(step_id="research", agent_name="researcher", instruction="find evidence")],
        )
        card = AgentCard(
            name="MemoChat",
            description="Local agent",
            url="http://localhost:8096/agent/a2a",
            version="0.10.0",
            capabilities=AgentCapabilities(streaming=True, state_transition_history=True),
            skills=[
                AgentCardSkill(
                    id="agent:researcher",
                    name="Researcher",
                    description="Research",
                    input_modes=["text/plain"],
                    output_modes=["text/plain"],
                )
            ],
        )
        remote = RemoteAgentRef(
            peer_id="peer-1",
            name="Remote",
            card_url="https://example.test/.well-known/agent-card.json",
        )
        remote_task = RemoteAgentTask(task_id="remote-task-1", peer_id=remote.peer_id)

        self.assertEqual(agent.to_dict()["memory_policy"]["include_graph"], True)
        self.assertEqual(tool.to_dict()["requires_confirmation"], True)
        self.assertEqual(guardrail.to_dict()["status"], "block")
        self.assertEqual(graph.nodes[0].duration_ms(), 40)
        self.assertEqual(graph.to_dict()["edges"][0]["relation"], "next")
        self.assertEqual(context_pack.to_dict()["sections"][0]["source"], "memory")
        self.assertEqual(task.to_dict()["checkpoints"][0]["status"], "queued")
        self.assertEqual(message.to_dict()["role"], "handoff")
        self.assertEqual(flow.to_dict()["steps"][0]["agent_name"], "researcher")
        self.assertEqual(card.to_dict()["defaultInputModes"], ["text/plain"])
        self.assertEqual(card.to_dict()["protocolVersion"], "0.3.0")
        self.assertEqual(card.to_dict()["capabilities"]["stateTransitionHistory"], True)
        self.assertEqual(card.to_dict()["skills"][0]["inputModes"], ["text/plain"])
        self.assertEqual(remote.to_dict()["status"], "registered")
        self.assertEqual(remote_task.to_dict()["peer_id"], "peer-1")

    def test_tool_specs_include_permissions_and_validation(self):
        executor = ToolExecutor.__new__(ToolExecutor)
        calculator_spec = executor._builtin_tool_specs()["calculator"]

        self.assertEqual(calculator_spec.permission, "read")
        self.assertFalse(calculator_spec.requires_confirmation)
        self.assertEqual(calculator_spec.parameters_schema["required"], ["expression"])
        self.assertEqual(
            executor._validate_tool_payload(calculator_spec, {"expression": "1 + 1"}),
            {"expression": "1 + 1"},
        )

        with self.assertRaises(ValueError):
            executor._validate_tool_payload(calculator_spec, {})

        with self.assertRaises(ValueError):
            executor._validate_tool_payload(calculator_spec, {"expression": 2})

    def test_write_tool_requires_confirmation_before_execution(self):
        executor = ToolExecutor.__new__(ToolExecutor)
        spec = ToolSpec(
            name="mcp_filesystem_delete_file",
            display_name="Delete File",
            description="Delete a file",
            source="mcp",
            permission="admin",
            requires_confirmation=True,
            parameters_schema={
                "type": "object",
                "required": ["path"],
                "properties": {"path": {"type": "string"}},
            },
        )

        with self.assertRaises(PermissionError):
            executor._require_confirmation_if_needed(spec, {"path": "a.txt"})

        executor._require_confirmation_if_needed(spec, {"path": "a.txt", "confirmed": True})
        self.assertEqual(
            executor._validate_tool_payload(spec, {"path": "a.txt", "confirmed": True}),
            {"path": "a.txt"},
        )

    def test_guardrail_blocks_unconfirmed_write_tool(self):
        service = GuardrailService()
        request = SimpleNamespace(
            requested_tools=["mcp_filesystem_delete_file"],
            tool_arguments={"mcp_filesystem_delete_file": {"path": "a.txt"}},
        )
        spec = ToolSpec(
            name="mcp_filesystem_delete_file",
            display_name="Delete File",
            description="Delete a file",
            source="mcp",
            permission="admin",
            requires_confirmation=True,
        )

        results = service.check_tool_plan(
            request,
            [PlanStep(action="mcp_tool", reason="test")],
            [spec],
        )

        self.assertTrue(service.has_blocking(results))
        self.assertEqual(results[0].name, "tool_confirmation")

    def test_guardrail_warns_on_thinking_markup(self):
        service = GuardrailService()
        results = service.check_output("<think>hidden</think> answer", [])

        self.assertFalse(service.has_blocking(results))
        self.assertIn("warn", {result.status for result in results})

    def test_memory_service_builds_context_pack_sections(self):
        service = MemoryService()
        pack = service._build_context_pack(
            uid=1,
            session_id="session-1",
            chat_history=[],
            episodic=["用户喜欢简洁回答"],
            semantic={"likes": "简洁"},
            graph_context=[{"friend": "alice"}],
            system_messages=[],
        )

        section_names = {section.name for section in pack.sections}
        self.assertIn("episodic_summaries", section_names)
        self.assertIn("semantic_profile", section_names)
        self.assertIn("graph_context", section_names)
        self.assertEqual(pack.source_metadata["uid"], 1)
        self.assertGreater(sum(section.token_count for section in pack.sections), 0)

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

    def test_agent_spec_registry_exposes_builtin_templates_as_skills(self):
        registry = SkillRegistry()
        spec_registry = AgentSpecRegistry()
        spec_names = {spec.name for spec in spec_registry.list_specs()}

        self.assertEqual(
            spec_names,
            {"researcher", "writer", "reviewer", "support", "knowledge_curator"},
        )
        researcher = registry.resolve("researcher", "随便问", "")
        curator = registry.resolve("knowledge_curator", "整理知识", "")

        self.assertEqual(researcher.name, "researcher")
        self.assertTrue(researcher.allow_web)
        self.assertTrue(curator.allow_graph)
        self.assertIn("model_policy", reviewer_metadata := registry.resolve("reviewer", "审阅", "").metadata)
        self.assertEqual(reviewer_metadata["model_policy"]["temperature"], 0.2)

    def test_agent_spec_templates_drive_planner_actions(self):
        registry = SkillRegistry()
        planner = PlanningPolicy(registry)
        request = SimpleNamespace(
            skill_name="knowledge_curator",
            content="整理这些知识",
            requested_tools=[],
        )
        skill = planner.resolve_skill(request)
        actions = [step.action for step in planner.build_plan(request, skill)]

        self.assertEqual(skill.name, "knowledge_curator")
        self.assertIn("knowledge_search", actions)
        self.assertIn("graph_recall", actions)

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

    def test_trace_store_projects_linear_trace_to_run_graph(self):
        store = AgentTraceStore()
        trace = AgentTrace(
            trace_id="trace-graph",
            uid=1,
            session_id="session-1",
            skill="knowledge_copilot",
            request_summary="question",
            plan_steps=[PlanStep(action="knowledge_search", reason="kb")],
            events=[
                TraceEvent(
                    layer="memory",
                    name="load_context",
                    status="ok",
                    summary="history=0",
                    started_at=100,
                    finished_at=120,
                ),
                TraceEvent(
                    layer="execution",
                    name="tool_execution",
                    status="ok",
                    summary="observations=1",
                    started_at=130,
                    finished_at=160,
                    metadata={"tool_count": 1},
                ),
            ],
            status="completed",
            started_at=90,
            finished_at=200,
        )

        graph = store.build_run_graph(trace)

        self.assertEqual(graph.trace_id, trace.trace_id)
        self.assertEqual(graph.status, "completed")
        self.assertEqual(len(graph.nodes), 3)
        self.assertEqual(len(graph.edges), 2)
        self.assertEqual(graph.nodes[0].name, "plan")
        self.assertEqual(graph.nodes[1].parent_id, graph.nodes[0].node_id)
        self.assertEqual(graph.edges[0].relation, "next")
        self.assertEqual(graph.to_dict()["metadata"]["skill"], "knowledge_copilot")

    def test_eval_service_passes_trace_shape_expectations(self):
        service = AgentEvalService(agent_service=None, trace_store=None)
        case = AgentEvalCase(
            case_id="shape",
            name="Shape",
            expectations={
                "expected_status": "completed",
                "required_layers": ["orchestration", "guardrails", "memory", "execution", "feedback"],
                "required_events": ["plan", "load_context", "tool_execution", "model_completion"],
                "required_tools": ["knowledge_search"],
                "max_latency_ms": 500,
            },
        )
        trace = AgentTrace(
            trace_id="trace-eval",
            uid=1,
            session_id="session-1",
            skill="knowledge_copilot",
            request_summary="question",
            status="completed",
            model="fake-model",
            observations=["[knowledge_search] doc"],
            started_at=100,
            finished_at=240,
            events=[
                TraceEvent(layer="orchestration", name="plan", status="ok"),
                TraceEvent(layer="guardrails", name="input", status="ok"),
                TraceEvent(layer="memory", name="load_context", status="ok"),
                TraceEvent(layer="execution", name="tool_execution", status="ok", detail="[knowledge_search] doc"),
                TraceEvent(layer="execution", name="model_completion", status="ok"),
                TraceEvent(layer="feedback", name="evaluate_response", status="ok"),
            ],
        )

        result = service.evaluate_trace(case, trace)

        self.assertTrue(result["passed"], result["failures"])
        self.assertEqual(result["metrics"]["tools"], ["knowledge_search"])

    def test_eval_service_checks_guardrail_block_shape(self):
        service = AgentEvalService(agent_service=None, trace_store=None)
        case = AgentEvalCase(
            case_id="guardrail",
            name="Guardrail",
            expectations={
                "expected_status": "blocked",
                "required_layers": ["orchestration", "guardrails"],
                "required_guardrail_statuses": {"input_not_empty": "block"},
                "forbidden_events": ["model_completion"],
            },
        )
        trace = AgentTrace(
            trace_id="trace-blocked",
            uid=1,
            session_id="",
            skill="knowledge_copilot",
            request_summary="",
            status="blocked",
            started_at=100,
            finished_at=120,
            events=[
                TraceEvent(layer="orchestration", name="plan", status="ok"),
                TraceEvent(
                    layer="guardrails",
                    name="input",
                    status="blocked",
                    metadata={"results": [{"name": "input_not_empty", "status": "block"}]},
                ),
            ],
        )

        result = service.evaluate_trace(case, trace)

        self.assertTrue(result["passed"], result["failures"])
        self.assertEqual(result["metrics"]["guardrails"]["input_not_empty"], "block")

    def test_eval_service_reports_missing_expected_layer(self):
        service = AgentEvalService(agent_service=None, trace_store=None)
        case = AgentEvalCase(
            case_id="missing",
            name="Missing",
            expectations={"required_layers": ["feedback"]},
        )
        trace = AgentTrace(
            trace_id="trace-missing",
            uid=1,
            session_id="",
            skill="knowledge_copilot",
            request_summary="question",
            status="completed",
            events=[TraceEvent(layer="orchestration", name="plan", status="ok")],
        )

        result = service.evaluate_trace(case, trace)

        self.assertFalse(result["passed"])
        self.assertIn("missing required layer: feedback", result["failures"])

    def test_handoff_service_exposes_builtin_flow_templates(self):
        service = AgentHandoffService(agent_service=FakeFlowAgentService())
        flows = {flow.name: flow for flow in service.list_flows()}

        self.assertIn("research_write_review", flows)
        self.assertIn("support_triage_answer", flows)
        self.assertIn("document_curate_review", flows)
        self.assertEqual(
            [step.agent_name for step in flows["research_write_review"].steps],
            ["researcher", "writer", "reviewer"],
        )

    def test_interop_service_builds_local_agent_card_from_specs_and_flows(self):
        handoff_service = AgentHandoffService(agent_service=FakeFlowAgentService())
        service = AgentInteropService(
            skill_registry=SkillRegistry(),
            handoff_service=handoff_service,
            public_base_url="http://localhost:8096",
        )

        card = service.local_agent_card()
        skill_ids = {skill.id for skill in card.skills}
        card_dict = card.to_dict()

        self.assertEqual(card.url, "http://localhost:8096/agent/a2a")
        self.assertIn("agent:researcher", skill_ids)
        self.assertIn("flow:research_write_review", skill_ids)
        self.assertTrue(card.capabilities.streaming)
        self.assertEqual(card_dict["protocolVersion"], "0.3.0")
        self.assertEqual(card_dict["additionalInterfaces"][0]["transport"], "JSONRPC")
        self.assertEqual(card_dict["capabilities"]["pushNotifications"], False)
        self.assertEqual(card_dict["metadata"]["remoteInvocation"], "disabled")

    def test_interop_service_registers_updates_and_deletes_remote_agents(self):
        service = AgentInteropService(
            skill_registry=SkillRegistry(),
            handoff_service=AgentHandoffService(agent_service=FakeFlowAgentService()),
        )

        first = service.register_remote_agent(
            peer_id="peer-1",
            name="Remote One",
            card_url="https://example.test/card.json",
            metadata={"source": "test"},
        )
        updated = service.register_remote_agent(
            peer_id="peer-1",
            name="Remote One v2",
            card_url="https://example.test/card-v2.json",
            trusted=True,
        )

        self.assertEqual(first.peer_id, updated.peer_id)
        self.assertEqual(len(service.list_remote_agents()), 1)
        self.assertTrue(updated.trusted)
        self.assertEqual(updated.metadata["remoteInvocation"], "disabled")
        self.assertTrue(service.delete_remote_agent("peer-1"))
        self.assertIsNone(service.get_remote_agent("peer-1"))

    def test_interop_service_blocks_remote_task_placeholder(self):
        service = AgentInteropService(
            skill_registry=SkillRegistry(),
            handoff_service=AgentHandoffService(agent_service=FakeFlowAgentService()),
        )
        service.register_remote_agent(
            peer_id="peer-1",
            name="Remote One",
            card_url="https://example.test/card.json",
        )

        task = service.create_remote_task_placeholder("peer-1", {"content": "hello"})

        self.assertEqual(task.status, "blocked")
        self.assertEqual(task.input["content"], "hello")
        self.assertIn("not enabled", task.error)
        with self.assertRaises(ValueError):
            service.create_remote_task_placeholder("missing", {})


class HarnessRunTraceTests(unittest.IsolatedAsyncioTestCase):
    async def test_run_turn_records_core_layer_events(self):
        service = AgentHarnessService(
            planner=FakePlanner(),
            llm_registry=FakeLLM(),
            tool_executor=FakeToolExecutor(),
            memory_service=FakeMemory(),
            trace_store=FakeTraceStore(),
            feedback_evaluator=FakeFeedback(),
            guardrail_service=GuardrailService(),
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
        guardrail_names = [event.name for event in result.events if event.layer == "guardrails"]

        self.assertEqual(result.skill, "knowledge_copilot")
        self.assertIn("orchestration", layers)
        self.assertIn("guardrails", layers)
        self.assertIn("memory", layers)
        self.assertIn("execution", layers)
        self.assertIn("feedback", layers)
        self.assertEqual(guardrail_names, ["input", "tool_plan", "output"])

    async def test_agent_spec_model_policy_supplies_default_model_call(self):
        llm = RecordingLLM()
        service = AgentHarnessService(
            planner=PlanningPolicy(SkillRegistry()),
            llm_registry=llm,
            tool_executor=FakeToolExecutor(),
            memory_service=FakeMemory(),
            trace_store=FakeTraceStore(),
            feedback_evaluator=FakeFeedback(),
            guardrail_service=GuardrailService(),
        )
        request = SimpleNamespace(
            uid=1,
            session_id="",
            content="请审阅这段内容",
            model_type="",
            model_name="",
            deployment_preference="",
            target_lang="",
            skill_name="reviewer",
            requested_tools=[],
            tool_arguments={},
            metadata={},
        )

        result = await service.run_turn(request)

        self.assertEqual(result.skill, "reviewer")
        self.assertEqual(llm.last_call["prefer_backend"], "ollama")
        self.assertEqual(llm.last_call["model_name"], "qwen3:4b")
        self.assertEqual(llm.last_call["temperature"], 0.2)

    async def test_handoff_service_runs_sequential_flow_and_graph(self):
        agent_service = FakeFlowAgentService()
        service = AgentHandoffService(agent_service=agent_service)

        result = await service.run_flow(
            flow_name="research_write_review",
            uid=1,
            content="调研并写一份说明",
        )

        self.assertEqual(result["status"], "completed")
        self.assertEqual(len(agent_service.calls), 3)
        self.assertEqual([call.skill_name for call in agent_service.calls], ["researcher", "writer", "reviewer"])
        self.assertEqual([message.role for message in result["messages"]], ["handoff", "handoff", "final"])
        self.assertEqual(result["messages"][0].to_agent, "writer")
        self.assertEqual(result["graph"].status, "completed")
        self.assertEqual(len(result["graph"].nodes), 3)
        self.assertEqual(len(result["graph"].edges), 2)
        self.assertEqual(result["graph"].edges[0].relation, "handoff")

    async def test_handoff_service_reports_missing_flow(self):
        service = AgentHandoffService(agent_service=FakeFlowAgentService())

        with self.assertRaises(ValueError):
            await service.run_flow(flow_name="missing", uid=1, content="hello")

    async def test_eval_service_can_run_live_case_with_fake_harness(self):
        trace_store = FakeTraceStore()
        agent_service = AgentHarnessService(
            planner=FakePlanner(),
            llm_registry=FakeLLM(),
            tool_executor=FakeToolExecutor(),
            memory_service=FakeMemory(),
            trace_store=trace_store,
            feedback_evaluator=FakeFeedback(),
            guardrail_service=GuardrailService(),
        )

        with TemporaryDirectory() as storage:
            Path(storage, "live.json").write_text(
                json.dumps(
                    {
                        "case_id": "live",
                        "name": "Live fake harness",
                        "request": {"uid": 1, "content": "根据知识库回答"},
                        "expectations": {
                            "expected_status": "completed",
                            "required_layers": ["orchestration", "guardrails", "memory", "execution", "feedback"],
                            "required_events": ["plan", "tool_execution", "model_completion"],
                            "required_tools": ["knowledge_search"],
                            "max_latency_ms": 10000,
                        },
                    }
                ),
                encoding="utf-8",
            )
            service = AgentEvalService(agent_service, trace_store, case_dir=storage)
            result = await service.run_eval(case_id="live")

        self.assertTrue(result["passed"], result["failures"])
        self.assertEqual(result["trace_id"], "trace-1")

    async def test_agent_task_service_persists_and_resumes_lifecycle(self):
        with TemporaryDirectory() as storage:
            service = AgentTaskService(
                agent_service=SimpleNamespace(run_turn=None),
                storage_dir=storage,
                auto_run=False,
            )
            await service.startup()
            task = await service.create_task(uid=7, title="Research", content="Do durable work")

            listed = await service.list_tasks(uid=7)
            self.assertEqual(len(listed), 1)
            self.assertEqual(listed[0].task_id, task.task_id)

            canceled = await service.cancel_task(task.task_id)
            self.assertIsNotNone(canceled)
            self.assertEqual(canceled.status, "canceled")

            restored = AgentTaskService(
                agent_service=SimpleNamespace(run_turn=None),
                storage_dir=storage,
                auto_run=False,
            )
            await restored.startup()
            loaded = await restored.get_task(task.task_id)
            self.assertIsNotNone(loaded)
            self.assertEqual(loaded.status, "canceled")

            resumed = await restored.resume_task(task.task_id)
            self.assertIsNotNone(resumed)
            self.assertEqual(resumed.status, "queued")


if __name__ == "__main__":
    unittest.main()
