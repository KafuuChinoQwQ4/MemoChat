"""
LLM 抽象基类 — 所有 LLM 实现统一接口
"""
import json
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Any, AsyncIterator, Optional


@dataclass
class LLMMessage:
    role: str
    content: str
    metadata: dict = field(default_factory=dict)


@dataclass
class LLMUsage:
    prompt_tokens: int = 0
    completion_tokens: int = 0
    total_tokens: int = 0


@dataclass
class LLMResponse:
    content: str
    reasoning_content: str = ""
    usage: LLMUsage = field(default_factory=LLMUsage)
    model: str = ""
    finish_reason: str = ""
    metadata: dict = field(default_factory=dict)


@dataclass
class LLMStreamChunk:
    content: str
    reasoning_content: str = ""
    is_final: bool = False
    usage: Optional[LLMUsage] = None
    model: str = ""


class BaseLLM(ABC):
    """LLM 抽象基类"""

    def __init__(self, model_name: str, **kwargs):
        self.model_name = model_name
        self.kwargs = kwargs

    @abstractmethod
    async def chat(self, messages: list[LLMMessage], **kwargs) -> LLMResponse:
        """同步调用，返回完整响应"""
        pass

    @abstractmethod
    async def chat_stream(self, messages: list[LLMMessage], **kwargs) -> AsyncIterator[LLMStreamChunk]:
        """流式调用，yield 增量片段"""
        pass

    @abstractmethod
    async def list_models(self) -> list[str]:
        """列出可用模型"""
        pass

    def count_tokens(self, text: str) -> int:
        """粗略估算 token 数（按字符/4 估算）"""
        return len(text) // 4

    def _messages_to_dict(self, messages: list[LLMMessage]) -> list[dict]:
        """将 LLMMessage 列表转为 provider 原生格式"""
        result = []
        for m in messages:
            item = {"role": m.role, "content": m.content}
            item.update(m.metadata)
            result.append(item)
        return result
