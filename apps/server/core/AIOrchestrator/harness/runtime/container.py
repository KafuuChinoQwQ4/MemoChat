from __future__ import annotations

from harness.execution.tool_executor import ToolExecutor
from harness.evals.service import AgentEvalService
from harness.feedback.evaluator import FeedbackEvaluator
from harness.feedback.trace_store import AgentTraceStore
from harness.guardrails.service import GuardrailService
from harness.handoffs.service import AgentHandoffService
from harness.interop.service import AgentInteropService
from harness.knowledge.service import KnowledgeService
from harness.llm.service import LLMEndpointRegistry
from harness.mcp.service import MCPToolService
from harness.memory.graph_memory import GraphMemoryService
from harness.memory.service import MemoryService
from harness.orchestration.agent_service import AgentHarnessService
from harness.orchestration.planner import PlanningPolicy
from harness.runtime.task_service import AgentTaskService
from harness.skills.registry import SkillRegistry


class HarnessContainer:
    _instance: "HarnessContainer | None" = None

    def __init__(self):
        self.skill_registry = SkillRegistry()
        self.trace_store = AgentTraceStore()
        self.feedback_evaluator = FeedbackEvaluator()
        self.guardrail_service = GuardrailService()
        self.knowledge_service = KnowledgeService()
        self.graph_memory_service = GraphMemoryService()
        self.memory_service = MemoryService(self.graph_memory_service)
        self.llm_registry = LLMEndpointRegistry()
        self.mcp_service = MCPToolService()
        self.tool_executor = ToolExecutor(self.knowledge_service, self.graph_memory_service)
        self.planner = PlanningPolicy(self.skill_registry)
        self.agent_service = AgentHarnessService(
            planner=self.planner,
            llm_registry=self.llm_registry,
            tool_executor=self.tool_executor,
            memory_service=self.memory_service,
            trace_store=self.trace_store,
            feedback_evaluator=self.feedback_evaluator,
            guardrail_service=self.guardrail_service,
        )
        self.task_service = AgentTaskService(self.agent_service)
        self.eval_service = AgentEvalService(self.agent_service, self.trace_store)
        self.handoff_service = AgentHandoffService(self.agent_service)
        self.interop_service = AgentInteropService(self.skill_registry, self.handoff_service)

    @classmethod
    def get_instance(cls) -> "HarnessContainer":
        if cls._instance is None:
            cls._instance = HarnessContainer()
        return cls._instance

    async def startup(self) -> None:
        await self.task_service.startup()

    async def shutdown(self) -> None:
        await self.task_service.shutdown()
        await self.llm_registry.close()
