from __future__ import annotations

import time
import uuid
from typing import Any

from config import settings
from harness.contracts import (
    AgentCapabilities,
    AgentCard,
    AgentCardSkill,
    RemoteAgentRef,
    RemoteAgentTask,
)


def _now_ms() -> int:
    return int(time.time() * 1000)


class AgentInteropService:
    def __init__(self, skill_registry, handoff_service, public_base_url: str = ""):
        self._skill_registry = skill_registry
        self._handoff_service = handoff_service
        self._public_base_url = (public_base_url or self._default_base_url()).rstrip("/")
        self._remote_agents: dict[str, RemoteAgentRef] = {}
        self._remote_tasks: dict[str, RemoteAgentTask] = {}

    def local_agent_card(self) -> AgentCard:
        skills = [
            self._skill_from_agent_spec(spec)
            for spec in self._skill_registry.list_specs()
        ]
        skills.extend(
            self._skill_from_flow(flow)
            for flow in self._handoff_service.list_flows()
        )
        return AgentCard(
            name="MemoChat AI Agent",
            description=(
                "MemoChat local AI agent with traceable runs, tools, guardrails, "
                "memory, replay evals, and controlled multi-agent handoffs."
            ),
            url=f"{self._public_base_url}/agent/a2a",
            provider={
                "organization": "MemoChat",
                "url": self._public_base_url,
            },
            version="0.10.0",
            additional_interfaces=[
                {"url": f"{self._public_base_url}/agent/a2a", "transport": "JSONRPC"}
            ],
            capabilities=AgentCapabilities(
                streaming=True,
                push_notifications=False,
                state_transition_history=True,
            ),
            default_input_modes=["text/plain", "application/json"],
            default_output_modes=["text/plain", "application/json"],
            skills=skills,
            supports_authenticated_extended_card=False,
            metadata={
                "remoteInvocation": "disabled",
                "localEndpoints": {
                    "run": "/agent/run",
                    "a2a": "/agent/a2a",
                    "card": "/agent/card",
                    "flows": "/agent/flows",
                    "remoteAgents": "/agent/remote-agents",
                },
                "interopStage": "a2a-ready-placeholder",
            },
        )

    def list_remote_agents(self) -> list[RemoteAgentRef]:
        return list(self._remote_agents.values())

    def get_remote_agent(self, peer_id: str) -> RemoteAgentRef | None:
        return self._remote_agents.get((peer_id or "").strip())

    def register_remote_agent(
        self,
        name: str,
        card_url: str,
        base_url: str = "",
        trusted: bool = False,
        card: dict[str, Any] | None = None,
        metadata: dict[str, Any] | None = None,
        peer_id: str = "",
    ) -> RemoteAgentRef:
        normalized_id = (peer_id or "").strip() or uuid.uuid4().hex
        now = _now_ms()
        existing = self._remote_agents.get(normalized_id)
        ref = RemoteAgentRef(
            peer_id=normalized_id,
            name=(name or "").strip() or self._name_from_card(card or {}),
            card_url=(card_url or "").strip(),
            base_url=(base_url or "").strip(),
            status="registered",
            trusted=trusted,
            card=dict(card or {}),
            registered_at=existing.registered_at if existing else now,
            updated_at=now,
            metadata={
                **(existing.metadata if existing else {}),
                **(metadata or {}),
                "remoteInvocation": "disabled",
            },
        )
        self._remote_agents[normalized_id] = ref
        return ref

    def delete_remote_agent(self, peer_id: str) -> bool:
        return self._remote_agents.pop((peer_id or "").strip(), None) is not None

    def create_remote_task_placeholder(
        self,
        peer_id: str,
        input_payload: dict[str, Any] | None = None,
        metadata: dict[str, Any] | None = None,
    ) -> RemoteAgentTask:
        if self.get_remote_agent(peer_id) is None:
            raise ValueError(f"RemoteAgentRef not found: {peer_id}")
        now = _now_ms()
        task = RemoteAgentTask(
            task_id=uuid.uuid4().hex,
            peer_id=peer_id,
            status="blocked",
            input=dict(input_payload or {}),
            error="remote agent invocation is not enabled",
            created_at=now,
            updated_at=now,
            metadata={
                **(metadata or {}),
                "reason": "a2a_transport_not_configured",
            },
        )
        self._remote_tasks[task.task_id] = task
        return task

    def list_remote_tasks(self) -> list[RemoteAgentTask]:
        return list(self._remote_tasks.values())

    def _skill_from_agent_spec(self, spec) -> AgentCardSkill:
        category = str(spec.metadata.get("category", "agent"))
        return AgentCardSkill(
            id=f"agent:{spec.name}",
            name=spec.display_name,
            description=spec.description,
            tags=["agent_spec", category],
            examples=[],
            input_modes=["text/plain", "application/json"],
            output_modes=["text/plain", "application/json"],
            metadata={
                "kind": "agent_spec",
                "agentName": spec.name,
                "defaultModel": spec.default_model,
                "tools": list(spec.default_tools),
                "guardrails": dict(spec.guardrail_policy),
            },
        )

    def _skill_from_flow(self, flow) -> AgentCardSkill:
        return AgentCardSkill(
            id=f"flow:{flow.name}",
            name=flow.display_name,
            description=flow.description,
            tags=["agent_flow", str(flow.metadata.get("category", "workflow"))],
            input_modes=["text/plain", "application/json"],
            output_modes=["text/plain", "application/json"],
            metadata={
                "kind": "agent_flow",
                "flowName": flow.name,
                "steps": [step.to_dict() for step in flow.steps],
            },
        )

    def _default_base_url(self) -> str:
        port = settings.service.port
        return f"http://localhost:{port}"

    def _name_from_card(self, card: dict[str, Any]) -> str:
        value = card.get("name") if isinstance(card, dict) else ""
        return str(value or "Remote Agent")
