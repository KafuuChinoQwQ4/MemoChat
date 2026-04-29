from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any

from llm.base import LLMMessage


@dataclass
class AgentSkill:
    name: str
    display_name: str
    description: str
    system_prompt: str
    default_actions: list[str] = field(default_factory=list)
    allow_web: bool = False
    allow_knowledge: bool = False
    allow_graph: bool = False
    allow_mcp: bool = False


@dataclass
class PlanStep:
    action: str
    reason: str
    parameters: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class ToolObservation:
    name: str
    source: str
    output: str
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_summary(self) -> str:
        preview = self.output.strip().replace("\n", " ")
        return f"[{self.name}] {preview[:280]}"


@dataclass
class TraceEvent:
    layer: str
    name: str
    status: str
    summary: str = ""
    detail: str = ""
    started_at: int = 0
    finished_at: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class AgentTrace:
    trace_id: str
    uid: int
    session_id: str
    skill: str
    request_summary: str
    plan_steps: list[PlanStep] = field(default_factory=list)
    events: list[TraceEvent] = field(default_factory=list)
    observations: list[str] = field(default_factory=list)
    status: str = "running"
    response_content: str = ""
    model: str = ""
    feedback_summary: str = ""
    started_at: int = 0
    finished_at: int = 0

    def to_dict(self) -> dict[str, Any]:
        return {
            "trace_id": self.trace_id,
            "uid": self.uid,
            "session_id": self.session_id,
            "skill": self.skill,
            "request_summary": self.request_summary,
            "plan_steps": [step.to_dict() for step in self.plan_steps],
            "events": [event.to_dict() for event in self.events],
            "observations": list(self.observations),
            "status": self.status,
            "response_content": self.response_content,
            "model": self.model,
            "feedback_summary": self.feedback_summary,
            "started_at": self.started_at,
            "finished_at": self.finished_at,
        }


@dataclass
class ProviderEndpoint:
    provider_id: str
    adapter: str
    deployment: str
    base_url: str
    default_model: str
    enabled: bool
    thinking_parameter: str = ""
    models: list[dict[str, Any]] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class MemorySnapshot:
    system_messages: list[LLMMessage] = field(default_factory=list)
    chat_history: list[LLMMessage] = field(default_factory=list)
    episodic_summaries: list[str] = field(default_factory=list)
    semantic_profile: dict[str, Any] = field(default_factory=dict)
    graph_context: list[dict[str, Any]] = field(default_factory=list)

    def as_messages(self) -> list[LLMMessage]:
        return [*self.system_messages, *self.chat_history]


@dataclass
class HarnessRunResult:
    session_id: str
    content: str
    tokens: int
    model: str
    trace_id: str
    skill: str
    feedback_summary: str
    observations: list[str] = field(default_factory=list)
    events: list[TraceEvent] = field(default_factory=list)
