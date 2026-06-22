from __future__ import annotations

from typing import Any, Awaitable, Callable

from langgraph.graph import END, StateGraph

AgentGraphState = dict[str, Any]
AgentGraphNode = Callable[[AgentGraphState], Awaitable[AgentGraphState]]


class AgentTurnGraph:
    def __init__(self, nodes: Any):
        self._nodes = nodes
        self._prepare_graph = self._build_prepare_graph()
        self._complete_graph = self._build_complete_graph()
        self._finish_graph = self._build_finish_graph()

    @property
    def available(self) -> bool:
        return True

    @property
    def backend(self) -> str:
        return "langgraph"

    async def run_prepare(self, request: Any, route: str) -> AgentGraphState:
        return await self._prepare_graph.ainvoke({"request": request, "route": route, "graph_steps": []})

    async def run_complete(self, request: Any, prepared: Any) -> AgentGraphState:
        return await self._complete_graph.ainvoke(
            {
                "request": request,
                "prepared": prepared,
                "graph_steps": [],
            }
        )

    async def run_finish(
        self,
        request: Any,
        prepared: Any,
        response_content: str,
        response_tokens: int,
        model: str,
    ) -> AgentGraphState:
        return await self._finish_graph.ainvoke(
            {
                "request": request,
                "prepared": prepared,
                "response_content": response_content,
                "response_tokens": response_tokens,
                "model": model,
                "graph_steps": [],
            }
        )

    def metadata(self, state: AgentGraphState | None = None) -> dict[str, Any]:
        state = state or {}
        return {
            "name": "agent_turn",
            "backend": self.backend,
            "available": self.available,
            "nodes": list(state.get("graph_steps", [])),
            "error": "",
        }

    def _build_prepare_graph(self) -> Any:
        graph = StateGraph(dict)
        graph.add_node("start_trace", self._node("start_trace", self._nodes.graph_start_trace))
        graph.add_node("input_guardrails", self._node("input_guardrails", self._nodes.graph_input_guardrails))
        graph.add_node("semantic_cache", self._node("semantic_cache", self._nodes.graph_semantic_cache))
        graph.add_node("load_memory", self._node("load_memory", self._nodes.graph_load_memory))
        graph.add_node("tool_guardrails", self._node("tool_guardrails", self._nodes.graph_tool_guardrails))
        graph.add_node("execute_tools", self._node("execute_tools", self._nodes.graph_execute_tools))
        graph.add_node("react_observe", self._node("react_observe", self._nodes.graph_react_observe))
        graph.add_node("react_plan", self._node("react_plan", self._nodes.graph_react_plan))
        graph.add_node(
            "react_tool_guardrails",
            self._node("react_tool_guardrails", self._nodes.graph_react_tool_guardrails),
        )
        graph.add_node("react_execute", self._node("react_execute", self._nodes.graph_react_execute))
        graph.add_node("build_messages", self._node("build_messages", self._nodes.graph_build_messages))
        graph.set_entry_point("start_trace")
        graph.add_edge("start_trace", "input_guardrails")
        graph.add_conditional_edges(
            "input_guardrails",
            self._route_early_result,
            {"early_result": END, "continue": "semantic_cache"},
        )
        graph.add_conditional_edges(
            "semantic_cache",
            self._route_early_result,
            {"early_result": END, "continue": "load_memory"},
        )
        graph.add_edge("load_memory", "tool_guardrails")
        graph.add_conditional_edges(
            "tool_guardrails",
            self._route_early_result,
            {"early_result": END, "continue": "execute_tools"},
        )
        graph.add_edge("execute_tools", "react_observe")
        graph.add_conditional_edges(
            "react_observe",
            self._route_react_observe,
            {"continue": "react_plan", "done": "build_messages"},
        )
        graph.add_edge("react_plan", "react_tool_guardrails")
        graph.add_conditional_edges(
            "react_tool_guardrails",
            self._route_react_guardrails,
            {"continue": "react_execute", "done": "build_messages"},
        )
        graph.add_edge("react_execute", "react_observe")
        graph.add_edge("build_messages", END)
        return graph.compile()

    def _build_complete_graph(self) -> Any:
        graph = StateGraph(dict)
        graph.add_node("model_completion", self._node("model_completion", self._nodes.graph_model_completion))
        graph.add_node("finish_response", self._node("finish_response", self._nodes.graph_finish_response))
        graph.set_entry_point("model_completion")
        graph.add_edge("model_completion", "finish_response")
        graph.add_edge("finish_response", END)
        return graph.compile()

    def _build_finish_graph(self) -> Any:
        graph = StateGraph(dict)
        graph.add_node("finish_response", self._node("finish_response", self._nodes.graph_finish_response))
        graph.set_entry_point("finish_response")
        graph.add_edge("finish_response", END)
        return graph.compile()

    def _node(self, name: str, handler: AgentGraphNode) -> AgentGraphNode:
        async def _run(state: AgentGraphState) -> AgentGraphState:
            next_state = await handler(dict(state))
            return {
                **next_state,
                "graph_steps": [*next_state.get("graph_steps", []), name],
            }

        return _run

    def _route_early_result(self, state: AgentGraphState) -> str:
        return "early_result" if state.get("early_result") is not None else "continue"

    def _route_react_observe(self, state: AgentGraphState) -> str:
        return "done" if state.get("react_done", False) else "continue"

    def _route_react_guardrails(self, state: AgentGraphState) -> str:
        return "done" if state.get("react_done", False) else "continue"


__all__ = ["AgentGraphState", "AgentTurnGraph"]
