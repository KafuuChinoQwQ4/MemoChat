from __future__ import annotations

import json
import time
import uuid
from typing import Any

import structlog

from db.postgres_client import PostgresClient
from harness.contracts import AgentTrace, PlanStep, TraceEvent
from config import settings

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


class AgentTraceStore:
    def __init__(self):
        self._traces: dict[str, AgentTrace] = {}

    async def start_run(
        self,
        uid: int,
        session_id: str,
        skill_name: str,
        request_summary: str,
        plan_steps: list[PlanStep],
    ) -> AgentTrace:
        trace = AgentTrace(
            trace_id=uuid.uuid4().hex,
            uid=uid,
            session_id=session_id,
            skill=skill_name,
            request_summary=request_summary[:1000],
            plan_steps=plan_steps,
            started_at=_now_ms(),
        )
        self._traces[trace.trace_id] = trace
        await self._persist_run_start(trace)
        return trace

    async def add_event(self, trace_id: str, event: TraceEvent) -> None:
        trace = self._traces.get(trace_id)
        if trace is None:
            return
        trace.events.append(event)
        await self._persist_event(trace_id, len(trace.events) - 1, event)

    async def finish_run(
        self,
        trace_id: str,
        status: str,
        response_content: str,
        model: str,
        feedback_summary: str,
        observations: list[str],
    ) -> None:
        trace = self._traces.get(trace_id)
        if trace is None:
            return
        trace.status = status
        trace.response_content = response_content
        trace.model = model
        trace.feedback_summary = feedback_summary
        trace.observations = observations
        trace.finished_at = _now_ms()
        await self._persist_run_finish(trace)

    async def submit_feedback(
        self,
        trace_id: str,
        uid: int,
        rating: int,
        feedback_type: str,
        comment: str,
        payload: dict[str, Any],
    ) -> None:
        try:
            pg = PostgresClient()
            await pg.execute(
                """
                INSERT INTO ai_agent_feedback
                (trace_id, uid, rating, feedback_type, comment, payload_json, created_at)
                VALUES ($1, $2, $3, $4, $5, $6::jsonb, $7)
                """,
                trace_id,
                uid,
                rating,
                feedback_type,
                comment,
                json.dumps(payload, ensure_ascii=False),
                _now_ms(),
            )
        except Exception as exc:
            logger.warning("harness.feedback.persist_failed", trace_id=trace_id, error=str(exc))

    def get_trace(self, trace_id: str) -> AgentTrace | None:
        return self._traces.get(trace_id)

    async def _persist_run_start(self, trace: AgentTrace) -> None:
        if not settings.harness.trace_persist:
            return
        try:
            pg = PostgresClient()
            await pg.execute(
                """
                INSERT INTO ai_agent_run_trace
                (trace_id, uid, session_id, skill_name, request_summary, plan_json, status, started_at, updated_at)
                VALUES ($1, $2, $3, $4, $5, $6::jsonb, $7, $8, $8)
                """,
                trace.trace_id,
                trace.uid,
                trace.session_id,
                trace.skill,
                trace.request_summary,
                json.dumps([step.to_dict() for step in trace.plan_steps], ensure_ascii=False),
                trace.status,
                trace.started_at,
            )
        except Exception as exc:
            logger.warning("harness.trace.start_persist_failed", trace_id=trace.trace_id, error=str(exc))

    async def _persist_event(self, trace_id: str, step_index: int, event: TraceEvent) -> None:
        if not settings.harness.trace_persist:
            return
        try:
            pg = PostgresClient()
            await pg.execute(
                """
                INSERT INTO ai_agent_run_step
                (trace_id, step_index, layer, step_name, status, summary, detail, metadata_json, duration_ms, created_at)
                VALUES ($1, $2, $3, $4, $5, $6, $7, $8::jsonb, $9, $10)
                """,
                trace_id,
                step_index,
                event.layer,
                event.name,
                event.status,
                event.summary,
                event.detail,
                json.dumps(event.metadata, ensure_ascii=False),
                max(event.finished_at - event.started_at, 0),
                event.finished_at or _now_ms(),
            )
        except Exception as exc:
            logger.warning("harness.trace.event_persist_failed", trace_id=trace_id, error=str(exc))

    async def _persist_run_finish(self, trace: AgentTrace) -> None:
        if not settings.harness.trace_persist:
            return
        try:
            pg = PostgresClient()
            await pg.execute(
                """
                UPDATE ai_agent_run_trace
                SET response_content = $2,
                    model_name = $3,
                    status = $4,
                    feedback_summary = $5,
                    observations_json = $6::jsonb,
                    finished_at = $7,
                    updated_at = $7
                WHERE trace_id = $1
                """,
                trace.trace_id,
                trace.response_content,
                trace.model,
                trace.status,
                trace.feedback_summary,
                json.dumps(trace.observations, ensure_ascii=False),
                trace.finished_at,
            )
        except Exception as exc:
            logger.warning("harness.trace.finish_persist_failed", trace_id=trace.trace_id, error=str(exc))
