from __future__ import annotations

import time
import uuid
from typing import AsyncIterator

import structlog

from harness.contracts import HarnessRunResult, ToolObservation, TraceEvent
from harness.execution.tool_executor import ToolExecutor
from harness.feedback.evaluator import FeedbackEvaluator
from harness.feedback.trace_store import AgentTraceStore
from harness.llm.service import LLMEndpointRegistry
from harness.memory.service import MemoryService
from harness.orchestration.planner import PlanningPolicy
from llm.base import LLMMessage

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


class AgentHarnessService:
    def __init__(
        self,
        planner: PlanningPolicy,
        llm_registry: LLMEndpointRegistry,
        tool_executor: ToolExecutor,
        memory_service: MemoryService,
        trace_store: AgentTraceStore,
        feedback_evaluator: FeedbackEvaluator,
    ):
        self._planner = planner
        self._llm_registry = llm_registry
        self._tool_executor = tool_executor
        self._memory_service = memory_service
        self._trace_store = trace_store
        self._feedback_evaluator = feedback_evaluator

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
            ),
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
        response = await self._llm_registry.complete(
            messages,
            prefer_backend=request.model_type,
            model_name=request.model_name,
            deployment_preference=getattr(request, "deployment_preference", "any"),
            max_tokens=2048,
            temperature=0.4 if skill.name in {"summarize_thread", "translate_text"} else 0.7,
        )
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

        await self._memory_service.save_after_response(request.uid, session_id, request.content, response.content)
        feedback_summary = self._feedback_evaluator.build_summary(skill.name, observations, response.content)
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

        trace = await self._trace_store.start_run(
            uid=request.uid,
            session_id=session_id,
            skill_name=skill.name,
            request_summary=request.content[:1000],
            plan_steps=plan_steps,
        )

        memory_snapshot = await self._memory_service.load(
            uid=request.uid,
            session_id=session_id,
            include_graph=skill.allow_graph,
        )
        observations = await self._tool_executor.execute(
            uid=request.uid,
            content=request.content,
            plan_steps=plan_steps,
            target_lang=getattr(request, "target_lang", ""),
            requested_tools=getattr(request, "requested_tools", []),
            tool_arguments=getattr(request, "tool_arguments", {}),
        )

        system_prompt = self._planner.build_system_prompt(skill, memory_snapshot, observations)
        user_prompt = self._planner.build_user_prompt(request, observations)
        messages = [LLMMessage(role="system", content=system_prompt)] + memory_snapshot.as_messages() + [
            LLMMessage(role="user", content=user_prompt)
        ]

        msg_id = uuid.uuid4().hex
        accumulated = ""
        model_name = ""

        try:
            async for chunk in self._llm_registry.stream(
                messages,
                prefer_backend=request.model_type,
                model_name=request.model_name,
                deployment_preference=getattr(request, "deployment_preference", "any"),
                max_tokens=2048,
                temperature=0.4 if skill.name in {"summarize_thread", "translate_text"} else 0.7,
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

            await self._memory_service.save_after_response(request.uid, session_id, request.content, accumulated)
            feedback_summary = self._feedback_evaluator.build_summary(skill.name, observations, accumulated)
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
            }
        except Exception as exc:
            logger.error("harness.stream.failed", error=str(exc))
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
            }
