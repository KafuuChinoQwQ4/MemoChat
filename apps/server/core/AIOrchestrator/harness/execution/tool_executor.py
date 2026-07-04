from __future__ import annotations

import asyncio
import re
from typing import TYPE_CHECKING

import structlog
from graph.recommendation import RecommendationEngine
from harness.contracts import AgentSkill, PlanStep, ToolObservation, ToolSpec
from observability.langsmith_instrument import set_run_error, set_run_output, trace_context
from tools.knowledge_base_tool import KnowledgeBaseTool
from tools.registry import ToolRegistry

if TYPE_CHECKING:
    from harness.knowledge.service import KnowledgeService

logger = structlog.get_logger()


class ToolExecutor:
    def __init__(self, knowledge_service: KnowledgeService | None, graph_memory_service=None):
        self._knowledge_service = knowledge_service
        self._graph_memory_service = graph_memory_service
        self._tool_registry = ToolRegistry.get_instance()
        self._register_knowledge_tool()
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
        skill: AgentSkill | None = None,
    ) -> list[ToolObservation]:
        observations: list[ToolObservation] = []
        requested_tools = requested_tools or []
        tool_arguments = tool_arguments or {}

        for step in plan_steps:
            try:
                if step.action == "knowledge_search":
                    payload = self._knowledge_payload(uid, content, step.parameters)
                    result = await self._invoke_tool("knowledge_base_search", payload)
                    observations.append(
                        ToolObservation(
                            name="knowledge_search",
                            source="builtin",
                            output=result,
                            metadata={
                                "tool": "knowledge_base_search",
                                "hit_count": self._knowledge_hit_count(result),
                                "strategy": step.parameters.get("strategy", ""),
                            },
                        )
                    )
                elif step.action == "web_search":
                    queries = self._web_queries(content, step.parameters)
                    max_results = self._coerce_int(step.parameters.get("max_results"), default=5, minimum=1, maximum=10)
                    result = await self._run_web_searches(queries, max_results=max_results)
                    observations.append(
                        ToolObservation(
                            name="web_search",
                            source="builtin",
                            output=result,
                            metadata={
                                "query_count": len(queries),
                                "queries": queries,
                                "strategy": step.parameters.get("strategy", ""),
                            },
                        )
                    )
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
                        if not self._mcp_tool_allowed(skill, tool_name):
                            observations.append(
                                ToolObservation(
                                    name=tool_name,
                                    source="mcp",
                                    output=f"工具执行失败: MCP tool is not allowed by skill policy: {tool_name}",
                                    metadata={"blocked": True, "policy": "skill_mcp_allowlist"},
                                )
                            )
                            continue
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

    def _mcp_tool_allowed(self, skill: AgentSkill | None, tool_name: str) -> bool:
        if skill is None or not skill.allow_mcp:
            return False
        return str(tool_name or "").strip() in self._skill_allowed_tools(skill)

    def _skill_allowed_tools(self, skill: AgentSkill) -> set[str]:
        metadata = skill.metadata if isinstance(skill.metadata, dict) else {}
        allowed: set[str] = set()

        def add_values(raw) -> None:
            if not isinstance(raw, (list, tuple, set)):
                return
            for item in raw:
                text = str(item or "").strip()
                if text:
                    allowed.add(text)

        add_values(metadata.get("allowed_tools"))
        tool_policy = metadata.get("tool_policy", {})
        if isinstance(tool_policy, dict):
            add_values(tool_policy.get("allowed_tools"))
        agent_spec = metadata.get("agent_spec", {})
        if isinstance(agent_spec, dict):
            add_values(agent_spec.get("allowed_tools"))
            agent_tool_policy = agent_spec.get("tool_policy", {})
            if isinstance(agent_tool_policy, dict):
                add_values(agent_tool_policy.get("allowed_tools"))
        return allowed

    def _register_knowledge_tool(self) -> None:
        if self._knowledge_service is None:
            return
        tool = KnowledgeBaseTool(search_provider=self._knowledge_service.search).get_tool()
        replace = getattr(self._tool_registry, "register_or_replace_tool", None)
        if callable(replace):
            replace(tool)

    async def _invoke_tool(self, tool_name: str, payload: dict) -> str:
        for tool in self._tool_registry.get_tools():
            if tool.name != tool_name:
                continue
            spec = self._tool_to_spec(tool)
            clean_payload = self._validate_tool_payload(spec, payload or {})
            self._require_confirmation_if_needed(spec, payload or {})
            with trace_context(
                f"tool_{tool_name}",
                run_type="tool",
                inputs={"tool": tool_name, "payload": clean_payload},
                metadata={
                    "tool": tool_name,
                    "source": spec.source,
                    "category": spec.category,
                    "permission": spec.permission,
                    "timeout_seconds": spec.timeout_seconds,
                },
                tags=["tool", spec.source],
            ) as run:
                try:
                    result = await asyncio.wait_for(tool.ainvoke(clean_payload), timeout=spec.timeout_seconds)
                    output = result if isinstance(result, str) else str(result)
                    set_run_output(run, {"output": output[:2000]})
                    return output
                except Exception as exc:
                    set_run_error(run, exc)
                    raise
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
            "web_search": ToolSpec(
                name="web_search",
                display_name="Web Search",
                description="Search the public web and return summarized results.",
                source="builtin",
                category="search",
                parameters_schema={
                    "type": "object",
                    "required": ["query"],
                    "properties": {
                        "query": {"type": "string"},
                        "max_results": {"type": "integer"},
                    },
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
        if any(
            token in lowered for token in ("write", "create", "update", "edit", "insert", "upsert", "send", "publish")
        ):
            return "write"
        return "read"

    def _validate_tool_payload(self, spec: ToolSpec, payload: dict) -> dict:
        schema = spec.parameters_schema or {}
        clean_payload = {
            key: value for key, value in payload.items() if key not in {"confirm", "confirmed", "confirmation_token"}
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

    def _knowledge_payload(self, uid: int, content: str, parameters: dict) -> dict:
        payload = {"query": content, "uid": uid}
        top_k = parameters.get("top_k") if isinstance(parameters, dict) else None
        if top_k is not None:
            payload["top_k"] = self._coerce_int(top_k, default=5, minimum=1, maximum=12)
        return payload

    def _knowledge_hit_count(self, output: str) -> int:
        if not output or "知识库中没有找到" in output or "知识库检索失败" in output:
            return 0
        return len(re.findall(r"(?m)^\[\d+\]\s+来源:", output))

    def _extract_expression(self, text: str) -> str:
        matches = re.findall(r"[\d\.\+\-\*\/\(\)\%\^\s]+", text)
        if not matches:
            return text
        return max(matches, key=len).strip() or text

    async def _run_web_searches(self, queries: list[str], max_results: int) -> str:
        outputs: list[str] = []
        per_query = max(1, min(max_results, 5))
        for query in queries:
            result = await self._invoke_tool("web_search", {"query": query, "max_results": per_query})
            outputs.append(f"### 查询: {query}\n{result}")
        return "\n\n".join(outputs)

    def _web_queries(self, content: str, parameters: dict) -> list[str]:
        raw_queries = parameters.get("queries") if isinstance(parameters, dict) else None
        if isinstance(raw_queries, str):
            candidates = [raw_queries]
        elif isinstance(raw_queries, list):
            candidates = [str(query) for query in raw_queries]
        else:
            candidates = [content]

        deduped: list[str] = []
        seen: set[str] = set()
        for query in candidates:
            cleaned = re.sub(r"\s+", " ", str(query or "")).strip()
            key = cleaned.lower()
            if not cleaned or key in seen:
                continue
            seen.add(key)
            deduped.append(cleaned[:180])
            if len(deduped) >= 3:
                break
        return deduped or [content.strip() or "MemoChat"]

    def _coerce_int(self, value, default: int, minimum: int, maximum: int) -> int:
        try:
            parsed = int(value)
        except (TypeError, ValueError):
            parsed = default
        return min(max(parsed, minimum), maximum)
