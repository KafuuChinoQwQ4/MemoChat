from __future__ import annotations

import re

import structlog

from graph.recommendation import RecommendationEngine
from harness.contracts import PlanStep, ToolObservation
from harness.knowledge.service import KnowledgeService
from tools.registry import ToolRegistry

logger = structlog.get_logger()


class ToolExecutor:
    def __init__(self, knowledge_service: KnowledgeService, graph_memory_service=None):
        self._knowledge_service = knowledge_service
        self._graph_memory_service = graph_memory_service
        self._tool_registry = ToolRegistry.get_instance()
        self._recommendation = RecommendationEngine()

    def list_tools(self) -> list[dict]:
        return [
            {
                "name": tool.name,
                "description": tool.description or "",
                "source": "mcp" if tool.name.startswith("mcp_") else "builtin",
            }
            for tool in self._tool_registry.get_tools()
        ]

    async def execute(
        self,
        uid: int,
        content: str,
        plan_steps: list[PlanStep],
        target_lang: str = "",
        requested_tools: list[str] | None = None,
        tool_arguments: dict[str, dict] | None = None,
    ) -> list[ToolObservation]:
        observations: list[ToolObservation] = []
        requested_tools = requested_tools or []
        tool_arguments = tool_arguments or {}

        for step in plan_steps:
            try:
                if step.action == "knowledge_search":
                    hits = await self._knowledge_service.search(uid, content, top_k=step.parameters.get("top_k"))
                    observations.append(
                        ToolObservation(
                            name="knowledge_search",
                            source="knowledge_base",
                            output=self._format_knowledge_hits(hits),
                            metadata={"hit_count": len(hits)},
                        )
                    )
                elif step.action == "web_search":
                    result = await self._invoke_tool("duckduckgo_search", {"query": content})
                    observations.append(ToolObservation(name="duckduckgo_search", source="builtin", output=result))
                elif step.action == "calculate":
                    expression = self._extract_expression(content)
                    result = await self._invoke_tool("calculator", {"expression": expression})
                    observations.append(ToolObservation(name="calculator", source="builtin", output=result))
                elif step.action == "graph_recall":
                    if self._graph_memory_service is not None:
                        graph_result = await self._graph_memory_service.describe_graph_state(uid, limit=5)
                    else:
                        graph_result = str(self._recommendation.recommend_friends_mixed(uid, limit=5))
                    observations.append(
                        ToolObservation(
                            name="graph_recommender",
                            source="neo4j",
                            output=str(graph_result),
                            metadata={"item_count": len(graph_result) if isinstance(graph_result, list) else 1},
                        )
                    )
                elif step.action == "mcp_tool":
                    for tool_name in requested_tools:
                        result = await self._invoke_tool(tool_name, tool_arguments.get(tool_name, {}))
                        observations.append(ToolObservation(name=tool_name, source="mcp", output=result))
            except Exception as exc:
                logger.warning("harness.tool_execution.failed", action=step.action, error=str(exc))
                observations.append(
                    ToolObservation(
                        name=step.action,
                        source="execution",
                        output=f"工具执行失败: {exc}",
                    )
                )

        return observations

    async def _invoke_tool(self, tool_name: str, payload: dict) -> str:
        for tool in self._tool_registry.get_tools():
            if tool.name != tool_name:
                continue
            result = await tool.ainvoke(payload)
            if isinstance(result, str):
                return result
            return str(result)
        raise ValueError(f"Tool not found: {tool_name}")

    def _format_knowledge_hits(self, hits: list[dict]) -> str:
        if not hits:
            return "知识库中没有找到相关内容。"
        formatted = []
        for index, hit in enumerate(hits, start=1):
            formatted.append(
                f"[{index}] 来源: {hit.get('source', '')} 相关度: {hit.get('score', 0.0):.2f}\n{hit.get('content', '')[:320]}"
            )
        return "\n\n".join(formatted)

    def _extract_expression(self, text: str) -> str:
        matches = re.findall(r"[\d\.\+\-\*\/\(\)\%\^\s]+", text)
        if not matches:
            return text
        return max(matches, key=len).strip() or text
