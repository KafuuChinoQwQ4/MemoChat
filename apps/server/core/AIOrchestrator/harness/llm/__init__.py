from __future__ import annotations


def __getattr__(name: str):
    if name == "LLMEndpointRegistry":
        from .service import LLMEndpointRegistry

        return LLMEndpointRegistry
    raise AttributeError(f"module 'harness.llm' has no attribute {name!r}")

__all__ = ["LLMEndpointRegistry"]
