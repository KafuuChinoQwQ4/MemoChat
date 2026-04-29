from __future__ import annotations


def __getattr__(name: str):
    if name == "AgentTraceStore":
        from .trace_store import AgentTraceStore

        return AgentTraceStore
    if name == "FeedbackEvaluator":
        from .evaluator import FeedbackEvaluator

        return FeedbackEvaluator
    raise AttributeError(f"module 'harness.feedback' has no attribute {name!r}")

__all__ = ["AgentTraceStore", "FeedbackEvaluator"]
