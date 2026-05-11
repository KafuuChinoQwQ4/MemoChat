from __future__ import annotations

import json
from types import SimpleNamespace
from typing import Any, Callable

from harness.games.contracts import GameAction, GameParticipant, GameState


ParticipantNameResolver = Callable[[GameState, str], str]
TargetResolver = Callable[[GameState, str], str]


class AgentActionService:
    def __init__(
        self,
        agent_service,
        participant_public_name: ParticipantNameResolver,
        first_alive_target: TargetResolver,
    ):
        self._agent_service = agent_service
        self._participant_public_name = participant_public_name
        self._first_alive_target = first_alive_target

    async def ask_for_action(
        self,
        state: GameState,
        actor: GameParticipant,
        allowed: list[str],
    ) -> tuple[GameAction, str, str, bool]:
        target_hint = self._first_alive_target(state, actor.participant_id)
        if state.room.ruleset_id == "multi_ai_chat.test":
            private_inbox = self.private_a2a_timeline(state, actor.participant_id)
            prompt = {
                "task": "You are one AI speaker in a temporary group chat with a human and other AI speakers. Return only JSON.",
                "json_schema": {
                    "action_type": allowed,
                    "content": "natural short reply in Chinese unless the user clearly uses another language",
                    "target_participant_id": "required only when action_type is a2a_message; choose an alive agent participant other than yourself",
                    "payload": "object",
                },
                "ruleset_id": state.room.ruleset_id,
                "room_title": state.room.title,
                "self": actor.to_dict(include_private=False),
                "allowed_actions": allowed,
                "public_participants": [p.to_dict(include_private=False) for p in state.participants],
                "public_timeline": [event.to_dict() for event in state.events if event.visibility == "public"][-16:],
                "private_a2a_inbox": private_inbox[-12:],
                "persona": actor.persona,
                "strategy": str((actor.metadata or {}).get("strategy") or "balanced"),
                "instruction": (
                    "Reply as this AI only. Use speak for public chat. Use a2a_message only for a private agent-to-agent note "
                    "that should not be shown to the human or other agents. Do not mention game phases, hidden roles, winning, "
                    "voting, murder, or werewolf rules."
                ),
            }
        else:
            prompt = {
                "task": "You are an AI participant in a social deduction game. Return only JSON.",
                "json_schema": {
                    "action_type": allowed,
                    "content": "short in-character public message",
                    "target_participant_id": "optional participant id",
                    "payload": "object",
                },
                "ruleset_id": state.room.ruleset_id,
                "room": state.room.to_dict(),
                "self": actor.to_dict(),
                "allowed_actions": allowed,
                "suggested_target_participant_id": target_hint,
                "public_participants": [p.to_dict(include_private=False) for p in state.participants],
                "public_timeline": [event.to_dict() for event in state.events if event.visibility == "public"][-12:],
                "persona": actor.persona,
                "strategy": str((actor.metadata or {}).get("strategy") or "balanced"),
            }
        request = SimpleNamespace(
            uid=actor.uid,
            session_id=f"game-{state.room.room_id}-{actor.participant_id}",
            content=json.dumps(prompt, ensure_ascii=False),
            model_type=actor.model_type,
            model_name=actor.model_name,
            deployment_preference="any",
            skill_name=actor.skill_name or "writer",
            feature_type="",
            target_lang="",
            requested_tools=[],
            tool_arguments={},
            metadata={"source": "a2a_game", "room_id": state.room.room_id, "participant_id": actor.participant_id, "max_tokens": 512, "temperature": 0.6},
        )
        run_failed = False
        try:
            result = await self._agent_service.run_turn(request)
            raw = result.content or ""
            trace_id = result.trace_id
            payload = extract_json(raw)
        except Exception as exc:
            run_failed = True
            raw = f"{type(exc).__name__}: {exc}"
            trace_id = ""
            payload = {}

        if not run_failed and state.room.ruleset_id == "multi_ai_chat.test" and "speak" in allowed and not is_guardrail_block_output(raw):
            content = str(payload.get("content") or raw or "").strip()
            action_type = str(payload.get("action_type") or "").strip()
            if not action_type and content:
                action_type = "speak"
            if action_type not in allowed and content:
                action_type = "speak"
            if action_type == "speak":
                return (
                    GameAction(
                        participant_id=actor.participant_id,
                        action_type="speak",
                        content=content,
                        target_participant_id="",
                        payload=payload.get("payload") if isinstance(payload.get("payload"), dict) else {},
                    ),
                    trace_id,
                    raw,
                    True,
                )

        return (
            GameAction(
                participant_id=actor.participant_id,
                action_type=str(payload.get("action_type") or "skip"),
                content=str(payload.get("content") or ""),
                target_participant_id=str(payload.get("target_participant_id") or target_hint or ""),
                payload=payload.get("payload") if isinstance(payload.get("payload"), dict) else {},
            ),
            trace_id,
            raw,
            bool(payload),
        )

    def action_error_message(self, actor: GameParticipant, raw: str, fallback: str) -> str:
        suffix = f"已降级为 {fallback}。" if fallback else "已停止本次行动。"
        if is_guardrail_block_output(raw):
            return f"{actor.display_name} 本次没有生成有效内容，{suffix} 请检查模型/API Key、额度或模型服务状态。"
        return f"{actor.display_name} 返回了不可用行动，{suffix}"

    def private_a2a_timeline(self, state: GameState, participant_id: str) -> list[dict[str, Any]]:
        rows: list[dict[str, Any]] = []
        for event in state.events:
            if event.event_type != "a2a_message" or event.visibility != "private":
                continue
            if event.actor_participant_id != participant_id and event.target_participant_id != participant_id:
                continue
            direction = "outbound" if event.actor_participant_id == participant_id else "inbound"
            rows.append(
                {
                    **event.to_dict(),
                    "direction": direction,
                    "from_participant": self._participant_public_name(state, event.actor_participant_id),
                    "to_participant": self._participant_public_name(state, event.target_participant_id),
                }
            )
        return rows


def is_guardrail_block_output(raw: str) -> bool:
    return (raw or "").strip().lower().startswith("guardrail blocked")


def extract_json(content: str) -> dict[str, Any]:
    text = (content or "").strip()
    if not text:
        return {}
    try:
        data = json.loads(text)
    except json.JSONDecodeError:
        start = text.find("{")
        end = text.rfind("}")
        if start < 0 or end <= start:
            return {}
        try:
            data = json.loads(text[start : end + 1])
        except json.JSONDecodeError:
            return {}
    return data if isinstance(data, dict) else {}
