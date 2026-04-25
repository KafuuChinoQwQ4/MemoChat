from __future__ import annotations

from harness.execution.tool_executor import ToolExecutor
from harness.feedback.evaluator import FeedbackEvaluator
from harness.feedback.trace_store import AgentTraceStore
from harness.knowledge.service import KnowledgeService
from harness.llm.service import LLMEndpointRegistry
from harness.mcp.service import MCPToolService
from harness.memory.graph_memory import GraphMemoryService
from harness.memory.service import MemoryService
from harness.orchestration.agent_service import AgentHarnessService
from harness.orchestration.planner import PlanningPolicy
from harness.skills.registry import SkillRegistry


class HarnessContainer:
    _instance: "HarnessContainer | None" = None

    def __init__(self):
        self.skill_registry = SkillRegistry()
        self.trace_store = AgentTraceStore()
        self.feedback_evaluator = FeedbackEvaluator()
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
        )

    @classmethod
    def get_instance(cls) -> "HarnessContainer":
        if cls._instance is None:
            cls._instance = HarnessContainer()
        return cls._instance

    async def startup(self) -> None:
        return None

    async def shutdown(self) -> None:
        await self.llm_registry.close()
