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
            parameters_schema=raw.get("parameters_schema", {}) if isinstance(raw.get("parameters_schema", {}), dict) else {},
            timeout_seconds=int(raw.get("timeout_seconds", 30) or 30),
            permission=str(raw.get("permission", "read")),
            requires_confirmation=bool(raw.get("requires_confirmation", False)),
            metadata=raw.get("metadata", {}) if isinstance(raw.get("metadata", {}), dict) else {},
        )

    def _is_confirmed(self, payload: dict[str, Any]) -> bool:
        return payload.get("confirmed", payload.get("confirm", False)) is True

