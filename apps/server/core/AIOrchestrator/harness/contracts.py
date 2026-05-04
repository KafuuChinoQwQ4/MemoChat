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
    metadata: dict[str, Any] = field(default_factory=dict)


@dataclass
class ModelPolicy:
    provider: str = ""
    model: str = ""
    deployment_preference: str = "any"
    max_tokens: int = 2048
    temperature: float = 0.7
    think: bool = False

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class ToolPolicy:
    default_tools: list[str] = field(default_factory=list)
    allowed_tools: list[str] = field(default_factory=list)
    default_actions: list[str] = field(default_factory=list)
    permission_level: str = "read"
    requires_confirmation_for_write: bool = True

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class KnowledgePolicy:
    enabled: bool = False
    top_k: int = 5
    cite_sources: bool = True

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class MemoryPolicy:
    include_history: bool = True
    include_semantic_profile: bool = True
    include_graph: bool = False
    write_back: bool = True

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class AgentSpec:
    name: str
    display_name: str
    description: str
    system_prompt: str
    default_model: str = ""
    model_policy: dict[str, Any] = field(default_factory=dict)
    default_tools: list[str] = field(default_factory=list)
    allowed_tools: list[str] = field(default_factory=list)
    tool_policy: dict[str, Any] = field(default_factory=dict)
    knowledge_policy: dict[str, Any] = field(default_factory=dict)
    memory_policy: dict[str, Any] = field(default_factory=dict)
    guardrail_policy: dict[str, Any] = field(default_factory=dict)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class ToolSpec:
    name: str
    display_name: str
    description: str
    source: str = "builtin"
    category: str = ""
    parameters_schema: dict[str, Any] = field(default_factory=dict)
    timeout_seconds: int = 30
    permission: str = "read"
    requires_confirmation: bool = False
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class GuardrailResult:
    name: str
    status: str
    message: str = ""
    detail: str = ""
    blocking: bool = False
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


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
class RunNode:
    node_id: str
    name: str
    layer: str
    status: str
    summary: str = ""
    detail: str = ""
    parent_id: str = ""
    started_at: int = 0
    finished_at: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def duration_ms(self) -> int:
        if self.finished_at <= 0 or self.started_at <= 0:
            return 0
        return max(self.finished_at - self.started_at, 0)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class RunEdge:
    source_id: str
    target_id: str
    relation: str = "next"
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class RunGraph:
    trace_id: str
    nodes: list[RunNode] = field(default_factory=list)
    edges: list[RunEdge] = field(default_factory=list)
    status: str = "running"
    started_at: int = 0
    finished_at: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return {
            "trace_id": self.trace_id,
            "nodes": [node.to_dict() for node in self.nodes],
            "edges": [edge.to_dict() for edge in self.edges],
            "status": self.status,
            "started_at": self.started_at,
            "finished_at": self.finished_at,
            "metadata": dict(self.metadata),
        }


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
    run_graph: RunGraph | None = None

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
            "run_graph": self.run_graph.to_dict() if self.run_graph is not None else None,
        }


@dataclass
class ContextSection:
    name: str
    source: str
    content: str
    token_count: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class ContextPack:
    sections: list[ContextSection] = field(default_factory=list)
    token_budget: int = 0
    source_metadata: dict[str, Any] = field(default_factory=dict)
    system_messages: list[LLMMessage] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return {
            "sections": [section.to_dict() for section in self.sections],
            "token_budget": self.token_budget,
            "source_metadata": dict(self.source_metadata),
            "system_messages": [asdict(message) for message in self.system_messages],
            "metadata": dict(self.metadata),
        }


@dataclass
class AgentTask:
    task_id: str
    title: str
    status: str = "queued"
    trace_id: str = ""
    description: str = ""
    priority: int = 0
    payload: dict[str, Any] = field(default_factory=dict)
    result: dict[str, Any] = field(default_factory=dict)
    checkpoints: list[dict[str, Any]] = field(default_factory=list)
    error: str = ""
    created_at: int = 0
    updated_at: int = 0
    completed_at: int = 0
    cancelled_at: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class AgentMessage:
    message_id: str
    flow_id: str
    from_agent: str
    to_agent: str
    role: str
    content: str
    trace_id: str = ""
    created_at: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class HandoffStep:
    step_id: str
    agent_name: str
    instruction: str
    input_template: str = ""
    depends_on: list[str] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class AgentFlow:
    name: str
    display_name: str
    description: str
    steps: list[HandoffStep] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "display_name": self.display_name,
            "description": self.description,
            "steps": [step.to_dict() for step in self.steps],
            "metadata": dict(self.metadata),
        }


@dataclass
class AgentCapabilities:
    streaming: bool = False
    push_notifications: bool = False
    state_transition_history: bool = False
    extensions: list[dict[str, Any]] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return {
            "streaming": self.streaming,
            "pushNotifications": self.push_notifications,
            "stateTransitionHistory": self.state_transition_history,
            "extensions": list(self.extensions),
        }


@dataclass
class AgentCardSkill:
    id: str
    name: str
    description: str
    tags: list[str] = field(default_factory=list)
    examples: list[str] = field(default_factory=list)
    input_modes: list[str] = field(default_factory=list)
    output_modes: list[str] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        data = {
            "id": self.id,
            "name": self.name,
            "description": self.description,
            "tags": list(self.tags),
            "examples": list(self.examples),
            "metadata": dict(self.metadata),
        }
        if self.input_modes:
            data["inputModes"] = list(self.input_modes)
        if self.output_modes:
            data["outputModes"] = list(self.output_modes)
        return data


@dataclass
class AgentCard:
    name: str
    description: str
    url: str
    version: str
    protocol_version: str = "0.3.0"
    capabilities: AgentCapabilities = field(default_factory=AgentCapabilities)
    skills: list[AgentCardSkill] = field(default_factory=list)
    provider: dict[str, Any] = field(default_factory=dict)
    icon_url: str = ""
    documentation_url: str = ""
    preferred_transport: str = "JSONRPC"
    additional_interfaces: list[dict[str, Any]] = field(default_factory=list)
    default_input_modes: list[str] = field(default_factory=lambda: ["text/plain"])
    default_output_modes: list[str] = field(default_factory=lambda: ["text/plain"])
    security_schemes: dict[str, Any] = field(default_factory=dict)
    security: list[dict[str, list[str]]] = field(default_factory=list)
    supports_authenticated_extended_card: bool = False
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        data: dict[str, Any] = {
            "name": self.name,
            "description": self.description,
            "url": self.url,
            "provider": dict(self.provider),
            "version": self.version,
            "protocolVersion": self.protocol_version,
            "capabilities": self.capabilities.to_dict(),
            "defaultInputModes": list(self.default_input_modes),
            "defaultOutputModes": list(self.default_output_modes),
            "skills": [skill.to_dict() for skill in self.skills],
            "supportsAuthenticatedExtendedCard": self.supports_authenticated_extended_card,
            "metadata": dict(self.metadata),
        }
        if self.icon_url:
            data["iconUrl"] = self.icon_url
        if self.documentation_url:
            data["documentationUrl"] = self.documentation_url
        if self.preferred_transport:
            data["preferredTransport"] = self.preferred_transport
        if self.additional_interfaces:
            data["additionalInterfaces"] = list(self.additional_interfaces)
        if self.security_schemes:
            data["securitySchemes"] = dict(self.security_schemes)
        if self.security:
            data["security"] = list(self.security)
        return data


@dataclass
class RemoteAgentRef:
    peer_id: str
    name: str
    card_url: str
    base_url: str = ""
    status: str = "registered"
    trusted: bool = False
    card: dict[str, Any] = field(default_factory=dict)
    registered_at: int = 0
    updated_at: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class RemoteAgentTask:
    task_id: str
    peer_id: str
    status: str = "queued"
    input: dict[str, Any] = field(default_factory=dict)
    result: dict[str, Any] = field(default_factory=dict)
    error: str = ""
    created_at: int = 0
    updated_at: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


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
    context_pack: ContextPack | None = None

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
