"""
LLM 包初始化
"""
from .base import BaseLLM, LLMMessage, LLMResponse, LLMStreamChunk, LLMUsage


def __getattr__(name: str):
    if name == "OllamaLLM":
        from .ollama_llm import OllamaLLM

        return OllamaLLM
    if name == "OpenAILLM":
        from .openai_llm import OpenAILLM

        return OpenAILLM
    if name == "ClaudeLLM":
        from .claude_llm import ClaudeLLM

        return ClaudeLLM
    if name == "KimiLLM":
        from .kimi_llm import KimiLLM

        return KimiLLM
    if name in {"LLMManager", "AllBackendsFailedError", "RateLimitError"}:
        from .manager import AllBackendsFailedError, LLMManager, RateLimitError

        return {
            "LLMManager": LLMManager,
            "AllBackendsFailedError": AllBackendsFailedError,
            "RateLimitError": RateLimitError,
        }[name]
    raise AttributeError(f"module 'llm' has no attribute {name!r}")

__all__ = [
    "BaseLLM", "LLMMessage", "LLMResponse", "LLMStreamChunk", "LLMUsage",
    "OllamaLLM", "OpenAILLM", "ClaudeLLM", "KimiLLM",
    "LLMManager", "AllBackendsFailedError", "RateLimitError",
]
