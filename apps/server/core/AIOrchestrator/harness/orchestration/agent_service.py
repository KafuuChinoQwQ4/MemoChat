from __future__ import annotations

import asyncio
import time
import uuid
from dataclasses import dataclass
from typing import AsyncIterator

import structlog
from config import settings
from harness.cache.semantic_cache import SemanticCacheHit, SemanticCacheService
from harness.contracts import GuardrailResult, HarnessRunResult, ToolObservation, TraceEvent
from harness.orchestration.agent_graph import AgentGraphState, AgentTurnGraph
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
from observability.langsmith_instrument import set_run_error, set_run_output, trace_context

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


def _request_metadata(request) -> dict:
    metadata = getattr(request, "metadata", {})
    return metadata if isinstance(metadata, dict) else {}


def _request_max_tokens(request, default_value: int | None = None, skill=None) -> int:
    if default_value is None:
        default_value = int(getattr(settings.agent, "max_tokens_per_response", 8192) or 8192)
    metadata = _request_metadata(request)
    raw_value = metadata.get("max_tokens", metadata.get("num_predict", default_value))
    if raw_value == default_value and skill is not None:
        raw_value = _skill_model_policy(skill).get("max_tokens", default_value)
    try:
        value = int(raw_value)
    except (TypeError, ValueError):
        value = default_value
    return min(max(value, 1), 16384)


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
    deployment_preference = getattr(request, "deployment_preference", "") or str(
        policy.get("deployment_preference", "any") or "any"
    )
    return prefer_backend, model_name, deployment_preference


@dataclass
class _PreparedTurn:
    session_id: str
    skill: object
    plan_steps: list
    trace: object
    langsmith_metadata: dict
    memory_snapshot: object
    observations: list[ToolObservation]
    messages: list[LLMMessage]
    semantic_lookup: object | None = None


@dataclass
class _EarlyTurnResult:
    result: HarnessRunResult


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
        semantic_cache: SemanticCacheService | None = None,
    ):
        self._planner = planner
        self._llm_registry = llm_registry
        self._tool_executor = tool_executor
        self._memory_service = memory_service
        self._trace_store = trace_store
        self._feedback_evaluator = feedback_evaluator
        self._guardrail_service = guardrail_service
        self._semantic_cache = semantic_cache
        self._turn_graph = AgentTurnGraph(self)

    @property
    def orchestration_backend(self) -> str:
        return self._turn_graph.backend

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

    async def _finish_semantic_cache_hit(
        self,
        trace_id: str,
        session_id: str,
        skill_name: str,
        request,
        hit: SemanticCacheHit,
        observations: list[str] | None = None,
    ) -> HarnessRunResult:
        response_content = hit.answer
        feedback_summary = (
            str(hit.metadata.get("feedback_summary") or "").strip() if isinstance(hit.metadata, dict) else ""
        )
        match_kind = getattr(hit, "match_kind", "") or "semantic"
        if not feedback_summary:
            feedback_summary = f"semantic_cache_hit {match_kind} similarity={hit.similarity:.4f}"

        cache_started = _now_ms()
        await self._trace_store.add_event(
            trace_id,
            TraceEvent(
                layer="cache",
                name="semantic_lookup",
                status="ok",
                summary=f"match={match_kind}, similarity={hit.similarity:.4f}, model={hit.model or 'cached'}",
                detail=hit.question[:1000] or response_content[:1000],
                started_at=cache_started,
                finished_at=_now_ms(),
                metadata={
                    "match_kind": match_kind,
                    "similarity": hit.similarity,
                    "distance": hit.distance,
                    "source_trace_id": hit.trace_id,
                    "cached_at": hit.cached_at,
                    "source_model": hit.model,
                },
            ),
        )

        if self._semantic_cache is not None and self._semantic_cache.persist_hits_to_memory:
            memory_save_started = _now_ms()
            self._schedule_semantic_cache_memory_save(request, session_id, response_content, trace_id)
            await self._trace_store.add_event(
                trace_id,
                TraceEvent(
                    layer="memory",
                    name="save_context",
                    status="queued",
                    summary="semantic cache hit memory persistence queued",
                    started_at=memory_save_started,
                    finished_at=_now_ms(),
                ),
            )

        model = f"semantic-cache:{hit.model or 'cached'}"
        await self._trace_store.finish_run(
            trace_id,
            status="completed",
            response_content=response_content,
            model=model,
            feedback_summary=feedback_summary,
            observations=observations or [],
        )
        trace_ref = self._trace_store.get_trace(trace_id)
        events = trace_ref.events if trace_ref else []
        return HarnessRunResult(
            session_id=session_id,
            content=response_content,
            tokens=0,
            model=model,
            trace_id=trace_id,
            skill=skill_name,
            feedback_summary=feedback_summary,
            observations=observations or [],
            events=events,
        )

    def _schedule_semantic_cache_memory_save(
        self, request, session_id: str, response_content: str, trace_id: str
    ) -> None:
        save_cache_hit = getattr(self._memory_service, "save_semantic_cache_hit", None)
        if save_cache_hit is None:
            logger.debug(
                "harness.semantic_cache.memory_save_skipped", trace_id=trace_id, reason="unsupported_memory_port"
            )
            return

        async def _save() -> None:
            try:
                await save_cache_hit(request.uid, session_id, request.content, response_content)
            except Exception as exc:
                logger.warning("harness.semantic_cache.memory_save_failed", trace_id=trace_id, error=str(exc))

        def _consume_background_result(done: asyncio.Task) -> None:
            try:
                done.result()
            except asyncio.CancelledError:
                logger.debug("harness.semantic_cache.memory_save_cancelled", trace_id=trace_id)

        task = asyncio.create_task(_save())
        task.add_done_callback(_consume_background_result)

    def _tool_specs(self):
        try:
            return self._tool_executor.list_tool_specs()
        except AttributeError:
            return []

    async def graph_start_trace(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        route = state["route"]
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
        langsmith_metadata = {
            "local_trace_id": trace.trace_id,
            "session_id": session_id,
            "skill": skill.name,
            "plan_steps": len(plan_steps),
            "route": route,
        }

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
                metadata={"graph_backend": self._turn_graph.backend, "graph_node": "start_trace"},
            ),
        )
        return {
            **state,
            "session_id": session_id,
            "skill": skill,
            "plan_steps": plan_steps,
            "trace": trace,
            "langsmith_metadata": langsmith_metadata,
        }

    async def graph_input_guardrails(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        session_id = state["session_id"]
        skill = state["skill"]
        plan_steps = state["plan_steps"]
        trace = state["trace"]
        langsmith_metadata = state["langsmith_metadata"]
        with trace_context(
            "agent_input_guardrails",
            run_type="chain",
            inputs={"uid": request.uid, "content": request.content, "skill": skill.name},
            metadata=langsmith_metadata,
            tags=["agent", "guardrails"],
        ) as guardrail_run:
            input_guardrails = self._guardrail_service.check_input(request, skill, plan_steps)
            set_run_output(
                guardrail_run, {"status": _guardrail_event_status(input_guardrails), "count": len(input_guardrails)}
            )
            await self._add_guardrail_event(trace.trace_id, "input", input_guardrails)
        if self._guardrail_service.has_blocking(input_guardrails):
            return {
                **state,
                "early_result": await self._finish_blocked_run(
                    trace.trace_id,
                    session_id,
                    skill.name,
                    "input",
                    input_guardrails,
                ),
            }
        return state

    async def graph_semantic_cache(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        session_id = state["session_id"]
        skill = state["skill"]
        plan_steps = state["plan_steps"]
        trace = state["trace"]
        semantic_lookup = None
        if self._semantic_cache is not None:
            semantic_lookup = await self._semantic_cache.lookup(request, skill, plan_steps)
            if semantic_lookup.hit is not None:
                return {
                    **state,
                    "semantic_lookup": semantic_lookup,
                    "early_result": await self._finish_semantic_cache_hit(
                        trace.trace_id,
                        session_id,
                        skill.name,
                        request,
                        semantic_lookup.hit,
                    ),
                }
        return {**state, "semantic_lookup": semantic_lookup}

    async def graph_load_memory(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        session_id = state["session_id"]
        skill = state["skill"]
        trace = state["trace"]
        langsmith_metadata = state["langsmith_metadata"]
        memory_started = _now_ms()
        with trace_context(
            "agent_memory_load",
            run_type="chain",
            inputs={"uid": request.uid, "session_id": session_id, "include_graph": skill.allow_graph},
            metadata=langsmith_metadata,
            tags=["agent", "memory"],
        ) as memory_run:
            try:
                memory_snapshot = await self._memory_service.load(
                    uid=request.uid,
                    session_id=session_id,
                    include_graph=skill.allow_graph,
                )
                set_run_output(
                    memory_run,
                    {
                        "chat_history": len(memory_snapshot.chat_history),
                        "episodic": len(memory_snapshot.episodic_summaries),
                    },
                )
            except Exception as exc:
                set_run_error(memory_run, exc)
                raise
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
        return {**state, "memory_snapshot": memory_snapshot}

    async def graph_tool_guardrails(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        session_id = state["session_id"]
        skill = state["skill"]
        plan_steps = state["plan_steps"]
        trace = state["trace"]
        langsmith_metadata = state["langsmith_metadata"]
        with trace_context(
            "agent_tool_plan_guardrails",
            run_type="chain",
            inputs={"uid": request.uid, "skill": skill.name, "plan_steps": [step.to_dict() for step in plan_steps]},
            metadata=langsmith_metadata,
            tags=["agent", "guardrails", "tools"],
        ) as tool_guardrail_run:
            tool_guardrails = self._guardrail_service.check_tool_plan(request, plan_steps, self._tool_specs(), skill)
            set_run_output(
                tool_guardrail_run, {"status": _guardrail_event_status(tool_guardrails), "count": len(tool_guardrails)}
            )
            await self._add_guardrail_event(trace.trace_id, "tool_plan", tool_guardrails)
        if self._guardrail_service.has_blocking(tool_guardrails):
            return {
                **state,
                "early_result": await self._finish_blocked_run(
                    trace.trace_id,
                    session_id,
                    skill.name,
                    "tool_plan",
                    tool_guardrails,
                ),
            }
        return state

    async def graph_execute_tools(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        skill = state["skill"]
        plan_steps = state["plan_steps"]
        trace = state["trace"]
        langsmith_metadata = state["langsmith_metadata"]
        execution_started = _now_ms()
        with trace_context(
            "agent_tool_execution",
            run_type="tool",
            inputs={
                "uid": request.uid,
                "content": request.content,
                "plan_steps": [step.to_dict() for step in plan_steps],
                "requested_tools": getattr(request, "requested_tools", []),
            },
            metadata=langsmith_metadata,
            tags=["agent", "tools"],
        ) as tool_run:
            try:
                observations = await self._tool_executor.execute(
                    uid=request.uid,
                    content=request.content,
                    plan_steps=plan_steps,
                    target_lang=getattr(request, "target_lang", ""),
                    requested_tools=getattr(request, "requested_tools", []),
                    tool_arguments=getattr(request, "tool_arguments", {}),
                    skill=skill,
                )
                set_run_output(
                    tool_run, {"observation_count": len(observations), "tools": [obs.name for obs in observations]}
                )
            except Exception as exc:
                set_run_error(tool_run, exc)
                raise
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
        return {
            **state,
            "observations": observations,
            "react_steps": list(plan_steps),
            "react_round": 0,
            "react_max_rounds": max(settings.harness.max_tool_rounds - len(plan_steps), 0),
            "react_done": False,
        }

    async def graph_react_observe(self, state: AgentGraphState) -> AgentGraphState:
        build_followup = getattr(self._planner, "build_react_followup_plan", None)
        if build_followup is None:
            return {**state, "react_done": True, "react_followup_steps": []}

        round_index = int(state.get("react_round", 0) or 0) + 1
        if round_index > int(state.get("react_max_rounds", 0) or 0):
            return {**state, "react_done": True, "react_followup_steps": []}

        request = state["request"]
        skill = state["skill"]
        all_steps = list(state.get("react_steps", state["plan_steps"]))
        all_observations = list(state.get("observations", []))
        assess = getattr(self._planner, "assess_react_observations", None)
        assessment = assess(request, skill, all_steps, all_observations) if assess is not None else {}
        if assessment:
            observe_started = _now_ms()
            await self._trace_store.add_event(
                state["trace"].trace_id,
                TraceEvent(
                    layer="orchestration",
                    name="react_observe",
                    status="ok",
                    summary=(
                        f"round={round_index}, confidence={assessment.get('confidence', '')}, "
                        f"needs_followup={assessment.get('needs_followup', False)}"
                    ),
                    detail=str(assessment),
                    started_at=observe_started,
                    finished_at=_now_ms(),
                    metadata={"round": round_index, **assessment},
                ),
            )

        remaining_steps = max(settings.harness.max_tool_rounds - len(all_steps), 0)
        followup_steps = build_followup(request, skill, all_steps, all_observations)[:remaining_steps]
        return {
            **state,
            "react_round": round_index,
            "react_assessment": assessment,
            "react_followup_steps": followup_steps,
            "react_done": not bool(followup_steps),
        }

    async def graph_react_plan(self, state: AgentGraphState) -> AgentGraphState:
        followup_steps = list(state.get("react_followup_steps", []))
        if not followup_steps:
            return {**state, "react_done": True}

        plan_started = _now_ms()
        await self._trace_store.add_event(
            state["trace"].trace_id,
            TraceEvent(
                layer="orchestration",
                name="react_plan",
                status="ok",
                summary=f"round={state.get('react_round', 0)}, followup_steps={len(followup_steps)}",
                detail=str([step.to_dict() for step in followup_steps]),
                started_at=plan_started,
                finished_at=_now_ms(),
                metadata={
                    "round": state.get("react_round", 0),
                    "base_steps": len(state.get("react_steps", state["plan_steps"])),
                    "observation_count": len(state.get("observations", [])),
                    "method": "plan_execute_plus_react",
                    "assessment": state.get("react_assessment", {}),
                },
            ),
        )
        return state

    async def graph_react_tool_guardrails(self, state: AgentGraphState) -> AgentGraphState:
        followup_steps = list(state.get("react_followup_steps", []))
        if not followup_steps:
            return {**state, "react_done": True}

        request = state["request"]
        skill = state["skill"]
        followup_guardrails = self._guardrail_service.check_tool_plan(
            request,
            followup_steps,
            self._tool_specs(),
            skill,
        )
        await self._add_guardrail_event(
            state["trace"].trace_id,
            f"react_tool_plan_round_{state.get('react_round', 0)}",
            followup_guardrails,
        )
        if self._guardrail_service.has_blocking(followup_guardrails):
            return {**state, "react_done": True, "react_followup_steps": []}
        return state

    async def graph_react_execute(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        followup_steps = list(state.get("react_followup_steps", []))
        if not followup_steps:
            return {**state, "react_done": True}

        round_index = int(state.get("react_round", 0) or 0)
        execution_started = _now_ms()
        with trace_context(
            "agent_react_tool_followup",
            run_type="tool",
            inputs={
                "uid": request.uid,
                "content": request.content,
                "round": round_index,
                "followup_steps": [step.to_dict() for step in followup_steps],
            },
            metadata={**state["langsmith_metadata"], "react_round": round_index},
            tags=["agent", "tools", "react"],
        ) as tool_run:
            try:
                followup_observations = await self._tool_executor.execute(
                    uid=request.uid,
                    content=request.content,
                    plan_steps=followup_steps,
                    target_lang=getattr(request, "target_lang", ""),
                    requested_tools=getattr(request, "requested_tools", []),
                    tool_arguments=getattr(request, "tool_arguments", {}),
                    skill=state["skill"],
                )
                set_run_output(
                    tool_run,
                    {
                        "round": round_index,
                        "observation_count": len(followup_observations),
                        "tools": [obs.name for obs in followup_observations],
                    },
                )
            except Exception as exc:
                set_run_error(tool_run, exc)
                raise

        observations = [*state.get("observations", []), *followup_observations]
        react_steps = [*state.get("react_steps", state["plan_steps"]), *followup_steps]
        await self._trace_store.add_event(
            state["trace"].trace_id,
            TraceEvent(
                layer="execution",
                name="react_tool_execution",
                status="ok",
                summary=f"round={round_index}, observations={len(followup_observations)}",
                detail="\n".join(observation.to_summary() for observation in followup_observations),
                started_at=execution_started,
                finished_at=_now_ms(),
                metadata={
                    "round": round_index,
                    "method": "react_followup",
                    "step_count": len(followup_steps),
                },
            ),
        )
        return {
            **state,
            "observations": observations,
            "react_steps": react_steps,
            "react_followup_steps": [],
            "react_done": False,
        }

    async def graph_build_messages(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        skill = state["skill"]
        memory_snapshot = state["memory_snapshot"]
        observations = state["observations"]
        system_prompt = self._planner.build_system_prompt(skill, memory_snapshot, observations)
        user_prompt = self._planner.build_user_prompt(request, observations)
        messages = (
            [LLMMessage(role="system", content=system_prompt)]
            + memory_snapshot.as_messages()
            + [LLMMessage(role="user", content=user_prompt)]
        )
        return {**state, "messages": messages}

    async def _prepare_turn(self, request, route: str) -> _PreparedTurn | _EarlyTurnResult:
        state = await self._turn_graph.run_prepare(request, route)
        early_result = state.get("early_result")
        if early_result is not None:
            return _EarlyTurnResult(early_result)

        return self._prepared_from_graph_state(state)

    def _prepared_from_graph_state(self, state: AgentGraphState) -> _PreparedTurn:
        return _PreparedTurn(
            session_id=state["session_id"],
            skill=state["skill"],
            plan_steps=state["plan_steps"],
            trace=state["trace"],
            langsmith_metadata=state["langsmith_metadata"],
            memory_snapshot=state["memory_snapshot"],
            observations=state["observations"],
            messages=state["messages"],
            semantic_lookup=state.get("semantic_lookup"),
        )

    async def graph_model_completion(self, state: AgentGraphState) -> AgentGraphState:
        request = state["request"]
        prepared = state["prepared"]
        model_started = _now_ms()
        try:
            prefer_backend, model_name, deployment_preference = _skill_model_target(prepared.skill, request)
            with trace_context(
                "agent_model_completion",
                run_type="llm",
                inputs={
                    "message_count": len(prepared.messages),
                    "prefer_backend": prefer_backend,
                    "model_name": model_name,
                    "deployment_preference": deployment_preference,
                },
                metadata=prepared.langsmith_metadata,
                tags=["agent", "llm"],
            ) as model_run:
                try:
                    response = await self._llm_registry.complete(
                        prepared.messages,
                        prefer_backend=prefer_backend,
                        model_name=model_name,
                        deployment_preference=deployment_preference,
                        max_tokens=_request_max_tokens(request, skill=prepared.skill),
                        temperature=_request_temperature(
                            request,
                            0.4 if prepared.skill.name in {"summarize_thread", "translate_text"} else 0.7,
                            skill=prepared.skill,
                        ),
                        think=_request_think_enabled(request, skill=prepared.skill),
                    )
                    set_run_output(
                        model_run,
                        {
                            "model": response.model,
                            "tokens": response.usage.total_tokens,
                            "finish_reason": response.finish_reason,
                        },
                    )
                except Exception as exc:
                    set_run_error(model_run, exc)
                    raise
        except Exception as exc:
            logger.error("harness.model_completion.failed", trace_id=prepared.trace.trace_id, error=str(exc))
            await self._trace_store.add_event(
                prepared.trace.trace_id,
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
                prepared.trace.trace_id,
                status="failed",
                response_content=str(exc),
                model="",
                feedback_summary=f"model_failed={type(exc).__name__}",
                observations=[observation.to_summary() for observation in prepared.observations],
            )
            raise
        await self._trace_store.add_event(
            prepared.trace.trace_id,
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
        response_tokens = response.usage.total_tokens or max(len(response.content) // 4, 0)
        return {
            **state,
            "response_content": response.content,
            "response_tokens": response_tokens,
            "model": response.model,
        }

    async def graph_finish_response(self, state: AgentGraphState) -> AgentGraphState:
        result = await self._finish_response(
            state["prepared"],
            state["request"],
            state.get("response_content", ""),
            int(state.get("response_tokens", 0) or 0),
            state.get("model", ""),
        )
        return {**state, "result": result}

    async def _finish_response(
        self,
        prepared: _PreparedTurn,
        request,
        response_content: str,
        response_tokens: int,
        model: str,
    ) -> HarnessRunResult:
        with trace_context(
            "agent_output_guardrails",
            run_type="chain",
            inputs={"uid": request.uid, "skill": prepared.skill.name, "observation_count": len(prepared.observations)},
            metadata=prepared.langsmith_metadata,
            tags=["agent", "guardrails"],
        ) as output_guardrail_run:
            output_guardrails = self._guardrail_service.check_output(response_content, prepared.observations)
            set_run_output(
                output_guardrail_run,
                {"status": _guardrail_event_status(output_guardrails), "count": len(output_guardrails)},
            )
            await self._add_guardrail_event(prepared.trace.trace_id, "output", output_guardrails)
        if self._guardrail_service.has_blocking(output_guardrails):
            return await self._finish_blocked_run(
                prepared.trace.trace_id,
                prepared.session_id,
                prepared.skill.name,
                "output",
                output_guardrails,
                observations=[observation.to_summary() for observation in prepared.observations],
                tokens=response_tokens,
                model=model,
            )

        memory_save_started = _now_ms()
        with trace_context(
            "agent_memory_writeback",
            run_type="chain",
            inputs={"uid": request.uid, "session_id": prepared.session_id},
            metadata=prepared.langsmith_metadata,
            tags=["agent", "memory"],
        ) as memory_save_run:
            try:
                await self._memory_service.save_after_response(
                    request.uid,
                    prepared.session_id,
                    request.content,
                    response_content,
                )
                set_run_output(memory_save_run, {"status": "ok"})
            except Exception as exc:
                set_run_error(memory_save_run, exc)
                raise
        await self._trace_store.add_event(
            prepared.trace.trace_id,
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
        feedback_summary = self._feedback_evaluator.build_summary(
            prepared.skill.name,
            prepared.observations,
            response_content,
        )
        await self._trace_store.add_event(
            prepared.trace.trace_id,
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
            prepared.trace.trace_id,
            status="completed",
            response_content=response_content,
            model=model,
            feedback_summary=feedback_summary,
            observations=[observation.to_summary() for observation in prepared.observations],
        )
        if self._semantic_cache is not None:
            await self._semantic_cache.store(
                request,
                prepared.skill,
                prepared.plan_steps,
                answer=response_content,
                model=model,
                tokens=response_tokens,
                trace_id=prepared.trace.trace_id,
                feedback_summary=feedback_summary,
                vector=prepared.semantic_lookup.vector if prepared.semantic_lookup is not None else None,
            )

        trace_ref = self._trace_store.get_trace(prepared.trace.trace_id)
        events = trace_ref.events if trace_ref else []
        return HarnessRunResult(
            session_id=prepared.session_id,
            content=response_content,
            tokens=response_tokens,
            model=model,
            trace_id=prepared.trace.trace_id,
            skill=prepared.skill.name,
            feedback_summary=feedback_summary,
            observations=[observation.to_summary() for observation in prepared.observations],
            events=events,
        )

    async def run_turn(self, request) -> HarnessRunResult:
        prepared = await self._prepare_turn(request, "/agent/run")
        if isinstance(prepared, _EarlyTurnResult):
            return prepared.result

        state = await self._turn_graph.run_complete(request, prepared)
        return state["result"]

    async def stream_turn(self, request) -> AsyncIterator[dict]:
        msg_id = uuid.uuid4().hex

        prepared = await self._prepare_turn(request, "/agent/run/stream")
        if isinstance(prepared, _EarlyTurnResult):
            result = prepared.result
            yield {
                "chunk": result.content,
                "is_final": True,
                "msg_id": msg_id,
                "total_tokens": result.tokens,
                "trace_id": result.trace_id,
                "skill": result.skill,
                "feedback_summary": result.feedback_summary,
                "observations": result.observations,
                "events": _trace_events_payload(self._trace_store, result.trace_id),
            }
            return

        accumulated = ""
        model_name = ""
        model_started = _now_ms()

        try:
            prefer_backend, selected_model_name, deployment_preference = _skill_model_target(prepared.skill, request)
            async for chunk in self._llm_registry.stream(
                prepared.messages,
                prefer_backend=prefer_backend,
                model_name=selected_model_name,
                deployment_preference=deployment_preference,
                max_tokens=_request_max_tokens(request, skill=prepared.skill),
                temperature=_request_temperature(
                    request,
                    0.4 if prepared.skill.name in {"summarize_thread", "translate_text"} else 0.7,
                    skill=prepared.skill,
                ),
                think=_request_think_enabled(request, skill=prepared.skill),
            ):
                if chunk.content:
                    accumulated += chunk.content
                    model_name = chunk.model or model_name
                    yield {
                        "chunk": chunk.content,
                        "is_final": False,
                        "msg_id": msg_id,
                        "total_tokens": max(len(accumulated) // 4, 0),
                        "trace_id": prepared.trace.trace_id,
                        "skill": prepared.skill.name,
                    }

            await self._trace_store.add_event(
                prepared.trace.trace_id,
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

            response_tokens = max(len(accumulated) // 4, 0)
            finish_state = await self._turn_graph.run_finish(
                request,
                prepared,
                accumulated,
                response_tokens,
                model_name,
            )
            result = finish_state["result"]
            yield {
                "chunk": "",
                "is_final": True,
                "msg_id": msg_id,
                "total_tokens": result.tokens,
                "trace_id": result.trace_id,
                "skill": result.skill,
                "feedback_summary": result.feedback_summary,
                "observations": result.observations,
                "events": _trace_events_payload(self._trace_store, result.trace_id),
            }
        except Exception as exc:
            logger.error("harness.stream.failed", error=str(exc))
            await self._trace_store.add_event(
                prepared.trace.trace_id,
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
                prepared.trace.trace_id,
                status="failed",
                response_content=str(exc),
                model=model_name,
                feedback_summary=f"stream_failed={exc}",
                observations=[observation.to_summary() for observation in prepared.observations],
            )
            yield {
                "chunk": f"发生错误: {exc}",
                "is_final": True,
                "msg_id": msg_id,
                "total_tokens": 0,
                "trace_id": prepared.trace.trace_id,
                "skill": prepared.skill.name,
                "feedback_summary": f"stream_failed={exc}",
                "observations": [observation.to_summary() for observation in prepared.observations],
                "events": _trace_events_payload(self._trace_store, prepared.trace.trace_id),
            }
