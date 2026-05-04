from __future__ import annotations

import asyncio
import re

import structlog

from graph.recommendation import RecommendationEngine
from harness.contracts import PlanStep, ToolObservation, ToolSpec
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
        return [spec.to_dict() for spec in self.list_tool_specs()]

    def list_tool_specs(self) -> list[ToolSpec]:
        return [self._tool_to_spec(tool) for tool in self._tool_registry.get_tools()]

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
            spec = self._tool_to_spec(tool)
            clean_payload = self._validate_tool_payload(spec, payload or {})
            self._require_confirmation_if_needed(spec, payload or {})
            result = await asyncio.wait_for(tool.ainvoke(clean_payload), timeout=spec.timeout_seconds)
            if isinstance(result, str):
                return result
            return str(result)
        raise ValueError(f"Tool not found: {tool_name}")

    def _tool_to_spec(self, tool) -> ToolSpec:
        builtin_specs = self._builtin_tool_specs()
        if tool.name in builtin_specs:
            return builtin_specs[tool.name]

        permission = self._infer_permission(tool.name)
        return ToolSpec(
            name=tool.name,
            display_name=tool.name,
            description=tool.description or "",
            source="mcp" if tool.name.startswith("mcp_") else "builtin",
            category="external" if tool.name.startswith("mcp_") else "tool",
            parameters_schema=self._extract_parameter_schema(tool),
            timeout_seconds=60 if tool.name.startswith("mcp_") else 30,
            permission=permission,
            requires_confirmation=permission in {"write", "admin"},
            metadata={"dynamic": True},
        )

    def _builtin_tool_specs(self) -> dict[str, ToolSpec]:
        return {
            "duckduckgo_search": ToolSpec(
                name="duckduckgo_search",
                display_name="DuckDuckGo Search",
                description="Search the public web and return summarized results.",
                source="builtin",
                category="search",
                parameters_schema={
                    "type": "object",
                    "required": ["query"],
                    "properties": {"query": {"type": "string"}},
                },
                timeout_seconds=30,
                permission="read",
            ),
            "knowledge_base_search": ToolSpec(
                name="knowledge_base_search",
                display_name="Knowledge Base Search",
                description="Search private user knowledge-base chunks.",
                source="builtin",
                category="knowledge",
                parameters_schema={
                    "type": "object",
                    "required": ["query"],
                    "properties": {
                        "query": {"type": "string"},
                        "uid": {"type": "integer"},
                        "top_k": {"type": "integer"},
                    },
                },
                timeout_seconds=30,
                permission="read",
            ),
            "calculator": ToolSpec(
                name="calculator",
                display_name="Calculator",
                description="Evaluate a safe math expression.",
                source="builtin",
                category="compute",
                parameters_schema={
                    "type": "object",
                    "required": ["expression"],
                    "properties": {"expression": {"type": "string"}},
                },
                timeout_seconds=10,
                permission="read",
            ),
            "translator": ToolSpec(
                name="translator",
                display_name="Translator",
                description="Translate text to a target language.",
                source="builtin",
                category="language",
                parameters_schema={
                    "type": "object",
                    "required": ["text"],
                    "properties": {
                        "text": {"type": "string"},
                        "target_lang": {"type": "string"},
                    },
                },
                timeout_seconds=120,
                permission="read",
            ),
        }

    def _extract_parameter_schema(self, tool) -> dict:
        args_schema = getattr(tool, "args_schema", None)
        if args_schema is None:
            return {"type": "object", "properties": {}}
        try:
            if hasattr(args_schema, "model_json_schema"):
                return args_schema.model_json_schema()
            if hasattr(args_schema, "schema"):
                return args_schema.schema()
        except Exception:
            logger.warning("harness.tool_schema.extract_failed", tool=getattr(tool, "name", ""))
        return {"type": "object", "properties": {}}

    def _infer_permission(self, tool_name: str) -> str:
        lowered = tool_name.lower()
        if any(token in lowered for token in ("delete", "remove", "drop", "destroy", "purge", "reset")):
            return "admin"
        if any(token in lowered for token in ("write", "create", "update", "edit", "insert", "upsert", "send", "publish")):
            return "write"
        return "read"

    def _validate_tool_payload(self, spec: ToolSpec, payload: dict) -> dict:
        schema = spec.parameters_schema or {}
        clean_payload = {
            key: value
            for key, value in payload.items()
            if key not in {"confirm", "confirmed", "confirmation_token"}
        }

        required = schema.get("required", [])
        properties = schema.get("properties", {})
        for name in required:
            if name not in clean_payload:
                raise ValueError(f"Tool {spec.name} missing required parameter: {name}")

        for name, value in clean_payload.items():
            prop = properties.get(name)
            if not isinstance(prop, dict):
                continue
            expected = prop.get("type")
            if expected and not self._matches_json_type(value, expected):
                raise ValueError(f"Tool {spec.name} parameter {name} must be {expected}")
        return clean_payload

    def _matches_json_type(self, value, expected) -> bool:
        expected_types = expected if isinstance(expected, list) else [expected]
        for expected_type in expected_types:
            if expected_type == "string" and isinstance(value, str):
                return True
            if expected_type == "integer" and isinstance(value, int) and not isinstance(value, bool):
                return True
            if expected_type == "number" and isinstance(value, (int, float)) and not isinstance(value, bool):
                return True
            if expected_type == "boolean" and isinstance(value, bool):
                return True
            if expected_type == "object" and isinstance(value, dict):
                return True
            if expected_type == "array" and isinstance(value, list):
                return True
        return False

    def _require_confirmation_if_needed(self, spec: ToolSpec, payload: dict) -> None:
        if not spec.requires_confirmation:
            return
        confirmed = payload.get("confirmed", payload.get("confirm", False))
        if confirmed is True:
            return
        raise PermissionError(f"Tool {spec.name} requires confirmation before execution")

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
