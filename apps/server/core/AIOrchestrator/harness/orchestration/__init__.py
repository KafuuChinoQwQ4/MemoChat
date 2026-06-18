from __future__ import annotations


def __getattr__(name: str):
    if name == "AgentHarnessService":
        from .agent_service import AgentHarnessService

        return AgentHarnessService
    if name == "AgentTurnGraph":
        from .agent_graph import AgentTurnGraph

        return AgentTurnGraph
    if name == "PlanningPolicy":
        from .planner import PlanningPolicy

        return PlanningPolicy
    raise AttributeError(f"module 'harness.orchestration' has no attribute {name!r}")


__all__ = ["AgentHarnessService", "AgentTurnGraph", "PlanningPolicy"]
