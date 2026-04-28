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


class TraceEventModel(BaseModel):
    layer: str
    name: str
    status: str
    summary: str = ""
    detail: str = ""
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


class ToolInfo(BaseModel):
    name: str
    description: str
    source: str = "builtin"


class ToolListRsp(BaseModel):
    code: int = 0
    tools: list[ToolInfo] = Field(default_factory=list)
