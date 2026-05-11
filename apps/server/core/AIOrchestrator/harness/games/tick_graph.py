from __future__ import annotations

from typing import Any, Callable

from harness.games.contracts import GameState
from harness.games.rules import GameRuleEngine

try:
    from langgraph.graph import END, StateGraph
except Exception:
    END = None
    StateGraph = None


StateResolver = Callable[[str], GameState]
EngineResolver = Callable[[str], GameRuleEngine]


class GameTickGraph:
    def __init__(self, state_resolver: StateResolver, engine_resolver: EngineResolver):
        self._state_resolver = state_resolver
        self._engine_resolver = engine_resolver
        self._compiled = self._build()

    @property
    def available(self) -> bool:
        return self._compiled is not None

    async def run(self, context: dict[str, Any]) -> dict[str, Any]:
        if self._compiled is not None:
            try:
                return await self._compiled.ainvoke(context)
            except Exception as exc:
                context["graph_error"] = f"{type(exc).__name__}: {exc}"
        return await self._run_fallback(context)

    def metadata(self, context: dict[str, Any]) -> dict[str, Any]:
        backend = "langgraph" if self.available and not context.get("graph_error") else "fallback"
        return {
            "name": "a2a_game_tick",
            "backend": backend,
            "available": self.available,
            "nodes": list(context.get("graph_steps", [])),
            "actor_id": context.get("actor_id", ""),
            "error": context.get("graph_error", ""),
        }

    def _build(self):
        if StateGraph is None or END is None:
            return None
        graph = StateGraph(dict)
        graph.add_node("load_state", self._load_state)
        graph.add_node("select_actor", self._select_actor)
        graph.add_edge("load_state", "select_actor")
        graph.add_edge("select_actor", END)
        graph.set_entry_point("load_state")
        return graph.compile()

    async def _run_fallback(self, context: dict[str, Any]) -> dict[str, Any]:
        context = await self._load_state(context)
        return await self._select_actor(context)

    async def _load_state(self, context: dict[str, Any]) -> dict[str, Any]:
        state = self._state_resolver(context["room_id"])
        return {
            **context,
            "state": state,
            "engine": self._engine_resolver(state.room.ruleset_id),
            "graph_steps": [*context.get("graph_steps", []), "load_state"],
        }

    async def _select_actor(self, context: dict[str, Any]) -> dict[str, Any]:
        engine = context["engine"]
        actor_id = engine.next_pending_actor(context["state"])
        return {**context, "actor_id": actor_id, "graph_steps": [*context.get("graph_steps", []), "select_actor"]}
