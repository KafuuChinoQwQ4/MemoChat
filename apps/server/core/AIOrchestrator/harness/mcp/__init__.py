from __future__ import annotations


def __getattr__(name: str):
    if name == "MCPToolService":
        from .service import MCPToolService

        return MCPToolService
    raise AttributeError(f"module 'harness.mcp' has no attribute {name!r}")

__all__ = ["MCPToolService"]
