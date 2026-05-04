from __future__ import annotations

import json
import time
import uuid
from typing import Any

import structlog

from db.postgres_client import PostgresClient
from harness.contracts import AgentTrace, PlanStep, RunEdge, RunGraph, RunNode, TraceEvent
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
        trace.run_graph = self.build_run_graph(trace)
        await self._persist_run_start(trace)
        return trace

    async def add_event(self, trace_id: str, event: TraceEvent) -> None:
        trace = self._traces.get(trace_id)
        if trace is None:
            return
        trace.events.append(event)
        trace.run_graph = self.build_run_graph(trace)
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
        trace.run_graph = self.build_run_graph(trace)
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

    async def get_trace_or_load(self, trace_id: str) -> AgentTrace | None:
        trace = self.get_trace(trace_id)
        if trace is not None:
            return trace
        if not settings.harness.trace_persist:
            return None

        try:
            pg = PostgresClient()
            row = await pg.fetchone(
                """
                SELECT trace_id, uid, session_id, skill_name, request_summary,
                       response_content, plan_json, observations_json, model_name,
                       status, feedback_summary, started_at, finished_at
                FROM ai_agent_run_trace
                WHERE trace_id = $1 AND deleted_at IS NULL
                """,
                trace_id,
            )
            if not row:
                return None

            step_rows = await pg.fetchall(
                """
                SELECT step_index, layer, step_name, status, summary, detail,
                       metadata_json, duration_ms, created_at
                FROM ai_agent_run_step
                WHERE trace_id = $1
                ORDER BY step_index ASC
                """,
                trace_id,
            )
        except Exception as exc:
            logger.warning("harness.trace.load_failed", trace_id=trace_id, error=str(exc))
            return None

        trace = AgentTrace(
            trace_id=row["trace_id"],
            uid=row["uid"],
            session_id=row["session_id"],
            skill=row["skill_name"],
            request_summary=row["request_summary"],
            plan_steps=self._decode_plan_steps(row["plan_json"]),
            events=[self._decode_event(step_row) for step_row in step_rows],
            observations=self._decode_list(row["observations_json"]),
            status=row["status"],
            response_content=row["response_content"],
            model=row["model_name"],
            feedback_summary=row["feedback_summary"],
            started_at=row["started_at"],
            finished_at=row["finished_at"] or 0,
        )
        trace.run_graph = self.build_run_graph(trace)
        self._traces[trace.trace_id] = trace
        return trace

    async def get_run_graph_or_load(self, trace_id: str) -> RunGraph | None:
        trace = await self.get_trace_or_load(trace_id)
        if trace is None:
            return None
        if trace.run_graph is None:
            trace.run_graph = self.build_run_graph(trace)
        return trace.run_graph

    def build_run_graph(self, trace: AgentTrace) -> RunGraph:
        nodes: list[RunNode] = []
        edges: list[RunEdge] = []
        started_at = trace.started_at or _now_ms()
        previous_id = ""

        plan_node_id = f"{trace.trace_id}:plan"
        plan_finished_at = trace.events[0].started_at if trace.events else trace.finished_at or started_at
        nodes.append(
            RunNode(
                node_id=plan_node_id,
                name="plan",
                layer="orchestration",
                status="ok",
                summary=f"steps={len(trace.plan_steps)}",
                detail=json.dumps([step.to_dict() for step in trace.plan_steps], ensure_ascii=False),
                started_at=started_at,
                finished_at=max(plan_finished_at, started_at),
                metadata={"step_count": len(trace.plan_steps)},
            )
        )
        previous_id = plan_node_id

        for index, event in enumerate(trace.events):
            node_id = f"{trace.trace_id}:event:{index}"
            nodes.append(
                RunNode(
                    node_id=node_id,
                    name=event.name,
                    layer=event.layer,
                    status=event.status,
                    summary=event.summary,
                    detail=event.detail,
                    parent_id=previous_id,
                    started_at=event.started_at,
                    finished_at=event.finished_at,
                    metadata=dict(event.metadata),
                )
            )
            edges.append(
                RunEdge(
                    source_id=previous_id,
                    target_id=node_id,
                    relation="next",
                )
            )
            previous_id = node_id

        return RunGraph(
            trace_id=trace.trace_id,
            nodes=nodes,
            edges=edges,
            status=trace.status,
            started_at=trace.started_at or started_at,
            finished_at=trace.finished_at or 0,
            metadata={
                "skill": trace.skill,
                "event_count": len(trace.events),
            },
        )

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

    def _decode_plan_steps(self, raw_value: Any) -> list[PlanStep]:
        items = self._decode_list(raw_value)
        steps: list[PlanStep] = []
        for item in items:
            if not isinstance(item, dict):
                continue
            parameters = item.get("parameters", {})
            steps.append(
                PlanStep(
                    action=str(item.get("action", "")),
                    reason=str(item.get("reason", "")),
                    parameters=parameters if isinstance(parameters, dict) else {},
                )
            )
        return steps

    def _decode_event(self, row: Any) -> TraceEvent:
        created_at = int(row["created_at"] or _now_ms())
        duration_ms = int(row["duration_ms"] or 0)
        return TraceEvent(
            layer=row["layer"],
            name=row["step_name"],
            status=row["status"],
            summary=row["summary"],
            detail=row["detail"],
            started_at=max(created_at - duration_ms, 0),
            finished_at=created_at,
            metadata=self._decode_dict(row["metadata_json"]),
        )

    def _decode_list(self, raw_value: Any) -> list[Any]:
        if raw_value is None:
            return []
        if isinstance(raw_value, list):
            return raw_value
        if isinstance(raw_value, str):
            try:
                decoded = json.loads(raw_value)
                return decoded if isinstance(decoded, list) else []
            except Exception:
                return []
        return []

    def _decode_dict(self, raw_value: Any) -> dict[str, Any]:
        if raw_value is None:
            return {}
        if isinstance(raw_value, dict):
            return raw_value
        if isinstance(raw_value, str):
            try:
                decoded = json.loads(raw_value)
                return decoded if isinstance(decoded, dict) else {}
            except Exception:
                return {}
        return {}
