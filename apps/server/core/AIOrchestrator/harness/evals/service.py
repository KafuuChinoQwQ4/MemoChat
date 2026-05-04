from __future__ import annotations

import json
import re
import time
from dataclasses import dataclass, field
from pathlib import Path
from types import SimpleNamespace
from typing import Any

from harness.contracts import AgentTrace


def _now_ms() -> int:
    return int(time.time() * 1000)


@dataclass
class AgentEvalCase:
    case_id: str
    name: str
    description: str = ""
    request: dict[str, Any] = field(default_factory=dict)
    expectations: dict[str, Any] = field(default_factory=dict)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return {
            "case_id": self.case_id,
            "name": self.name,
            "description": self.description,
            "request": dict(self.request),
            "expectations": dict(self.expectations),
            "metadata": dict(self.metadata),
        }


class AgentEvalService:
    def __init__(
        self,
        agent_service,
        trace_store,
        case_dir: str | Path | None = None,
    ):
        self._agent_service = agent_service
        self._trace_store = trace_store
        self._case_dir = Path(case_dir) if case_dir is not None else Path(__file__).resolve().parent / "agent_tasks"

    def list_cases(self) -> list[AgentEvalCase]:
        cases: list[AgentEvalCase] = []
        if self._case_dir.exists():
            for path in sorted(self._case_dir.glob("*.json")):
                case = self._load_case_file(path)
                if case is not None:
                    cases.append(case)
        if cases:
            return cases
        return [
            AgentEvalCase(
                case_id="trace_shape_basic",
                name="Basic trace shape",
                description="Checks that a completed trace has core harness layers and no exact-text dependency.",
                expectations={
                    "expected_status": "completed",
                    "required_layers": ["orchestration", "guardrails", "memory", "execution", "feedback"],
                    "required_events": ["plan", "load_context", "tool_execution", "model_completion"],
                    "max_latency_ms": 120000,
                },
                metadata={"source": "builtin"},
            )
        ]

    def get_case(self, case_id: str) -> AgentEvalCase | None:
        for case in self.list_cases():
            if case.case_id == case_id:
                return case
        return None

    async def run_eval(
        self,
        case_id: str = "",
        trace_id: str = "",
        uid: int = 0,
    ) -> dict[str, Any]:
        case = self.get_case(case_id) if case_id else None
        if case is None:
            case = self.list_cases()[0]

        live_latency_ms = 0
        trace: AgentTrace | None = None
        if trace_id:
            trace = await self._load_trace(trace_id)
        elif case.request:
            started_at = _now_ms()
            trace = await self._run_live_case(case, uid)
            live_latency_ms = _now_ms() - started_at
        else:
            return self._failure_result(
                case,
                "",
                ["trace_id is required for replay-only eval cases"],
            )

        if trace is None:
            return self._failure_result(case, trace_id, [f"trace not found: {trace_id}"])
        return self.evaluate_trace(case, trace, live_latency_ms=live_latency_ms)

    async def run_suite(self, trace_id: str = "", uid: int = 0) -> list[dict[str, Any]]:
        results: list[dict[str, Any]] = []
        for case in self.list_cases():
            results.append(await self.run_eval(case.case_id, trace_id=trace_id, uid=uid))
        return results

    def evaluate_trace(
        self,
        case: AgentEvalCase,
        trace: AgentTrace,
        live_latency_ms: int = 0,
    ) -> dict[str, Any]:
        expectations = case.expectations
        metrics = self._trace_metrics(trace, live_latency_ms)
        failures: list[str] = []

        expected_status = expectations.get("expected_status", expectations.get("status", ""))
        if expected_status and trace.status != expected_status:
            failures.append(f"expected status {expected_status!r}, got {trace.status!r}")

        layers = set(metrics["layers"])
        for layer in expectations.get("required_layers", []):
            if layer not in layers:
                failures.append(f"missing required layer: {layer}")

        event_names = set(metrics["events"])
        for event_name in expectations.get("required_events", []):
            if event_name not in event_names:
                failures.append(f"missing required event: {event_name}")
        for event_name in expectations.get("forbidden_events", []):
            if event_name in event_names:
                failures.append(f"forbidden event present: {event_name}")

        tools = set(metrics["tools"])
        for tool in expectations.get("required_tools", []):
            if tool not in tools:
                failures.append(f"missing required tool: {tool}")
        for tool in expectations.get("forbidden_tools", []):
            if tool in tools:
                failures.append(f"forbidden tool present: {tool}")

        required_guardrails = expectations.get("required_guardrail_statuses", {})
        if isinstance(required_guardrails, dict):
            guardrails = metrics["guardrails"]
            for name, status in required_guardrails.items():
                if guardrails.get(name) != status:
                    failures.append(
                        f"guardrail {name!r} expected {status!r}, got {guardrails.get(name)!r}"
                    )

        max_latency_ms = int(expectations.get("max_latency_ms") or 0)
        if max_latency_ms > 0 and metrics["latency_ms"] > max_latency_ms:
            failures.append(f"latency {metrics['latency_ms']}ms exceeds {max_latency_ms}ms")

        return {
            "case_id": case.case_id,
            "passed": not failures,
            "trace_id": trace.trace_id,
            "failures": failures,
            "metrics": metrics,
            "expected": dict(expectations),
            "observed": {
                "status": trace.status,
                "model": trace.model,
                "skill": trace.skill,
                "event_count": len(trace.events),
            },
        }

    async def _load_trace(self, trace_id: str) -> AgentTrace | None:
        get_or_load = getattr(self._trace_store, "get_trace_or_load", None)
        if get_or_load is not None:
            return await get_or_load(trace_id)
        get_trace = getattr(self._trace_store, "get_trace", None)
        return get_trace(trace_id) if get_trace is not None else None

    async def _run_live_case(self, case: AgentEvalCase, uid: int) -> AgentTrace | None:
        request_data = dict(case.request)
        if uid > 0:
            request_data["uid"] = uid
        request_data.setdefault("uid", 0)
        request_data.setdefault("session_id", "")
        request_data.setdefault("content", "")
        request_data.setdefault("model_type", "")
        request_data.setdefault("model_name", "")
        request_data.setdefault("deployment_preference", "any")
        request_data.setdefault("skill_name", "")
        request_data.setdefault("feature_type", "")
        request_data.setdefault("target_lang", "")
        request_data.setdefault("requested_tools", [])
        request_data.setdefault("tool_arguments", {})
        request_data.setdefault("metadata", {})

        result = await self._agent_service.run_turn(SimpleNamespace(**request_data))
        trace = None
        get_trace = getattr(self._trace_store, "get_trace", None)
        if get_trace is not None:
            trace = get_trace(result.trace_id)
        if trace is not None:
            return trace
        return AgentTrace(
            trace_id=result.trace_id,
            uid=int(request_data["uid"]),
            session_id=result.session_id,
            skill=result.skill,
            request_summary=request_data["content"][:1000],
            events=result.events,
            observations=result.observations,
            status="completed",
            response_content=result.content,
            model=result.model,
            feedback_summary=result.feedback_summary,
            started_at=_now_ms(),
            finished_at=_now_ms(),
        )

    def _trace_metrics(self, trace: AgentTrace, live_latency_ms: int) -> dict[str, Any]:
        latency_ms = live_latency_ms
        if trace.started_at > 0 and trace.finished_at > 0:
            latency_ms = max(trace.finished_at - trace.started_at, 0)
        return {
            "latency_ms": latency_ms,
            "model": trace.model,
            "skill": trace.skill,
            "layers": [event.layer for event in trace.events],
            "events": [event.name for event in trace.events],
            "tools": sorted(self._extract_tools(trace)),
            "guardrails": self._extract_guardrails(trace),
        }

    def _extract_tools(self, trace: AgentTrace) -> set[str]:
        tools: set[str] = set()
        for observation in trace.observations:
            self._add_tool_from_text(tools, str(observation))
        for event in trace.events:
            metadata_tools = event.metadata.get("tools", event.metadata.get("tool_names", []))
            if isinstance(metadata_tools, list):
                tools.update(str(tool) for tool in metadata_tools if str(tool))
            if event.name == "tool_execution":
                self._add_tool_from_text(tools, event.detail)
        return tools

    def _extract_guardrails(self, trace: AgentTrace) -> dict[str, str]:
        guardrails: dict[str, str] = {}
        for event in trace.events:
            if event.layer != "guardrails":
                continue
            results = event.metadata.get("results", [])
            if isinstance(results, list):
                for result in results:
                    if isinstance(result, dict) and result.get("name"):
                        guardrails[str(result["name"])] = str(result.get("status", ""))
            guardrails.setdefault(event.name, event.status)
        return guardrails

    def _add_tool_from_text(self, tools: set[str], text: str) -> None:
        for match in re.finditer(r"\[([A-Za-z0-9_.:-]+)\]", text or ""):
            tools.add(match.group(1))

    def _failure_result(self, case: AgentEvalCase, trace_id: str, failures: list[str]) -> dict[str, Any]:
        return {
            "case_id": case.case_id,
            "passed": False,
            "trace_id": trace_id,
            "failures": failures,
            "metrics": {},
            "expected": dict(case.expectations),
            "observed": {},
        }

    def _load_case_file(self, path: Path) -> AgentEvalCase | None:
        try:
            raw = json.loads(path.read_text(encoding="utf-8"))
        except Exception:
            return None
        case_id = str(raw.get("case_id", raw.get("id", path.stem)))
        return AgentEvalCase(
            case_id=case_id,
            name=str(raw.get("name", case_id)),
            description=str(raw.get("description", "")),
            request=raw.get("request", {}) if isinstance(raw.get("request", {}), dict) else {},
            expectations=raw.get("expectations", {}) if isinstance(raw.get("expectations", {}), dict) else {},
            metadata=raw.get("metadata", {}) if isinstance(raw.get("metadata", {}), dict) else {},
        )
