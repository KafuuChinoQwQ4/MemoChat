from __future__ import annotations


def __getattr__(name: str):
    if name == "ToolExecutor":
        from .tool_executor import ToolExecutor

        return ToolExecutor
    raise AttributeError(f"module 'harness.execution' has no attribute {name!r}")

__all__ = ["ToolExecutor"]
