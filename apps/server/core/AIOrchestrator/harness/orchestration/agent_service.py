from __future__ import annotations

import time
import uuid
from typing import AsyncIterator

import structlog

from harness.contracts import GuardrailResult, HarnessRunResult, TraceEvent
from harness.ports import (
    FeedbackPort,
    GuardrailPort,
    LLMCompletionPort,
    MemoryPort,
    SkillPlanningPort,
    ToolExecutionPort,
    TraceStorePort,
)
from llm.base import LLMMessage

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


def _request_metadata(request) -> dict:
    metadata = getattr(request, "metadata", {})
    return metadata if isinstance(metadata, dict) else {}


def _request_max_tokens(request, default_value: int = 2048, skill=None) -> int:
    metadata = _request_metadata(request)
    raw_value = metadata.get("max_tokens", metadata.get("num_predict", default_value))
    if raw_value == default_value and skill is not None:
        raw_value = _skill_model_policy(skill).get("max_tokens", default_value)
    try:
        value = int(raw_value)
    except (TypeError, ValueError):
        value = default_value
    return min(max(value, 1), 4096)


def _request_temperature(request, default_value: float, skill=None) -> float:
    metadata = _request_metadata(request)
    raw_value = metadata.get("temperature", default_value)
    if raw_value == default_value and skill is not None:
        raw_value = _skill_model_policy(skill).get("temperature", default_value)
    try:
        value = float(raw_value)
    except (TypeError, ValueError):
        value = default_value
    return min(max(value, 0.0), 2.0)


def _request_think_enabled(request, skill=None) -> bool:
    metadata = _request_metadata(request)
    raw_value = metadata.get("think", metadata.get("enable_thinking", False))
    if not raw_value and skill is not None:
        raw_value = _skill_model_policy(skill).get("think", False)
    if isinstance(raw_value, bool):
        return raw_value
    if isinstance(raw_value, str):
        return raw_value.strip().lower() in {"1", "true", "yes", "on"}
    return bool(raw_value)


def _trace_events_payload(trace_store: TraceStorePort, trace_id: str) -> list[dict]:
    trace = trace_store.get_trace(trace_id)
    if trace is None:
        return []
    return [event.to_dict() for event in trace.events]


def _guardrail_event_status(results: list[GuardrailResult]) -> str:
    if any(result.blocking or result.status == "block" for result in results):
        return "blocked"
    if any(result.status == "warn" for result in results):
        return "warn"
    return "ok"


def _guardrail_summary(results: list[GuardrailResult]) -> str:
    if not results:
        return "results=0"
    counts: dict[str, int] = {}
    for result in results:
        counts[result.status] = counts.get(result.status, 0) + 1
    return ", ".join(f"{status}={count}" for status, count in sorted(counts.items()))


def _guardrail_block_message(stage: str, results: list[GuardrailResult]) -> str:
    messages = [result.message for result in results if result.blocking or result.status == "block"]
    reason = "; ".join(message for message in messages if message) or "Guardrail blocked the request."
    return f"Guardrail blocked {stage}: {reason}"


def _context_pack_metadata(memory_snapshot) -> dict:
    context_pack = getattr(memory_snapshot, "context_pack", None)
    if context_pack is None:
        return {}
    return {
        "token_budget": context_pack.token_budget,
        "sections": [
            {
                "name": section.name,
                "source": section.source,
                "token_count": section.token_count,
                "metadata": dict(section.metadata),
            }
            for section in context_pack.sections
        ],
    }


def _skill_metadata(skill) -> dict:
    metadata = getattr(skill, "metadata", {})
    return metadata if isinstance(metadata, dict) else {}


def _skill_model_policy(skill) -> dict:
    metadata = _skill_metadata(skill)
    policy = metadata.get("model_policy", {})
    return policy if isinstance(policy, dict) else {}


def _skill_number_policy(skill, key: str, default_value: int) -> int:
    policy = _skill_model_policy(skill)
    raw_value = policy.get(key, default_value)
    try:
        return int(raw_value)
    except (TypeError, ValueError):
        return default_value


def _skill_float_policy(skill, key: str, default_value: float) -> float:
    policy = _skill_model_policy(skill)
    raw_value = policy.get(key, default_value)
    try:
        return float(raw_value)
    except (TypeError, ValueError):
        return default_value


def _skill_bool_policy(skill, key: str, default_value: bool) -> bool:
    policy = _skill_model_policy(skill)
    raw_value = policy.get(key, default_value)
    if isinstance(raw_value, bool):
        return raw_value
    if isinstance(raw_value, str):
        return raw_value.strip().lower() in {"1", "true", "yes", "on"}
    return bool(raw_value)


def _skill_model_target(skill, request) -> tuple[str, str, str]:
    policy = _skill_model_policy(skill)
    prefer_backend = getattr(request, "model_type", "") or str(policy.get("provider", "") or "")
    model_name = getattr(request, "model_name", "") or str(policy.get("model", "") or "")
    deployment_preference = getattr(request, "deployment_preference", "") or str(policy.get("deployment_preference", "any") or "any")
    return prefer_backend, model_name, deployment_preference


class AgentHarnessService:
    def __init__(
        self,
        planner: SkillPlanningPort,
        llm_registry: LLMCompletionPort,
        tool_executor: ToolExecutionPort,
        memory_service: MemoryPort,
        trace_store: TraceStorePort,
        feedback_evaluator: FeedbackPort,
        guardrail_service: GuardrailPort,
    ):
        self._planner = planner
        self._llm_registry = llm_registry
        self._tool_executor = tool_executor
        self._memory_service = memory_service
        self._trace_store = trace_store
        self._feedback_evaluator = feedback_evaluator
        self._guardrail_service = guardrail_service

    async def _add_guardrail_event(self, trace_id: str, name: str, results: list[GuardrailResult]) -> None:
        started_at = _now_ms()
        await self._trace_store.add_event(
            trace_id,
            TraceEvent(
                layer="guardrails",
                name=name,
                status=_guardrail_event_status(results),
                summary=_guardrail_summary(results),
                detail=str([result.to_dict() for result in results]),
                started_at=started_at,
                finished_at=_now_ms(),
                metadata={"results": [result.to_dict() for result in results]},
            ),
        )

    async def _finish_blocked_run(
        self,
        trace_id: str,
        session_id: str,
        skill_name: str,
        stage: str,
        results: list[GuardrailResult],
        observations: list[str] | None = None,
        tokens: int = 0,
        model: str = "",
    ) -> HarnessRunResult:
        response_content = _guardrail_block_message(stage, results)
        await self._trace_store.finish_run(
            trace_id,
            status="blocked",
            response_content=response_content,
            model=model,
            feedback_summary=f"guardrail_blocked={stage}",
            observations=observations or [],
        )
        trace_ref = self._trace_store.get_trace(trace_id)
        events = trace_ref.events if trace_ref else []
        return HarnessRunResult(
            session_id=session_id,
            content=response_content,
            tokens=tokens,
            model=model,
            trace_id=trace_id,
            skill=skill_name,
            feedback_summary=f"guardrail_blocked={stage}",
            observations=observations or [],
            events=events,
        )

    def _tool_specs(self):
        try:
            return self._tool_executor.list_tool_specs()
        except AttributeError:
            return []

    async def run_turn(self, request) -> HarnessRunResult:
        session_id = getattr(request, "session_id", "") or uuid.uuid4().hex
        skill = self._planner.resolve_skill(request)
        plan_steps = self._planner.build_plan(request, skill)

        trace = await self._trace_store.start_run(
            uid=request.uid,
            session_id=session_id,
            skill_name=skill.name,
            request_summary=request.content[:1000],
            plan_steps=plan_steps,
        )

        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="orchestration",
                name="plan",
                status="ok",
                summary=f"skill={skill.name}, steps={len(plan_steps)}",
                detail=str([step.to_dict() for step in plan_steps]),
                started_at=_now_ms(),
                finished_at=_now_ms(),
            ),
        )

        input_guardrails = self._guardrail_service.check_input(request, skill, plan_steps)
        await self._add_guardrail_event(trace.trace_id, "input", input_guardrails)
        if self._guardrail_service.has_blocking(input_guardrails):
            return await self._finish_blocked_run(
                trace.trace_id,
                session_id,
                skill.name,
                "input",
                input_guardrails,
            )

        memory_started = _now_ms()
        memory_snapshot = await self._memory_service.load(
            uid=request.uid,
            session_id=session_id,
            include_graph=skill.allow_graph,
        )
        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="memory",
                name="load_context",
                status="ok",
                summary=f"history={len(memory_snapshot.chat_history)}, episodic={len(memory_snapshot.episodic_summaries)}",
                started_at=memory_started,
                finished_at=_now_ms(),
                metadata=_context_pack_metadata(memory_snapshot),
            ),
        )

        tool_guardrails = self._guardrail_service.check_tool_plan(request, plan_steps, self._tool_specs())
        await self._add_guardrail_event(trace.trace_id, "tool_plan", tool_guardrails)
        if self._guardrail_service.has_blocking(tool_guardrails):
            return await self._finish_blocked_run(
                trace.trace_id,
                session_id,
                skill.name,
                "tool_plan",
                tool_guardrails,
            )

        execution_started = _now_ms()
        observations = await self._tool_executor.execute(
            uid=request.uid,
            content=request.content,
            plan_steps=plan_steps,
            target_lang=getattr(request, "target_lang", ""),
            requested_tools=getattr(request, "requested_tools", []),
            tool_arguments=getattr(request, "tool_arguments", {}),
        )
        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="execution",
                name="tool_execution",
                status="ok",
                summary=f"observations={len(observations)}",
                detail="\n".join(observation.to_summary() for observation in observations),
                started_at=execution_started,
                finished_at=_now_ms(),
            ),
        )

        system_prompt = self._planner.build_system_prompt(skill, memory_snapshot, observations)
        user_prompt = self._planner.build_user_prompt(request, observations)
        messages = [LLMMessage(role="system", content=system_prompt)] + memory_snapshot.as_messages() + [
            LLMMessage(role="user", content=user_prompt)
        ]

        model_started = _now_ms()
        try:
            prefer_backend, model_name, deployment_preference = _skill_model_target(skill, request)
            response = await self._llm_registry.complete(
                messages,
                prefer_backend=prefer_backend,
                model_name=model_name,
                deployment_preference=deployment_preference,
                max_tokens=_request_max_tokens(request, skill=skill),
                temperature=_request_temperature(
                    request,
                    0.4 if skill.name in {"summarize_thread", "translate_text"} else 0.7,
                    skill=skill,
                ),
                think=_request_think_enabled(request, skill=skill),
            )
        except Exception as exc:
            logger.error("harness.model_completion.failed", trace_id=trace.trace_id, error=str(exc))
            await self._trace_store.add_event(
                trace.trace_id,
                TraceEvent(
                    layer="execution",
                    name="model_completion",
                    status="failed",
                    summary=type(exc).__name__,
                    detail=str(exc)[:2000],
                    started_at=model_started,
                    finished_at=_now_ms(),
                ),
            )
            await self._trace_store.finish_run(
                trace.trace_id,
                status="failed",
                response_content=str(exc),
                model="",
                feedback_summary=f"model_failed={type(exc).__name__}",
                observations=[observation.to_summary() for observation in observations],
            )
            raise
        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="execution",
                name="model_completion",
                status="ok",
                summary=f"model={response.model}",
                detail=response.content[:1000],
                started_at=model_started,
                finished_at=_now_ms(),
                metadata={"tokens": response.usage.total_tokens},
            ),
        )

        output_guardrails = self._guardrail_service.check_output(response.content, observations)
        await self._add_guardrail_event(trace.trace_id, "output", output_guardrails)
        if self._guardrail_service.has_blocking(output_guardrails):
            return await self._finish_blocked_run(
                trace.trace_id,
                session_id,
                skill.name,
                "output",
                output_guardrails,
                observations=[observation.to_summary() for observation in observations],
                tokens=response.usage.total_tokens or max(len(response.content) // 4, 0),
                model=response.model,
            )

        memory_save_started = _now_ms()
        await self._memory_service.save_after_response(request.uid, session_id, request.content, response.content)
        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="memory",
                name="save_context",
                status="ok",
                summary="conversation persisted",
                started_at=memory_save_started,
                finished_at=_now_ms(),
            ),
        )

        feedback_started = _now_ms()
        feedback_summary = self._feedback_evaluator.build_summary(skill.name, observations, response.content)
        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="feedback",
                name="evaluate_response",
                status="ok",
                summary=feedback_summary,
                started_at=feedback_started,
                finished_at=_now_ms(),
            ),
        )
        await self._trace_store.finish_run(
            trace.trace_id,
            status="completed",
            response_content=response.content,
            model=response.model,
            feedback_summary=feedback_summary,
            observations=[observation.to_summary() for observation in observations],
        )

        trace_ref = self._trace_store.get_trace(trace.trace_id)
        events = trace_ref.events if trace_ref else []
        return HarnessRunResult(
            session_id=session_id,
            content=response.content,
            tokens=response.usage.total_tokens or max(len(response.content) // 4, 0),
            model=response.model,
            trace_id=trace.trace_id,
            skill=skill.name,
            feedback_summary=feedback_summary,
            observations=[observation.to_summary() for observation in observations],
            events=events,
        )

    async def stream_turn(self, request) -> AsyncIterator[dict]:
        session_id = getattr(request, "session_id", "") or uuid.uuid4().hex
        skill = self._planner.resolve_skill(request)
        plan_steps = self._planner.build_plan(request, skill)
        msg_id = uuid.uuid4().hex

        trace = await self._trace_store.start_run(
            uid=request.uid,
            session_id=session_id,
            skill_name=skill.name,
            request_summary=request.content[:1000],
            plan_steps=plan_steps,
        )
        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="orchestration",
                name="plan",
                status="ok",
                summary=f"skill={skill.name}, steps={len(plan_steps)}",
                detail=str([step.to_dict() for step in plan_steps]),
                started_at=_now_ms(),
                finished_at=_now_ms(),
            ),
        )

        input_guardrails = self._guardrail_service.check_input(request, skill, plan_steps)
        await self._add_guardrail_event(trace.trace_id, "input", input_guardrails)
        if self._guardrail_service.has_blocking(input_guardrails):
            result = await self._finish_blocked_run(
                trace.trace_id,
                session_id,
                skill.name,
                "input",
                input_guardrails,
            )
            yield {
                "chunk": result.content,
                "is_final": True,
                "msg_id": msg_id,
                "total_tokens": 0,
                "trace_id": trace.trace_id,
                "skill": skill.name,
                "feedback_summary": result.feedback_summary,
                "observations": [],
                "events": _trace_events_payload(self._trace_store, trace.trace_id),
            }
            return

        memory_started = _now_ms()
        memory_snapshot = await self._memory_service.load(
            uid=request.uid,
            session_id=session_id,
            include_graph=skill.allow_graph,
        )
        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="memory",
                name="load_context",
                status="ok",
                summary=f"history={len(memory_snapshot.chat_history)}, episodic={len(memory_snapshot.episodic_summaries)}",
                started_at=memory_started,
                finished_at=_now_ms(),
                metadata=_context_pack_metadata(memory_snapshot),
            ),
        )

        tool_guardrails = self._guardrail_service.check_tool_plan(request, plan_steps, self._tool_specs())
        await self._add_guardrail_event(trace.trace_id, "tool_plan", tool_guardrails)
        if self._guardrail_service.has_blocking(tool_guardrails):
            result = await self._finish_blocked_run(
                trace.trace_id,
                session_id,
                skill.name,
                "tool_plan",
                tool_guardrails,
            )
            yield {
                "chunk": result.content,
                "is_final": True,
                "msg_id": msg_id,
                "total_tokens": 0,
                "trace_id": trace.trace_id,
                "skill": skill.name,
                "feedback_summary": result.feedback_summary,
                "observations": [],
                "events": _trace_events_payload(self._trace_store, trace.trace_id),
            }
            return

        execution_started = _now_ms()
        observations = await self._tool_executor.execute(
            uid=request.uid,
            content=request.content,
            plan_steps=plan_steps,
            target_lang=getattr(request, "target_lang", ""),
            requested_tools=getattr(request, "requested_tools", []),
            tool_arguments=getattr(request, "tool_arguments", {}),
        )
        await self._trace_store.add_event(
            trace.trace_id,
            TraceEvent(
                layer="execution",
                name="tool_execution",
                status="ok",
                summary=f"observations={len(observations)}",
                detail="\n".join(observation.to_summary() for observation in observations),
                started_at=execution_started,
                finished_at=_now_ms(),
            ),
        )

        system_prompt = self._planner.build_system_prompt(skill, memory_snapshot, observations)
        user_prompt = self._planner.build_user_prompt(request, observations)
        messages = [LLMMessage(role="system", content=system_prompt)] + memory_snapshot.as_messages() + [
            LLMMessage(role="user", content=user_prompt)
        ]

        accumulated = ""
        model_name = ""
        model_started = _now_ms()

        try:
            prefer_backend, selected_model_name, deployment_preference = _skill_model_target(skill, request)
            async for chunk in self._llm_registry.stream(
                messages,
                prefer_backend=prefer_backend,
                model_name=selected_model_name,
                deployment_preference=deployment_preference,
                max_tokens=_request_max_tokens(request, skill=skill),
                temperature=_request_temperature(
                    request,
                    0.4 if skill.name in {"summarize_thread", "translate_text"} else 0.7,
                    skill=skill,
                ),
                think=_request_think_enabled(request, skill=skill),
            ):
                if chunk.content:
                    accumulated += chunk.content
                    model_name = chunk.model or model_name
                    yield {
                        "chunk": chunk.content,
                        "is_final": False,
                        "msg_id": msg_id,
                        "total_tokens": max(len(accumulated) // 4, 0),
                        "trace_id": trace.trace_id,
                        "skill": skill.name,
                    }

            await self._trace_store.add_event(
                trace.trace_id,
                TraceEvent(
                    layer="execution",
                    name="model_completion",
                    status="ok",
                    summary=f"model={model_name}",
                    detail=accumulated[:1000],
                    started_at=model_started,
                    finished_at=_now_ms(),
                    metadata={"tokens": max(len(accumulated) // 4, 0)},
                ),
            )

            output_guardrails = self._guardrail_service.check_output(accumulated, observations)
            await self._add_guardrail_event(trace.trace_id, "output", output_guardrails)
            if self._guardrail_service.has_blocking(output_guardrails):
                result = await self._finish_blocked_run(
                    trace.trace_id,
                    session_id,
                    skill.name,
                    "output",
                    output_guardrails,
                    observations=[observation.to_summary() for observation in observations],
                    tokens=max(len(accumulated) // 4, 0),
                    model=model_name,
                )
                yield {
                    "chunk": result.content,
                    "is_final": True,
                    "msg_id": msg_id,
                    "total_tokens": result.tokens,
                    "trace_id": trace.trace_id,
                    "skill": skill.name,
                    "feedback_summary": result.feedback_summary,
                    "observations": [observation.to_summary() for observation in observations],
                    "events": _trace_events_payload(self._trace_store, trace.trace_id),
                }
                return

            memory_save_started = _now_ms()
            await self._memory_service.save_after_response(request.uid, session_id, request.content, accumulated)
            await self._trace_store.add_event(
                trace.trace_id,
                TraceEvent(
                    layer="memory",
                    name="save_context",
                    status="ok",
                    summary="conversation persisted",
                    started_at=memory_save_started,
                    finished_at=_now_ms(),
                ),
            )

            feedback_started = _now_ms()
            feedback_summary = self._feedback_evaluator.build_summary(skill.name, observations, accumulated)
            await self._trace_store.add_event(
                trace.trace_id,
                TraceEvent(
                    layer="feedback",
                    name="evaluate_response",
                    status="ok",
                    summary=feedback_summary,
                    started_at=feedback_started,
                    finished_at=_now_ms(),
                ),
            )
            await self._trace_store.finish_run(
                trace.trace_id,
                status="completed",
                response_content=accumulated,
                model=model_name,
                feedback_summary=feedback_summary,
                observations=[observation.to_summary() for observation in observations],
            )
            yield {
                "chunk": "",
                "is_final": True,
                "msg_id": msg_id,
                "total_tokens": max(len(accumulated) // 4, 0),
                "trace_id": trace.trace_id,
                "skill": skill.name,
                "feedback_summary": feedback_summary,
                "observations": [observation.to_summary() for observation in observations],
                "events": _trace_events_payload(self._trace_store, trace.trace_id),
            }
        except Exception as exc:
            logger.error("harness.stream.failed", error=str(exc))
            await self._trace_store.add_event(
                trace.trace_id,
                TraceEvent(
                    layer="execution",
                    name="model_completion",
                    status="failed",
                    summary=type(exc).__name__,
                    detail=str(exc)[:2000],
                    started_at=model_started,
                    finished_at=_now_ms(),
                ),
            )
            await self._trace_store.finish_run(
                trace.trace_id,
                status="failed",
                response_content=str(exc),
                model=model_name,
                feedback_summary=f"stream_failed={exc}",
                observations=[observation.to_summary() for observation in observations],
            )
            yield {
                "chunk": f"发生错误: {exc}",
                "is_final": True,
                "msg_id": msg_id,
                "total_tokens": 0,
                "trace_id": trace.trace_id,
                "skill": skill.name,
                "feedback_summary": f"stream_failed={exc}",
                "observations": [observation.to_summary() for observation in observations],
                "events": _trace_events_payload(self._trace_store, trace.trace_id),
            }
