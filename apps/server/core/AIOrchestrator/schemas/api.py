"""
Pydantic 请求/响应模型定义
"""
from typing import Any, Optional

from pydantic import BaseModel, Field


class ChatReq(BaseModel):
    uid: int
    session_id: str = ""
    content: str
    model_type: str = ""
    model_name: str = ""
    stream: bool = False
    deployment_preference: str = "any"
    skill_name: str = ""
    requested_tools: list[str] = Field(default_factory=list)
    tool_arguments: dict[str, dict[str, Any]] = Field(default_factory=dict)
    metadata: dict[str, Any] = Field(default_factory=dict)


class ChatRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    session_id: str
    content: str = ""
    tokens: int = 0
    model: str = ""
    trace_id: str = ""
    skill: str = ""
    feedback_summary: str = ""
    observations: list[str] = Field(default_factory=list)
    events: list["TraceEventModel"] = Field(default_factory=list)


class ChatChunk(BaseModel):
    chunk: str = ""
    is_final: bool = False
    msg_id: str = ""
    total_tokens: int = 0
    trace_id: str = ""
    skill: str = ""


class SmartReq(BaseModel):
    uid: int
    feature_type: str = Field(..., description="summary | suggest | translate")
    content: str
    target_lang: str = ""
    context_json: str = ""
    model_type: str = ""
    model_name: str = ""
    deployment_preference: str = "any"


class SmartRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    result: str = ""
    content: str = ""
    trace_id: str = ""
    skill: str = ""


class KbUploadReq(BaseModel):
    uid: int
    file_name: str
    file_type: str = Field(..., description="pdf | txt | md | docx")
    content: str = Field(..., description="Base64 编码的文件内容")


class KbUploadRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    chunks: int = 0
    kb_id: str = ""


class KbSearchReq(BaseModel):
    uid: int
    query: str
    top_k: int = 5
    metadata_filters: dict[str, Any] = Field(default_factory=dict)


class KbChunk(BaseModel):
    content: str
    score: float
    source: str = ""
    kb_id: str = ""


class KbSearchRsp(BaseModel):
    code: int = 0
    chunks: list[KbChunk] = Field(default_factory=list)


class KbListRsp(BaseModel):
    code: int = 0
    knowledge_bases: list[dict[str, Any]] = Field(default_factory=list)


class KbDeleteReq(BaseModel):
    uid: int
    kb_id: str


class KbDeleteRsp(BaseModel):
    code: int = 0
    message: str = "ok"


class ModelInfo(BaseModel):
    model_type: str
    model_name: str
    display_name: str
    is_enabled: bool
    context_window: int
    supports_thinking: bool = False
    provider_id: str = ""
    adapter: str = ""
    deployment: str = ""


class ProviderInfo(BaseModel):
    provider_id: str
    adapter: str
    deployment: str
    base_url: str = ""
    default_model: str = ""
    enabled: bool = True


class ListModelsRsp(BaseModel):
    code: int = 0
    models: list[ModelInfo] = Field(default_factory=list)
    default_model: Optional[ModelInfo] = None
    providers: list[ProviderInfo] = Field(default_factory=list)


class RegisterApiProviderReq(BaseModel):
    provider_name: str = Field(default="custom-api")
    base_url: str
    api_key: str
    adapter: str = Field(default="openai_compatible")


class RegisterApiProviderRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    provider_id: str = ""
    models: list[ModelInfo] = Field(default_factory=list)


class DeleteApiProviderReq(BaseModel):
    provider_id: str


class DeleteApiProviderRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    provider_id: str = ""


class TraceEventModel(BaseModel):
    layer: str
    name: str
    status: str
    summary: str = ""
    detail: str = ""
    started_at: int = 0
    finished_at: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class RunNodeModel(BaseModel):
    node_id: str
    name: str
    layer: str
    status: str
    summary: str = ""
    detail: str = ""
    parent_id: str = ""
    started_at: int = 0
    finished_at: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class RunEdgeModel(BaseModel):
    source_id: str
    target_id: str
    relation: str = "next"
    metadata: dict[str, Any] = Field(default_factory=dict)


class RunGraphModel(BaseModel):
    trace_id: str
    nodes: list[RunNodeModel] = Field(default_factory=list)
    edges: list[RunEdgeModel] = Field(default_factory=list)
    status: str = "running"
    started_at: int = 0
    finished_at: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentRunReq(BaseModel):
    uid: int
    session_id: str = ""
    content: str
    model_type: str = ""
    model_name: str = ""
    deployment_preference: str = "any"
    skill_name: str = ""
    feature_type: str = ""
    target_lang: str = ""
    requested_tools: list[str] = Field(default_factory=list)
    tool_arguments: dict[str, dict[str, Any]] = Field(default_factory=dict)
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentRunRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    session_id: str
    content: str = ""
    tokens: int = 0
    model: str = ""
    trace_id: str = ""
    skill: str = ""
    feedback_summary: str = ""
    observations: list[str] = Field(default_factory=list)
    events: list[TraceEventModel] = Field(default_factory=list)


class AgentTraceRsp(BaseModel):
    code: int = 0
    trace_id: str
    uid: int
    session_id: str = ""
    skill: str = ""
    status: str = ""
    request_summary: str = ""
    response_content: str = ""
    model: str = ""
    feedback_summary: str = ""
    observations: list[str] = Field(default_factory=list)
    events: list[TraceEventModel] = Field(default_factory=list)


class AgentRunGraphRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    trace_id: str
    graph: RunGraphModel


class ContextSectionModel(BaseModel):
    name: str
    source: str
    content: str
    token_count: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class ContextPackModel(BaseModel):
    sections: list[ContextSectionModel] = Field(default_factory=list)
    token_budget: int = 0
    source_metadata: dict[str, Any] = Field(default_factory=dict)
    metadata: dict[str, Any] = Field(default_factory=dict)


class MemoryItemModel(BaseModel):
    memory_id: str
    type: str
    source: str = ""
    content: str
    created_at: int = 0
    updated_at: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class MemoryListRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    memories: list[MemoryItemModel] = Field(default_factory=list)


class MemoryCreateReq(BaseModel):
    uid: int
    content: str


class MemoryDeleteReq(BaseModel):
    uid: int
    memory_id: str


class MemoryMutationRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    memory: Optional[MemoryItemModel] = None


class AgentTaskModel(BaseModel):
    task_id: str
    title: str
    status: str = "queued"
    trace_id: str = ""
    description: str = ""
    priority: int = 0
    payload: dict[str, Any] = Field(default_factory=dict)
    result: dict[str, Any] = Field(default_factory=dict)
    checkpoints: list[dict[str, Any]] = Field(default_factory=list)
    error: str = ""
    created_at: int = 0
    updated_at: int = 0
    completed_at: int = 0
    cancelled_at: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentTaskCreateReq(BaseModel):
    uid: int
    title: str = ""
    content: str
    session_id: str = ""
    model_type: str = ""
    model_name: str = ""
    skill_name: str = ""
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentTaskListRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    tasks: list[AgentTaskModel] = Field(default_factory=list)


class AgentTaskRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    task: Optional[AgentTaskModel] = None


class AgentEvalCaseModel(BaseModel):
    case_id: str
    name: str
    description: str = ""
    request: dict[str, Any] = Field(default_factory=dict)
    expectations: dict[str, Any] = Field(default_factory=dict)
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentEvalListRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    evals: list[AgentEvalCaseModel] = Field(default_factory=list)


class AgentEvalRunReq(BaseModel):
    case_id: str = ""
    trace_id: str = ""
    uid: int = 0
    run_all: bool = False


class AgentEvalResultModel(BaseModel):
    case_id: str
    passed: bool = False
    trace_id: str = ""
    failures: list[str] = Field(default_factory=list)
    metrics: dict[str, Any] = Field(default_factory=dict)
    expected: dict[str, Any] = Field(default_factory=dict)
    observed: dict[str, Any] = Field(default_factory=dict)


class AgentEvalRunRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    passed: bool = False
    results: list[AgentEvalResultModel] = Field(default_factory=list)


class FeedbackReq(BaseModel):
    trace_id: str
    uid: int
    rating: int = Field(..., ge=1, le=5)
    feedback_type: str = "user_rating"
    comment: str = ""
    payload: dict[str, Any] = Field(default_factory=dict)


class FeedbackRsp(BaseModel):
    code: int = 0
    message: str = "ok"


class SkillInfo(BaseModel):
    name: str
    display_name: str
    description: str
    allow_web: bool = False
    allow_knowledge: bool = False
    allow_graph: bool = False
    allow_mcp: bool = False


class SkillListRsp(BaseModel):
    code: int = 0
    skills: list[SkillInfo] = Field(default_factory=list)


class AgentSpecModel(BaseModel):
    name: str
    display_name: str
    description: str
    system_prompt: str
    default_model: str = ""
    model_policy: dict[str, Any] = Field(default_factory=dict)
    default_tools: list[str] = Field(default_factory=list)
    allowed_tools: list[str] = Field(default_factory=list)
    tool_policy: dict[str, Any] = Field(default_factory=dict)
    knowledge_policy: dict[str, Any] = Field(default_factory=dict)
    memory_policy: dict[str, Any] = Field(default_factory=dict)
    guardrail_policy: dict[str, Any] = Field(default_factory=dict)
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentSpecListRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    specs: list[AgentSpecModel] = Field(default_factory=list)


class AgentSpecRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    spec: Optional[AgentSpecModel] = None


class AgentMessageModel(BaseModel):
    message_id: str
    flow_id: str
    from_agent: str
    to_agent: str
    role: str
    content: str
    trace_id: str = ""
    created_at: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class HandoffStepModel(BaseModel):
    step_id: str
    agent_name: str
    instruction: str
    input_template: str = ""
    depends_on: list[str] = Field(default_factory=list)
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentFlowModel(BaseModel):
    name: str
    display_name: str
    description: str
    steps: list[HandoffStepModel] = Field(default_factory=list)
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentFlowListRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    flows: list[AgentFlowModel] = Field(default_factory=list)


class AgentFlowRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    flow: Optional[AgentFlowModel] = None


class AgentFlowRunReq(BaseModel):
    flow_name: str
    uid: int
    content: str
    session_id: str = ""
    model_type: str = ""
    model_name: str = ""
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentFlowRunRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    flow_id: str = ""
    status: str = ""
    flow: Optional[AgentFlowModel] = None
    messages: list[AgentMessageModel] = Field(default_factory=list)
    graph: Optional[RunGraphModel] = None


class GameAgentConfigModel(BaseModel):
    display_name: str = ""
    model_type: str = ""
    model_name: str = ""
    persona: str = ""
    skill_name: str = ""
    strategy: str = "balanced"
    role_key: str = ""
    metadata: dict[str, Any] = Field(default_factory=dict)


class GameRoomCreateReq(BaseModel):
    uid: int
    title: str = ""
    ruleset_id: str = "werewolf.basic"
    max_players: int = Field(default=12, ge=1, le=64)
    agent_count: int = Field(default=0, ge=0, le=63)
    agents: list[GameAgentConfigModel] = Field(default_factory=list)
    config: dict[str, Any] = Field(default_factory=dict)


class GameRoomJoinReq(BaseModel):
    uid: int
    display_name: str = ""


class GameRoomAutoTickReq(BaseModel):
    uid: int
    max_steps: int = Field(default=8, ge=1, le=32)


class GameAgentAddReq(BaseModel):
    agent_count: int = Field(default=1, ge=0, le=64)
    agents: list[GameAgentConfigModel] = Field(default_factory=list)


class GameTemplateSaveReq(BaseModel):
    template_id: str = ""
    uid: int
    title: str
    description: str = ""
    ruleset_id: str = "werewolf.basic"
    max_players: int = Field(default=12, ge=1, le=64)
    agents: list[GameAgentConfigModel] = Field(default_factory=list)
    config: dict[str, Any] = Field(default_factory=dict)
    metadata: dict[str, Any] = Field(default_factory=dict)


class GameRoomFromTemplateReq(BaseModel):
    uid: int
    title: str = ""


class GameTemplatePresetCloneReq(BaseModel):
    uid: int
    title: str = ""


class GameActionReq(BaseModel):
    uid: int = 0
    participant_id: str
    action_type: str
    content: str = ""
    target_participant_id: str = ""
    payload: dict[str, Any] = Field(default_factory=dict)


class GameRoomRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    room: Optional[dict[str, Any]] = None
    rooms: list[dict[str, Any]] = Field(default_factory=list)
    template: Optional[dict[str, Any]] = None
    templates: list[dict[str, Any]] = Field(default_factory=list)
    template_presets: list[dict[str, Any]] = Field(default_factory=list)
    state: Optional[dict[str, Any]] = None
    rulesets: list[dict[str, Any]] = Field(default_factory=list)
    role_presets: list[dict[str, Any]] = Field(default_factory=list)
    tick: Optional[dict[str, Any]] = None


class AgentCapabilitiesModel(BaseModel):
    streaming: bool = False
    pushNotifications: bool = False
    stateTransitionHistory: bool = False
    extensions: list[dict[str, Any]] = Field(default_factory=list)


class AgentCardSkillModel(BaseModel):
    id: str
    name: str
    description: str = ""
    tags: list[str] = Field(default_factory=list)
    examples: list[str] = Field(default_factory=list)
    inputModes: list[str] = Field(default_factory=list)
    outputModes: list[str] = Field(default_factory=list)
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentCardModel(BaseModel):
    name: str
    description: str = ""
    url: str
    provider: dict[str, Any] = Field(default_factory=dict)
    version: str = ""
    protocolVersion: str = ""
    capabilities: AgentCapabilitiesModel = Field(default_factory=AgentCapabilitiesModel)
    preferredTransport: str = ""
    additionalInterfaces: list[dict[str, Any]] = Field(default_factory=list)
    defaultInputModes: list[str] = Field(default_factory=list)
    defaultOutputModes: list[str] = Field(default_factory=list)
    skills: list[AgentCardSkillModel] = Field(default_factory=list)
    supportsAuthenticatedExtendedCard: bool = False
    iconUrl: str = ""
    documentationUrl: str = ""
    securitySchemes: dict[str, Any] = Field(default_factory=dict)
    security: list[dict[str, list[str]]] = Field(default_factory=list)
    metadata: dict[str, Any] = Field(default_factory=dict)


class AgentCardRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    card: AgentCardModel


class RemoteAgentModel(BaseModel):
    peer_id: str
    name: str
    card_url: str
    base_url: str = ""
    status: str = "registered"
    trusted: bool = False
    card: dict[str, Any] = Field(default_factory=dict)
    registered_at: int = 0
    updated_at: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class RemoteAgentRegisterReq(BaseModel):
    peer_id: str = ""
    name: str = ""
    card_url: str = ""
    base_url: str = ""
    trusted: bool = False
    card: dict[str, Any] = Field(default_factory=dict)
    metadata: dict[str, Any] = Field(default_factory=dict)


class RemoteAgentListRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    agents: list[RemoteAgentModel] = Field(default_factory=list)


class RemoteAgentRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    agent: Optional[RemoteAgentModel] = None


class RemoteAgentTaskCreateReq(BaseModel):
    input: dict[str, Any] = Field(default_factory=dict)
    metadata: dict[str, Any] = Field(default_factory=dict)


class RemoteAgentTaskModel(BaseModel):
    task_id: str
    peer_id: str
    status: str = "queued"
    input: dict[str, Any] = Field(default_factory=dict)
    result: dict[str, Any] = Field(default_factory=dict)
    error: str = ""
    created_at: int = 0
    updated_at: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class RemoteAgentTaskRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    task: Optional[RemoteAgentTaskModel] = None


class ToolInfo(BaseModel):
    name: str
    display_name: str = ""
    description: str
    source: str = "builtin"
    category: str = ""
    parameters_schema: dict[str, Any] = Field(default_factory=dict)
    timeout_seconds: int = 30
    permission: str = "read"
    requires_confirmation: bool = False
    metadata: dict[str, Any] = Field(default_factory=dict)


class ToolListRsp(BaseModel):
    code: int = 0
    tools: list[ToolInfo] = Field(default_factory=list)


class AgentLayerInfo(BaseModel):
    name: str
    display_name: str
    path: str
    responsibilities: list[str] = Field(default_factory=list)
    primary_files: list[str] = Field(default_factory=list)


class AgentLayerListRsp(BaseModel):
    code: int = 0
    layers: list[AgentLayerInfo] = Field(default_factory=list)
