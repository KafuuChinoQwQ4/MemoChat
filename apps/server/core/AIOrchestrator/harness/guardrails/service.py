from __future__ import annotations

from typing import Any, Iterable

from harness.contracts import AgentSkill, GuardrailResult, PlanStep, ToolObservation, ToolSpec


class GuardrailService:
    def check_input(self, request: Any, skill: AgentSkill, plan_steps: list[PlanStep]) -> list[GuardrailResult]:
        content = str(getattr(request, "content", "") or "")
        results: list[GuardrailResult] = []

        if not content.strip():
            results.append(
                GuardrailResult(
                    name="input_not_empty",
                    status="block",
                    message="Request content is empty.",
                    blocking=True,
                    metadata={"skill": skill.name, "plan_steps": len(plan_steps)},
                )
            )
            return results

        results.append(
            GuardrailResult(
                name="input_not_empty",
                status="pass",
                message="Request content is present.",
                metadata={"length": len(content)},
            )
        )

        if len(content) > 24000:
            results.append(
                GuardrailResult(
                    name="input_length",
                    status="warn",
                    message="Request content is unusually long.",
                    metadata={"length": len(content), "soft_limit": 24000},
                )
            )

        return results

    def check_tool_plan(
        self,
        request: Any,
        plan_steps: list[PlanStep],
        tool_specs: Iterable[ToolSpec | dict[str, Any]] | None = None,
        skill: AgentSkill | None = None,
    ) -> list[GuardrailResult]:
        requested_tools = list(getattr(request, "requested_tools", []) or [])
        tool_arguments = getattr(request, "tool_arguments", {}) or {}
        if not isinstance(tool_arguments, dict):
            tool_arguments = {}

        specs = self._spec_map(tool_specs or [])
        results: list[GuardrailResult] = []

        if not requested_tools:
            if any(step.action == "mcp_tool" for step in plan_steps):
                results.append(
                    GuardrailResult(
                        name="tool_selection",
                        status="warn",
                        message="Plan requested external tools but no tool names were selected.",
                    )
                )
            else:
                results.append(
                    GuardrailResult(
                        name="tool_selection",
                        status="pass",
                        message="No external tools requested.",
                    )
                )
            return results

        for tool_name in requested_tools:
            spec = specs.get(tool_name)
            payload = tool_arguments.get(tool_name, {})
            if not isinstance(payload, dict):
                payload = {}

            if spec is None:
                results.append(
                    GuardrailResult(
                        name="tool_exists",
                        status="block",
                        message=f"Requested tool is not registered: {tool_name}",
                        blocking=True,
                        metadata={"tool": tool_name},
                    )
                )
                continue

            if self._is_mcp_tool_request(tool_name, spec, plan_steps):
                policy_result = self._check_mcp_tool_policy(skill, tool_name)
                if policy_result is not None:
                    results.append(policy_result)
                    continue

            if spec.requires_confirmation and not self._is_confirmed(payload):
                results.append(
                    GuardrailResult(
                        name="tool_confirmation",
                        status="block",
                        message=f"Tool requires explicit confirmation before execution: {tool_name}",
                        blocking=True,
                        metadata={
                            "tool": tool_name,
                            "permission": spec.permission,
                            "source": spec.source,
                        },
                    )
                )
                continue

            results.append(
                GuardrailResult(
                    name="tool_permission",
                    status="pass",
                    message=f"Tool request allowed: {tool_name}",
                    metadata={
                        "tool": tool_name,
                        "permission": spec.permission,
                        "requires_confirmation": spec.requires_confirmation,
                    },
                )
            )

        return results

    def _is_mcp_tool_request(self, tool_name: str, spec: ToolSpec | None, plan_steps: list[PlanStep]) -> bool:
        return (
            str(tool_name or "").startswith("mcp_")
            or (spec is not None and spec.source == "mcp")
            or any(step.action == "mcp_tool" for step in plan_steps)
        )

    def _check_mcp_tool_policy(self, skill: AgentSkill | None, tool_name: str) -> GuardrailResult | None:
        if skill is None or not skill.allow_mcp:
            return GuardrailResult(
                name="mcp_tool_policy",
                status="block",
                message=f"MCP tool is not allowed by the selected skill: {tool_name}",
                blocking=True,
                metadata={"tool": tool_name, "skill": getattr(skill, "name", "") if skill else ""},
            )

        allowed_tools = self._skill_allowed_tools(skill)
        if tool_name not in allowed_tools:
            return GuardrailResult(
                name="mcp_tool_policy",
                status="block",
                message=f"MCP tool is outside the selected skill allowlist: {tool_name}",
                blocking=True,
                metadata={"tool": tool_name, "skill": skill.name, "allowed_tools": sorted(allowed_tools)},
            )
        return None

    def _skill_allowed_tools(self, skill: AgentSkill) -> set[str]:
        metadata = skill.metadata if isinstance(skill.metadata, dict) else {}
        allowed: set[str] = set()

        def add_values(raw: Any) -> None:
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

    def check_output(self, response_text: str, observations: list[ToolObservation]) -> list[GuardrailResult]:
        results: list[GuardrailResult] = []
        response_text = response_text or ""

        if not response_text.strip():
            results.append(
                GuardrailResult(
                    name="output_not_empty",
                    status="block",
                    message="Model output is empty.",
                    blocking=True,
                )
            )
            return results

        results.append(
            GuardrailResult(
                name="output_not_empty",
                status="pass",
                message="Model output is present.",
                metadata={"length": len(response_text), "observations": len(observations)},
            )
        )

        lowered = response_text.lower()
        if "<think>" in lowered or "</think>" in lowered:
            results.append(
                GuardrailResult(
                    name="thinking_leakage",
                    status="warn",
                    message="Model output appears to include hidden reasoning markup.",
                )
            )

        failed_tools = [observation.name for observation in observations if "工具执行失败" in observation.output]
        if failed_tools:
            results.append(
                GuardrailResult(
                    name="tool_observation_failures",
                    status="warn",
                    message="One or more tool observations reported execution failure.",
                    metadata={"tools": failed_tools},
                )
            )

        return results

    def has_blocking(self, results: list[GuardrailResult]) -> bool:
        return any(result.blocking or result.status == "block" for result in results)

    def _spec_map(self, raw_specs: Iterable[ToolSpec | dict[str, Any]]) -> dict[str, ToolSpec]:
        specs: dict[str, ToolSpec] = {}
        for raw in raw_specs:
            spec = raw if isinstance(raw, ToolSpec) else self._dict_to_spec(raw)
            if spec is not None:
                specs[spec.name] = spec
        return specs

    def _dict_to_spec(self, raw: dict[str, Any]) -> ToolSpec | None:
        if not isinstance(raw, dict) or not raw.get("name"):
            return None
        return ToolSpec(
            name=str(raw.get("name", "")),
            display_name=str(raw.get("display_name", raw.get("name", ""))),
            description=str(raw.get("description", "")),
            source=str(raw.get("source", "builtin")),
            category=str(raw.get("category", "")),
            parameters_schema=raw.get("parameters_schema", {})
            if isinstance(raw.get("parameters_schema", {}), dict)
            else {},
            timeout_seconds=int(raw.get("timeout_seconds", 30) or 30),
            permission=str(raw.get("permission", "read")),
            requires_confirmation=bool(raw.get("requires_confirmation", False)),
            metadata=raw.get("metadata", {}) if isinstance(raw.get("metadata", {}), dict) else {},
        )

    def _is_confirmed(self, payload: dict[str, Any]) -> bool:
        return payload.get("confirmed", payload.get("confirm", False)) is True
