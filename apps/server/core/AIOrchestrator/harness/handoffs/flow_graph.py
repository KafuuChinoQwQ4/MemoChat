from __future__ import annotations

from typing import Any, Awaitable, Callable

from langgraph.graph import END, StateGraph

HandoffGraphState = dict[str, Any]
HandoffGraphNode = Callable[[HandoffGraphState], Awaitable[HandoffGraphState]]


class HandoffFlowGraph:
    def __init__(self, nodes: Any):
        self._nodes = nodes
        self._compiled = self._build()

    @property
    def available(self) -> bool:
        return True

    @property
    def backend(self) -> str:
        return "langgraph"

    async def run(self, initial_state: HandoffGraphState) -> HandoffGraphState:
        if initial_state.get("done", False):
            return initial_state
        return await self._compiled.ainvoke(initial_state)

    def metadata(self, state: HandoffGraphState | None = None) -> dict[str, Any]:
        state = state or {}
        return {
            "name": "handoff_flow",
            "backend": self.backend,
            "available": self.available,
            "nodes": list(state.get("graph_steps", [])),
            "error": str(state.get("error", "") or ""),
        }

    def _build(self):
        graph = StateGraph(dict)
        graph.add_node("run_step", self._node("run_step", self._nodes.graph_run_step))
        graph.set_entry_point("run_step")
        graph.add_conditional_edges(
            "run_step",
            self._route_next_step,
            {"continue": "run_step", "done": END},
        )
        return graph.compile()

    def _node(self, name: str, handler: HandoffGraphNode) -> HandoffGraphNode:
        async def _run(state: HandoffGraphState) -> HandoffGraphState:
            next_state = await handler(dict(state))
            return {
                **next_state,
                "graph_steps": [*next_state.get("graph_steps", []), name],
            }

        return _run

    def _route_next_step(self, state: HandoffGraphState) -> str:
        return "done" if state.get("done", False) else "continue"


__all__ = ["HandoffFlowGraph", "HandoffGraphState"]
