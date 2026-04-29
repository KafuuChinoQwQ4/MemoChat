from __future__ import annotations

from typing import Any, AsyncIterator, Protocol

from harness.contracts import AgentSkill, AgentTrace, HarnessRunResult, MemorySnapshot, PlanStep, ToolObservation, TraceEvent
from llm.base import LLMMessage, LLMResponse, LLMStreamChunk


class SkillPlanningPort(Protocol):
    def resolve_skill(self, request: Any) -> AgentSkill: ...
    def build_plan(self, request: Any, skill: AgentSkill) -> list[PlanStep]: ...
    def build_system_prompt(
        self,
        skill: AgentSkill,
        memory_snapshot: MemorySnapshot,
        observations: list[ToolObservation],
    ) -> str: ...
    def build_user_prompt(self, request: Any, observations: list[ToolObservation]) -> str: ...


class LLMCompletionPort(Protocol):
    async def complete(
        self,
        messages: list[LLMMessage],
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
        **kwargs: Any,
    ) -> LLMResponse: ...

    def stream(
        self,
        messages: list[LLMMessage],
        prefer_backend: str = "",
        model_name: str = "",
        deployment_preference: str = "any",
        **kwargs: Any,
    ) -> AsyncIterator[LLMStreamChunk]: ...


class ToolExecutionPort(Protocol):
    def list_tools(self) -> list[dict[str, Any]]: ...

    async def execute(
        self,
        uid: int,
        content: str,
        plan_steps: list[PlanStep],
        target_lang: str = "",
        requested_tools: list[str] | None = None,
        tool_arguments: dict[str, dict] | None = None,
    ) -> list[ToolObservation]: ...


class MemoryPort(Protocol):
    async def load(self, uid: int, session_id: str, include_graph: bool = False) -> MemorySnapshot: ...
    async def save_after_response(self, uid: int, session_id: str, user_message: str, ai_message: str) -> None: ...


class TraceStorePort(Protocol):
    async def start_run(
        self,
        uid: int,
        session_id: str,
        skill_name: str,
        request_summary: str,
        plan_steps: list[PlanStep],
    ) -> AgentTrace: ...

    async def add_event(self, trace_id: str, event: TraceEvent) -> None: ...
    async def finish_run(
        self,
        trace_id: str,
        status: str,
        response_content: str,
        model: str,
        feedback_summary: str,
        observations: list[str],
    ) -> None: ...
    def get_trace(self, trace_id: str) -> AgentTrace | None: ...
    async def get_trace_or_load(self, trace_id: str) -> AgentTrace | None: ...


class FeedbackPort(Protocol):
    def build_summary(
        self,
        skill_name: str,
        observations: list[ToolObservation],
        response_text: str,
    ) -> str: ...


__all__ = [
    "FeedbackPort",
    "LLMCompletionPort",
    "MemoryPort",
    "SkillPlanningPort",
    "ToolExecutionPort",
    "TraceStorePort",
]
