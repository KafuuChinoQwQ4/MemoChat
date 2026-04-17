"""
Pydantic 请求/响应模型定义
"""
from pydantic import BaseModel, Field
from typing import Optional


class ChatReq(BaseModel):
    uid: int
    session_id: str = ""
    content: str
    model_type: str = "ollama"
    model_name: str = ""
    stream: bool = False


class ChatRsp(BaseModel):
    code: int = 0
    message: str = "ok"
    session_id: str
    content: str = ""
    tokens: int = 0
    model: str = ""


class ChatChunk(BaseModel):
    chunk: str = ""
    is_final: bool = False
    msg_id: str = ""
    total_tokens: int = 0


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


class KbSearchRsp(BaseModel):
    code: int = 0
    chunks: list[KbChunk] = []


class KbListRsp(BaseModel):
    code: int = 0
    knowledge_bases: list[dict] = []


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


class ListModelsRsp(BaseModel):
    code: int = 0
    models: list[ModelInfo] = []
    default_model: Optional[ModelInfo] = None
