from __future__ import annotations

import time
import uuid
from types import SimpleNamespace
from typing import Any

from harness.contracts import AgentFlow, AgentMessage, HandoffStep, RunEdge, RunGraph, RunNode


def _now_ms() -> int:
    return int(time.time() * 1000)


class AgentHandoffService:
    def __init__(self, agent_service):
        self._agent_service = agent_service
        self._flows = {flow.name: flow for flow in self._builtin_flows()}

    def list_flows(self) -> list[AgentFlow]:
        return list(self._flows.values())

    def get_flow(self, flow_name: str) -> AgentFlow | None:
        return self._flows.get((flow_name or "").strip())

    async def run_flow(
        self,
        flow_name: str,
        uid: int,
        content: str,
        session_id: str = "",
        model_type: str = "",
        model_name: str = "",
        metadata: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        flow = self.get_flow(flow_name)
        if flow is None:
            raise ValueError(f"AgentFlow not found: {flow_name}")

        flow_id = uuid.uuid4().hex
        started_at = _now_ms()
        messages: list[AgentMessage] = []
        step_outputs: dict[str, str] = {}
        trace_ids: dict[str, str] = {}
        nodes: list[RunNode] = []
        edges: list[RunEdge] = []
        previous_node_id = ""
        current_content = content
        status = "completed"

        for index, step in enumerate(flow.steps):
            step_started = _now_ms()
            step_input = self._render_step_input(step, content, current_content, step_outputs)
            request = SimpleNamespace(
                uid=uid,
                session_id=session_id,
                content=step_input,
                model_type=model_type,
                model_name=model_name,
                deployment_preference="any",
                skill_name=step.agent_name,
                feature_type="",
                target_lang="",
                requested_tools=[],
                tool_arguments={},
                metadata={
                    **(metadata or {}),
                    "flow_id": flow_id,
                    "flow_name": flow.name,
                    "handoff_step": step.step_id,
                    "source": "agent_flow",
                },
            )

            try:
                result = await self._agent_service.run_turn(request)
                current_content = result.content
                step_outputs[step.step_id] = result.content
                trace_ids[step.step_id] = result.trace_id
                step_status = "ok"
                detail = result.content[:1000]
            except Exception as exc:
                status = "failed"
                current_content = f"{type(exc).__name__}: {str(exc)[:300]}"
                step_outputs[step.step_id] = current_content
                step_status = "failed"
                detail = current_content

            to_agent = flow.steps[index + 1].agent_name if index + 1 < len(flow.steps) else "user"
            messages.append(
                AgentMessage(
                    message_id=uuid.uuid4().hex,
                    flow_id=flow_id,
                    from_agent=step.agent_name,
                    to_agent=to_agent,
                    role="handoff" if to_agent != "user" else "final",
                    content=current_content,
                    trace_id=trace_ids.get(step.step_id, ""),
                    created_at=_now_ms(),
                    metadata={"step_id": step.step_id, "flow_name": flow.name},
                )
            )

            node_id = f"{flow_id}:handoff:{step.step_id}"
            nodes.append(
                RunNode(
                    node_id=node_id,
                    name=step.step_id,
                    layer="handoff",
                    status=step_status,
                    summary=f"{step.agent_name} -> {to_agent}",
                    detail=detail,
                    parent_id=previous_node_id,
                    started_at=step_started,
                    finished_at=_now_ms(),
                    metadata={
                        "agent": step.agent_name,
                        "to_agent": to_agent,
                        "trace_id": trace_ids.get(step.step_id, ""),
                    },
                )
            )
            if previous_node_id:
                edges.append(RunEdge(source_id=previous_node_id, target_id=node_id, relation="handoff"))
            previous_node_id = node_id
            if status == "failed":
                break

        graph = RunGraph(
            trace_id=flow_id,
            nodes=nodes,
            edges=edges,
            status=status,
            started_at=started_at,
            finished_at=_now_ms(),
            metadata={
                "flow_name": flow.name,
                "message_count": len(messages),
                "trace_ids": trace_ids,
            },
        )
        return {
            "flow": flow,
            "flow_id": flow_id,
            "status": status,
            "messages": messages,
            "graph": graph,
        }

    def _render_step_input(
        self,
        step: HandoffStep,
        original_content: str,
        current_content: str,
        step_outputs: dict[str, str],
    ) -> str:
        previous = current_content or original_content
        template = step.input_template or "{instruction}\n\n用户原始请求:\n{original}\n\n上一阶段输出:\n{previous}"
        return template.format(
            instruction=step.instruction,
            original=original_content,
            previous=previous,
            outputs=step_outputs,
        )

    def _builtin_flows(self) -> list[AgentFlow]:
        return [
            AgentFlow(
                name="research_write_review",
                display_name="Research -> Write -> Review",
                description="Research evidence, draft an answer, then review it before returning to the user.",
                steps=[
                    HandoffStep(
                        step_id="research",
                        agent_name="researcher",
                        instruction="收集证据，输出结论、依据和不确定点。",
                    ),
                    HandoffStep(
                        step_id="write",
                        agent_name="writer",
                        instruction="基于研究结果写出可直接给用户的回答。",
                        depends_on=["research"],
                    ),
                    HandoffStep(
                        step_id="review",
                        agent_name="reviewer",
                        instruction="审阅草稿，指出风险并给出最终修订版。",
                        depends_on=["write"],
                    ),
                ],
                metadata={"category": "knowledge_work"},
            ),
            AgentFlow(
                name="support_triage_answer",
                display_name="Support Triage -> KB Answer",
                description="Triage a support issue, consult knowledge, and produce a concise answer.",
                steps=[
                    HandoffStep(
                        step_id="triage",
                        agent_name="support",
                        instruction="识别用户问题类型、影响范围和最短排查路径。",
                    ),
                    HandoffStep(
                        step_id="knowledge_lookup",
                        agent_name="researcher",
                        instruction="围绕排查路径检索知识库或证据，列出可验证依据。",
                        depends_on=["triage"],
                    ),
                    HandoffStep(
                        step_id="answer",
                        agent_name="support",
                        instruction="整合排查和证据，输出简洁支持答复。",
                        depends_on=["knowledge_lookup"],
                    ),
                ],
                metadata={"category": "support"},
            ),
            AgentFlow(
                name="document_curate_review",
                display_name="Document Curate -> Review",
                description="Summarize document-like input into durable knowledge and review quality.",
                steps=[
                    HandoffStep(
                        step_id="curate",
                        agent_name="knowledge_curator",
                        instruction="提取主题、事实、术语、待补充项和可归档摘要。",
                    ),
                    HandoffStep(
                        step_id="review",
                        agent_name="reviewer",
                        instruction="检查整理结果是否保留事实、标注不确定性、避免过度改写。",
                        depends_on=["curate"],
                    ),
                ],
                metadata={"category": "knowledge"},
            ),
        ]
