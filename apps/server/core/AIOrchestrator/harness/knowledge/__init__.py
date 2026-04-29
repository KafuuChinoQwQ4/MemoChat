from __future__ import annotations


def __getattr__(name: str):
    if name == "KnowledgeService":
        from .service import KnowledgeService

        return KnowledgeService
    raise AttributeError(f"module 'harness.knowledge' has no attribute {name!r}")

__all__ = ["KnowledgeService"]
