from __future__ import annotations


def __getattr__(name: str):
    if name == "AgentHandoffService":
        from .service import AgentHandoffService

        return AgentHandoffService
    if name == "HandoffFlowGraph":
        from .flow_graph import HandoffFlowGraph

        return HandoffFlowGraph
    raise AttributeError(f"module 'harness.handoffs' has no attribute {name!r}")


__all__ = ["AgentHandoffService", "HandoffFlowGraph"]
