from __future__ import annotations


def __getattr__(name: str):
    if name == "GraphMemoryService":
        from .graph_memory import GraphMemoryService

        return GraphMemoryService
    if name == "MemoryService":
        from .service import MemoryService

        return MemoryService
    raise AttributeError(f"module 'harness.memory' has no attribute {name!r}")

__all__ = ["MemoryService", "GraphMemoryService"]
